//Header files
#include "sim7080g.h"

#include <stdio.h>


//  "It ain't much but it's honest work"
/**
 *  @brief Convert integer from string to int
 * 
 *  @param number       Start of char array containing the numbers
 *  @param len          Length of the string to process (if 0 whole string will be processed)
*/
int CharToNmbr(char* number, size_t len = 0) {
    if(!number)
        return 0;
    
    if (!len)
        len = strlen(number);

    int val = 0;

    size_t mult = 1;

    //
    for(int i = len - 1; i >= 0; i--) {
        if(number[i] == '-') {
            val *= -1;
            continue;
        }
        
        if (number[i] - '0' >= 0 && number[i] - '0' <= 9) {
            val += (number[i] - '0') * mult;
            mult *= 10;
        }
    }

    return val; //number[0] == '-' ? val * -1 : val;
}

SIM7080G::SIM7080G() {
    //Setup DTR key
    pinMode(dtrKey, OUTPUT);
    digitalWrite(dtrKey, LOW);
}

//  #
//  #   IO / Power control
//  #

//
void SIM7080G::PowerUp() {
    if(pwrState == SIM_PWDN) {
        PowerCycle();
        pwrState = SIM_PWUP;
    }
}

//
void SIM7080G::PowerDown() {
    if(pwrState == SIM_PWUP) {
        PowerCycle();
        pwrState = SIM_PWDN;
    }
}

//
void SIM7080G::Reboot() {
    SendCommand("AT+CREBOOT\r\n", nullptr);
}

//
void SIM7080G::EnterSleep() {
    if(pwrState == SIM_PWUP) {
        digitalWrite(dtrKey, HIGH);
        pwrState = SIM_SLEEP;
    }
}

//
void SIM7080G::LeaveSleep() {
    if (pwrState == SIM_SLEEP) {
        digitalWrite(dtrKey, LOW);
        pwrState = SIM_PWUP;
    }
}

SIM7080G_PWR SIM7080G::GetPowerState() const {
    return pwrState;
}

//  #
//  #   UART
//  #

//
void SIM7080G::OpenUART() {
    if(!uartOpen) {
        uartInterface.begin(uartBaudrate, SERIAL_8N1, uartRX, uartTX);
        uartOpen = true;
    }
}

//
void SIM7080G::CloseUART() {
    if(uartOpen) {
        uartInterface.end();
        uartOpen = false;
    }
}

//
void SIM7080G::FlushUART() {
    uartInterface.flush();
}

//
size_t SIM7080G::AvailableUART() {
    return uartInterface.available();
}

//
size_t SIM7080G::SendCommand(char* command, char* response) {
    size_t bytesRecv = 0;
    
    //Send command
    uartInterface.write(command, strlen(command));
    uartInterface.write("\r\n");

    //Wait for response
    delay(uartResponseTimeout);

    //Read data from device
    if(response != nullptr)
        while(uartInterface.available() && bytesRecv < uartMaxRecvSize)
            response[bytesRecv++] = (char)uartInterface.read();
    
    return bytesRecv;
}

//
bool SIM7080G::SendCommand(char* command) {
    //Send command
    uartInterface.write(command, strlen(command));
    uartInterface.write("\r\n");

    //Wait for response
    delay(uartResponseTimeout);

    //Read data from device
    size_t bytesRecv;
    for(bytesRecv = 0; uartInterface.available() && bytesRecv < uartMaxRecvSize; bytesRecv++)
        rxBufer[bytesRecv] = (char)uartInterface.read();
    
    return strncmp(rxBufer + bytesRecv - 4, "OK", 2) == 0;
}

//
void SIM7080G::Send(uint8_t* src, size_t len) {
    uartInterface.write(src, len);
}

//
size_t SIM7080G::Receive(uint8_t* dst, size_t len) {
    size_t bytesRecv = 0;

    for (;uartInterface.available() && (len ? bytesRecv < len : true);) 
        dst[bytesRecv++] = (char)uartInterface.read();
    
    return bytesRecv;
}

//
bool SIM7080G::GetUART() const { return uartOpen; }

//
uint64_t SIM7080G::GetBaudrate() const { return uartBaudrate; }

//  #
//  #   GNSS Application
//  #

bool SIM7080G::PowerUpGNSS() { return SendCommand("AT+CGNSPWR=1"); }

bool SIM7080G::PowerDownGNSS() { return SendCommand("AT+CGNSPWR=0"); }

bool SIM7080G::ColdStartGNSS() { return SendCommand("AT+CGNSCOLD"); }

bool SIM7080G::WarmStartGNSS() { return SendCommand("AT+CGNSWARM"); }

bool SIM7080G::HotStartGNSS() { return SendCommand("AT+CGNSHOT"); }

void SIM7080G::GetGNSS(SIM7080G_GNSS* dst) {
    
    //get GNSS info from device
    size_t bytesrecv = SendCommand((char*)"", this->rxBufer);

    
    
    uint8_t comas = 0;      //Track number of comas in response text
    uint8_t infCntr = 0;    //For indexing SIM7080G_GNSS arrays

    for (uint8_t i = 0; i < bytesrecv; i++) {
        if(rxBufer[i] == ','){
            comas++;
            infCntr = 0;
            continue;
        }

        //
        switch (comas)
        {
        case 0:     //GNSS Run status
            dst->run = rxBufer[i] - '0';
            break;

        case 2:     //UTC date & time
            dst->datetime[infCntr++] = rxBufer[i];
            break;

        case 3:     //Latitude
            dst->latitude[infCntr++] = rxBufer[i];
            break;

        case 4:     //Longitude
            dst->longitude[infCntr++] = rxBufer[i];
            break;
        
        case 14:    //GPS Satellites in view
            dst->gpsSat = (rxBufer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
            break;

        case 15:    //GNSS Satellites in view
            dst->gnssSat = (rxBufer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
            break;

        case 16:    //GNSS Satellites in view
            dst->glonassSat = (rxBufer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
            break;

        default:
            break;
        }
    }

}

SIM7080G_GNSS SIM7080G::GetGNSS(void) {
    SIM7080G_GNSS gnssInfo;
    GetGNSS(&gnssInfo);
    return gnssInfo;
}


//  #
//  #   Power
//  #

uint16_t SIM7080G::GetVBat(void) const {

}

//  #
//  #   Private functions
//  #

//
void SIM7080G::PowerCycle() {
    pinMode(pwrKey, OUTPUT);
    digitalWrite(pwrKey, LOW);
    delay(1100);
    digitalWrite(pwrKey, HIGH);
    pinMode(pwrKey, INPUT);     //Leave pin floating
}
