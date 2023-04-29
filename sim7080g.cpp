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
SIM7080G::SIM7080G(uint8_t rx, uint8_t tx, uint8_t pwr, int dtr, bool openUART) {
    this->uartRX = rx;
    this->uartTX = tx;
    this->pwrKey = pwr;
    this->dtrKey = dtr;

    //Setup DTR key
    if (dtr >= 0) {
        pinMode(dtrKey, OUTPUT);
        digitalWrite(dtrKey, LOW);
    }

    if(openUART) {
        OpenUART();
        SetTAResponseFormat();
    }
}

//  #
//  #   IO / Power control
//  #

void SIM7080G::SetDTR(int dtr) {
    //Setup DTR key
    if (dtr >= 0) {
        pinMode(dtrKey, OUTPUT);
        digitalWrite(dtrKey, LOW);
    }
}

//
void SIM7080G::PowerUp() {
#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
    uartDebugInterface.printf("DEBUG START: PowerUp()\n");
#endif
    //Test if device is already powered up
    if (TestUART()){
#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
        uartDebugInterface.printf("\tDevice already powered up! Nothing to do here...\nDEBUG END: PowerUp()\n");
#elif defined SIM7080G_VERBOSE
        uartDebugInterface.printf("\tSIM7080G - Device already powered up! Nothing to do here...\n");
#endif
        pwrState = SIM_PWUP;
        return;
    }

    //Power cycle device
    if(pwrState == SIM_PWDN) {
#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
        uartDebugInterface.printf("\tPowering up device...\nDEBUG END: PowerUp()\n");
#elif defined SIM7080G_VERBOSE
        uartDebugInterface.printf("\tSIM7080G - Powering up device...\n");
#endif
        pwrState = SIM_PWUP;
        PowerCycle();
        delay(2000);    //Min delay specified is 1.8s
        SetTAResponseFormat();
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
size_t SIM7080G::SendCommand(const char* command, char* response, uint32_t recvTimeout) {
    //Send command
    uartInterface.print(command);
    //uartInterface.write("\r");

    //Wait for response
    delay(recvTimeout ? recvTimeout : uartRecvtimeout);

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

    uartDebugInterface.printf("DEBUG END: SendCommand(char*, char*)\n");
#endif

    return bytesRecv;
}

//
bool SIM7080G::SendCommand(const char* command, uint32_t recvTimeout) {
    size_t bytesRecv = SendCommand(command, rxBufer, recvTimeout);

    bool result = bytesRecv >= 2 && rxBufer[bytesRecv - 2] == '0';

#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG
    //Command debug
    uartDebugInterface.printf("DEBUG START: SendCommand(char*)\n");
    if(bytesRecv >= 2)
        uartDebugInterface.printf("\tCommand result: %c - %s\n", rxBufer[bytesRecv - 2], (result ? "SUCCESSFUL" : "FAILED"));
    else
        uartDebugInterface.printf("\tNot enough bytes received! Bytes received: %d\n", bytesRecv);
    uartDebugInterface.printf("DEBUG END: SendCommand(char*)\n");
#elif defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - Command: %s | Result: %s\n", command, (result ? "SUCCESSFUL" : "FAILED"));
#endif

    return result;
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
    return SendCommand("AT+CGMI=?\r");
}

void SIM7080G::SetTAResponseFormat(bool textResponse) {
    SendCommand(textResponse ? (char*)"ATV1\r" : (char*)"ATV0\r");
}

//  #
//  #   Cellular communication
//  #

//
uint8_t SIM7080G::GetNetworkReg(void) {
    size_t bytesRecv = SendCommand("AT+CREG?\r", rxBufer);
    uint8_t status = *(strrchr(rxBufer, ',') + 1) - '0';
#if defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - Network Registration status: %d\n", status);
#endif
    return status;
}


uint8_t SIM7080G::GetSignalQuality() {
    
    size_t bytesRecv = SendCommand("AT+CSQ\r", rxBufer);

    char* startPtr = rxBufer;
    while ((*startPtr) != ':' && (*startPtr) != '\0')
        startPtr++;
    startPtr++;     //Step forward another

    uint8_t offset = 0;
    for (; startPtr[offset] != ',' && startPtr[offset] != '\0'; offset++);

    //Check against mem overflow
    return bytesRecv <= uartMaxRecvSize && startPtr + 2 <= rxBufer + uartMaxRecvSize ? CharToNmbr(startPtr, offset) : 99;
}


//void SIM7080G::GetCellOperators(HardwareSerial& debugInterface);


//size_t SIM7080G::GetCellOperators(char* dst);


//void SIM7080G::SetCellOperator(char* opName);


//uint8_t SIM7080G::GetCellFunction(void);


//void SIM7080G::SetCellFunction(uint8_t functionCode);


//void SIM7080G::GetTime(char* dst);

//
bool SIM7080G::EnterPIN(const char* pin, bool force) {
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("DEBUG START: EnterPin(%s)\n", pin);
#endif
    
    //SIM PIN is already entered
    if (GetPINStatus()) {
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
        uartDebugInterface.printf("\tSIM PIN READY\nDEBUG END: EnterPin(%s)\n", pin);
#elif defined SIM7080G_VERBOSE
        uartDebugInterface.printf("\tSIM7080G - SIM PIN is already entered!\n");
#endif
        return true;
    }

    if (strlen(pin) == 4)
    {
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
        uartDebugInterface.printf("\tPin format OK!\nDEBUG END: EnterPin(%s)\n", pin);
#endif
        char tmpBuff[14] = {'A', 'T', '+', 'C', 'P', 'I', 'N', '=', '*', '*', '*', '*', '\r', '\0'};
        memcpy(tmpBuff + 8, pin, 4);
        return SendCommand(tmpBuff);
    }
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("\tPin format ERROR!\nDEBUG END: EnterPin(%s)\n", pin);
#elif defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - SIM PIN format ERROR!");
#endif
    return false;
}


bool SIM7080G::GetPINStatus(void) {
    SendCommand("AT+CPIN?\r", rxBufer);
    return strstr(rxBufer, "READY");
}

//

bool SIM7080G::ActivateAppNetwork(void) {
    if (GetAppNetworkStatus()) {
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("DEBUG START: Deactivatenetwork(void)\n\tSIM7080G - APP Network is already active!\nDEBUG END: Deactivatenetwork(void)\n");
#endif
        return false;
    }

    SendCommand("AT+CNACT=0,1\r");

    return GetAppNetworkStatus();  //Not the greatest, will do for now
}

//
bool SIM7080G::DeactivateAppNetwork(void) {
    if (!GetAppNetworkStatus()) {
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("DEBUG START: Deactivatenetwork(void)\n\tSIM7080G - APP Network is already inactive!\nDEBUG END: Deactivatenetwork(void)\n");
#endif
        return false;
    }

    SendCommand("AT+CNACT=0,0\r");

    return GetAppNetworkStatus();  //Same here...
}

//
uint8_t SIM7080G::GetAppNetworkStatus(void) {
    size_t bytesRecv = SendCommand("AT+CNACT?\r", rxBufer);
    uint8_t status = *(strchr(rxBufer, ' ') + 3) - '0';  //Convert a number in char to integer
#if defined SIM7080G_DEBUG || defined SIM7080G_DEBUG_ALL
    uartDebugInterface.printf("DEBUG START: ActivateNetwork(void)\n");
    uartDebugInterface.printf("\tAPP Network 0 Status:\n\tStatus offset: %d\n%s\n", status, strchr(rxBufer, ' ') - rxBufer + 3, rxBufer);
    uartDebugInterface.printf("DEBUG END: ActivateNetwork(void)\n");
#elif defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - APP Network status: %d\n", status);
#endif

    return status;
}

void SIM7080G::GetAppNetworkInfo(SIM7080G_APPN* info) {
    if(info == NULL)
        return;
    SendCommand("AT+CGNACT?\r", rxBufer);
    info->statusx = GetAppNetworkStatus();
    info->pdidx = 0x00;
    char* startPtr = strchr(rxBufer, '\"') + 1;
    size_t charCntr = (strchr(startPtr, '\"') - startPtr - 1);

    //Check against nullptr or wrong IP address length calculation
    if (startPtr == NULL || charCntr < 7 || charCntr > 15) //IP should be at leas 7 or at max 15 characters
        return;

    strncpy(info->ipv4, startPtr, charCntr);
    info->ipv4[charCntr] = '\0';
}

SIM7080G_APPN SIM7080G::GetAppNetworkInfo(void) {
    SIM7080G_APPN info;
    GetAppNetworkInfo(&info);
    return info;
}


//  #
//  #   HTTP(S) applications
//  #

//  #
//  #   GNSS Application
//  #

//
bool SIM7080G::PowerUpGNSS() { return GetGNSSPower() ? true : SendCommand("AT+CGNSPWR=1\r"); }

//
bool SIM7080G::PowerDownGNSS() { return GetGNSSPower() ? SendCommand("AT+CGNSPWR=0\r") : true; }

//
uint8_t SIM7080G::GetGNSSPower() {
    size_t bytesRecv = SendCommand("AT+CGNSPWR?\r", rxBufer);
    uint8_t status = rxBufer[bytesRecv - 5] - '0';
#if defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - GNSS Power status: %d\n", status);
#endif
    return status;
}

//
bool SIM7080G::ColdStartGNSS() { return GetGNSSPower() ? true : SendCommand("AT+CGNSCOLD\r", 2000); }

//
bool SIM7080G::WarmStartGNSS() { return GetGNSSPower() ? true : SendCommand("AT+CGNSWARM\r", 2000); }

//
bool SIM7080G::HotStartGNSS() { return GetGNSSPower() ? true : SendCommand("AT+CGNSHOT\r", 2000); }

//
void SIM7080G::GetGNSS(SIM7080G_GNSS* dst) {

    //Get GNSS info from device
    size_t bytesrecv = SendCommand("AT+CGNSINF\r", rxBufer);

#if defined SIM7080G_VERBOSE
    uartDebugInterface.printf("\tSIM7080G - GNSS update requested: %s\n", rxBufer);
#endif
    
    uint8_t comas = 0;      //Track comas in response text
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

//uint16_t SIM7080G::GetVBat(void) const {}

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