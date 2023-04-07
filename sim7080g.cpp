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

    return val;
}

//
SIM7080G::SIM7080G(bool openUART) {
    //Setup DTR key
    pinMode(dtrKey, OUTPUT);
    digitalWrite(dtrKey, LOW);

    if(openUART)
        OpenUART();
}

//  #
//  #   IO / Power control
//  #

//
void SIM7080G::PowerUp() {
#if defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("DEBUG START: PowerUp()\n");
#endif
    //Test if device is already powered up
    if (TestUART()){
#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
        uartDebugInterface.printf("\tDevice already powered up! Nothing to do here...\nDEBUG END: PowerUp()\n");
#endif
        pwrState = SIM_PWUP;
        return;
    }

    //Power cycle device
    if(pwrState == SIM_PWDN) {
#if defined SIM7080G_DEBUG_ALL
        uartDebugInterface.printf("\tPowering up device now...\nDEBUG END: PowerUp()\n");
#endif
        pwrState = SIM_PWUP;
        PowerCycle();
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
    SendCommand("AT+CREBOOT\r\n");
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

//
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
    uartInterface.flush(false);
}

//
size_t SIM7080G::AvailableUART() {
    return uartInterface.available();
}

//
size_t SIM7080G::SendCommand(char* command, char* response) {
    //Send command
    uartInterface.print(command);
    //uartInterface.write("\r");

    //Wait for response
    delay(uartResponseTimeout);

    //Read data from device
    size_t bytesRecv = 0;
    if(response) {
        while(uartInterface.available() && bytesRecv < uartMaxRecvSize)
            response[bytesRecv++] = (char)uartInterface.read();
        response[bytesRecv] = 0;
    }

#if defined SIM7080G_DEBUG_ALL
    //Command debug
    uartDebugInterface.printf("DEBUG START: SendCommand(char*, char*)\n");

    for(size_t j  = 0; j < strlen(command); j++)
        uartDebugInterface.printf("\t%d: %c - %d\n", j, command[j], command[j]);

    uartDebugInterface.printf("\tCommand length: %d\n", strlen(command));

    if(response) {
        printf("\tBytes received: %d\n\tResponse:", bytesRecv);
        for (size_t i = 0; i < bytesRecv; i++) 
            uartDebugInterface.printf("\t%d: %c - %d\n", i, response[i], response[i]);
    }
    else
        printf("\tIgnoring return value.\n");

    uartDebugInterface.printf("Debug message END: SendCommand(char*, char*)\n");
#endif

    return bytesRecv;
}

//
bool SIM7080G::SendCommand(char* command) {
    size_t bytesRecv = SendCommand(command, rxBufer);

#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
    //Command debug
    uartDebugInterface.printf("DEBUG START: SendCommand(char*)\n");
    if(bytesRecv >= 2)
        uartDebugInterface.printf("\tCommand result: %c - %s\n", rxBufer[bytesRecv - 2], (bytesRecv >= 2 && rxBufer[bytesRecv - 2] == '0' ? "SUCCESSFUL" : "FAILED"));
    else
        uartDebugInterface.printf("\tNot enough bytes received! Bytes received: %d\n", bytesRecv);
    uartDebugInterface.printf("Debug message END: SendCommand(char*)\n");
#endif

    return bytesRecv >= 2 && rxBufer[bytesRecv - 2] == '0';
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

//
bool SIM7080G::TestUART() {
    //FlushUART();
    return SendCommand("AT+CGMI=?\r");
}

void SIM7080G::SetTAResponseFormat(bool textResponse) {
    //FlushUART();
    if(textResponse)
        SendCommand("ATV1\r");
    else
        SendCommand("ATV0\r");
}

void SIM7080G::NOOP() {
    SendCommand("\r");
}

//  #
//  #   GNSS Application
//  #

//
bool SIM7080G::PowerUpGNSS() { return SendCommand("AT+CGNSPWR=1\r"); }

//
bool SIM7080G::PowerDownGNSS() { return SendCommand("AT+CGNSPWR=0\r"); }

//
bool SIM7080G::ColdStartGNSS() { return SendCommand("AT+CGNSCOLD\r"); }

//
bool SIM7080G::WarmStartGNSS() { return SendCommand("AT+CGNSWARM\r"); }

//
bool SIM7080G::HotStartGNSS() { return SendCommand("AT+CGNSHOT\r"); }

//
void SIM7080G::GetGNSS(SIM7080G_GNSS* dst) {

    //get GNSS info from device
    size_t bytesrecv = SendCommand("AT+CGNSINF\r", rxBufer);

    
    
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

//
SIM7080G_GNSS SIM7080G::GetGNSS(void) {
    SIM7080G_GNSS gnssInfo;
    GetGNSS(&gnssInfo);
    return gnssInfo;
}


//  #
//  #   Power
//  #

//uint16_t SIM7080G::GetVBat(void) const {

//}

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

//
void SIM7080G::EraseRXBuff(uint32_t value) {
    size_t rounds = this->uartMaxRecvSize / 4;
    //Erase buffer with 32 bit numbers for efficiency
    for(size_t i = 0; i < rounds; i+=4)
        rxBufer[i] = value;
}