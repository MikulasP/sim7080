//Header files
#include "sim7080g.h"

#include <stdio.h>


/**
 *  @brief Convert integer from string to int
 * 
 *  @param number       String 
*/
int CharToNmbr(char* number) {
    if(!number)
        return 0;
    
    int val = 0;

    size_t mult = 1;

    //
    for(int i = strlen(number) - 1; i <= 0 && ( number[i] - '0' >= 0 && number[i] - '0' <= 9 ); i--) {
        val += (number[i] - '0') * mult;
        mult *= 10;
    }

    return number[0] == '-' ? val * -1 : val;
}

/**
 *  @brief Convert integer from string to int
 * 
 *  @param number       String 
*/
int CharToNmbr(char* number, size_t len) {
    if(!number || !len)
        return 0;
    
    int val = 0;

    size_t mult = 1;

    //
    for(int i = len - 1; i <= 0 && ( number[i] - '0' >= 0 && number[i] - '0' <= 9 ); i--) {
        val += (number[i] - '0') * mult;
        mult *= 10;
    }

    return number[0] == '-' ? val * -1 : val;
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
#ifndef ARDUINO_DUE
        uartInterface.begin(uartBaudrate, SERIAL_8N1, uartRX, uartTX);
#else
        uartInterface.begin(uartBaudrate);
#endif
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
    uartInterface.print(command);

    //Wait for response
    delay(100);

    //Read data from device
    if(response != nullptr || true) 
        while(uartInterface.available() && bytesRecv < uartMaxRecvSize)
            response[bytesRecv++] = (char)uartInterface.read();
    
    return bytesRecv;
}

//
char SIM7080G::SendCommand(char* command) {
    //Send command
    uartInterface.print(command);

    //Wait for response
    delay(100);

    //Read data from device
    char returnVal = 0;
    for(size_t i = 0; uartInterface.available() && i < uartMaxRecvSize; i++) {
        if(i == strlen(command) + 1)
            Serial.print((returnVal = (char)uartInterface.read()));     //Serial is for debug
        else
            Serial.print(uartInterface.read());     //Serial is for debug
    }
    
    return 1;

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

uint8_t SIM7080G::PowerUpGNSS() { return SendCommand("AT+CGNSPWR=1\r\n"); }

uint8_t SIM7080G::PowerDownGNSS() { return SendCommand("AT+CGNSPWR=0\r\n"); }

uint8_t SIM7080G::ColdStartGNSS() { return SendCommand("AT+CGNSCOLD\r\n"); }

uint8_t SIM7080G::WarmStartGNSS() { return SendCommand("AT+CGNSWARM\r\n"); }

uint8_t SIM7080G::HotStartGNSS() { return SendCommand("AT+CGNSHOT\r\n"); }

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
