//Header files
#include "sim7080g.h"

//  "It ain't much but it's honest work"
/**
 *  @brief Convert integer from string to int
 * 
 *  @param number       Start of char array containing the numbers
 *  @param len          Length of the string to process (if 0 whole string will be processed)
 * 
 *  @returns Given number if input is valid or 0 if input invalid
*/
long long int CharToNmbr(char* number, size_t len = 0) {
    if(!number)     //return if nunmber is nullptr
        return 0;
    
    /*
        If no length was given start from the beginning
        and count until a non number character to define length.

        1st character can be '-' to indicate negative value.
    */
    if (number[0] == '-')
        len++;
    if (!len || number[0] == '-') 
        while(*(number + len) != '\0' && (*(number + len) >= '0' && *(number + len) <= '9')) 
            len++;
    
    //Return 0 if the 1st char is - but no number follows it
    if(len == 1 && number[0] == '-')
        return 0;

    long long int val = 0;
    size_t mult = 1;

    //
    for(int i = len - 1; i >= (number[0] == '-' ? 1 : 0); i--) {
        val += (number[i] - '0') * mult;
        mult *= 10;
    }

    if (number[0] == '-')
        val *= -1;

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

//
void SIM7080G::SetDTR(int dtr) {
    //Setup DTR key
    if (dtr >= 0) {
        pinMode(dtrKey, OUTPUT);
        digitalWrite(dtrKey, LOW);
    }
}

//
void SIM7080G::PowerUp() {
#if v
    uartDebugInterface.printf("DEBUG START: PowerUp()\n");
#endif
    //Test if device is already powered up
    if (TestUART()){
#if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tDevice already powered up! Nothing to do here...\nDEBUG END: PowerUp()\n");
#elif SIM7080G_DEBUG_LEVEL == 1
        uartDebugInterface.printf("\tSIM7080G - Device already powered up! Nothing to do here...\n");
#endif
        pwrState = SIM_PWUP;
        return;
    }

    //Power cycle device
    if(pwrState == SIM_PWDN) {
#if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tPowering up device...\nDEBUG END: PowerUp()\n");
#elif SIM7080G_DEBUG_LEVEL == 1
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
    SendCommand("AT+CREBOOT\r");
}

//
void SIM7080G::EnterSleep() {
    if(pwrState == SIM_PWUP && dtrKey >= 0) {
        digitalWrite(dtrKey, HIGH);
        pwrState = SIM_SLEEP;
    }
}

//
void SIM7080G::LeaveSleep() {
    if (pwrState == SIM_SLEEP && dtrKey >= 0) {
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
size_t SIM7080G::SendCommand(const char* command, char* response, uint32_t timeout) {
    if(!command)
        return 0;   //Retur 0 if command is nullptr
    
    //Send command
    uartInterface.print(command);

    //Read data from device
    size_t bytesRecv = 0;

    if(response) {
        //Wait for response
        if (timeout > 0)
            for(size_t i = 0; i < timeout && !uartInterface.available(); i++)
                delay(1);
        else
            delay(uartRecvtimeout);
    
        while(uartInterface.available() && bytesRecv < uartMaxRecvSize)
            response[bytesRecv++] = (char)uartInterface.read();
        response[bytesRecv] = 0;
    }

#if SIM7080G_DEBUG_LEVEL >= 3
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
bool SIM7080G::SendCommand(const char* command, uint32_t timeout) {
    size_t bytesRecv = SendCommand(command, rxBuffer, timeout);

    bool result = bytesRecv >= 2 && rxBuffer[bytesRecv - 2] == '0';

#if SIM7080G_DEBUG_LEVEL >= 3
    //Command debug
    uartDebugInterface.printf("DEBUG START: SendCommand(char*)\n");
    if(bytesRecv >= 2)
        uartDebugInterface.printf("\tCommand result: %c - %s\n", rxBuffer[bytesRecv - 2], (result ? "SUCCESSFUL" : "FAILED"));
    else
        uartDebugInterface.printf("\tNot enough bytes received! Bytes received: %d\n", bytesRecv);
    uartDebugInterface.printf("DEBUG END: SendCommand(char*)\n");
#elif SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - Command: %s | Result: %s\n", command, (result ? "SUCCESSFUL" : "FAILED"));
#endif

    return result;
}

//
void SIM7080G::Send(uint8_t* src, size_t len) {
    uartInterface.write(src, len);
}

//
size_t SIM7080G::Receive(uint8_t* dst, size_t len, uint32_t timeout) {
    size_t bytesRecv = 0;

    if (timeout > 0)
        for(size_t i = 0; i < timeout && !uartInterface.available(); i++)
            delay(1);

    for (;uartInterface.available() && (len ? bytesRecv < len : true);) 
        dst[bytesRecv++] = (uint8_t)uartInterface.read();
    
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

//
void SIM7080G::SetTAResponseFormat(bool textResponse) {
    SendCommand(textResponse ? (char*)"ATV1\r" : (char*)"ATV0\r");
}

//
bool SIM7080G::SetEcho(bool echo) {
    return SendCommand(echo ? "ATE1\r" : "ATE0\r");
}

//  #
//  #   Cellular communication
//  #

//
uint8_t SIM7080G::GetNetworkReg(void) {
    size_t bytesRecv = SendCommand("AT+CREG?\r", rxBuffer);
    char* startPtr = strchr(rxBuffer, ',');
    if (!startPtr)
        return 255;
    uint8_t status = *(startPtr + 1) - '0';
#if SIM7080G_DEBUG_LEVEL >= 1
    uartDebugInterface.printf("\tSIM7080G - Network Registration status: %d\n", status);
#endif
    return status;
}

//
uint8_t SIM7080G::GetSignalQuality() {
    
    size_t bytesRecv = SendCommand("AT+CSQ\r", rxBuffer);

    if(bytesRecv == 0)
        return 255;

    char* startPtr = rxBuffer;
    while ((*startPtr) != ':' && (*startPtr) != '\0')
        startPtr++;
    startPtr++;     //Step forward another

    uint8_t offset = 0;
    for (; startPtr[offset] != ',' && startPtr[offset] != '\0'; offset++);

    //Check against mem overflow
    return bytesRecv <= uartMaxRecvSize && startPtr + 2 <= rxBuffer + uartMaxRecvSize ? CharToNmbr(startPtr, offset) : SIM7080_SIGNAL_QUALITY_UNKNOWN;
}


//void SIM7080G::GetCellOperators(HardwareSerial& debugInterface);


//size_t SIM7080G::GetCellOperators(char* dst);


//void SIM7080G::SetCellOperator(char* opName);


//uint8_t SIM7080G::GetCellFunction(void);


//void SIM7080G::SetCellFunction(uint8_t functionCode);


//void SIM7080G::GetTime(char* dst);

//
bool SIM7080G::EnterPIN(const char* pin, bool force) {
#if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("DEBUG START: EnterPin(%s)\n", pin);
#endif
    
    //SIM PIN is already entered
    if (GetPINStatus()) {
#if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM PIN READY\nDEBUG END: EnterPin(%s)\n", pin);
#elif defined SIM7080G_DEBUG_LEVEL == 1
        uartDebugInterface.printf("\tSIM7080G - SIM PIN is already entered!\n");
#endif
        return true;
    }

    if (strlen(pin) == 4)
    {
#if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tPin format OK!\nDEBUG END: EnterPin(%s)\n", pin);
#endif
        char tmpBuff[14] = {'A', 'T', '+', 'C', 'P', 'I', 'N', '=', '*', '*', '*', '*', '\r', '\0'};
        memcpy(tmpBuff + 8, pin, 4);
        return SendCommand(tmpBuff);
    }
#if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tPin format ERROR!\nDEBUG END: EnterPin(%s)\n", pin);
#elif SIM7080G_DEBUG_LEVEL == 1
    uartDebugInterface.printf("\tSIM7080G - SIM PIN format ERROR!");
#endif
    return false;
}

//
bool SIM7080G::GetPINStatus(void) {
    SendCommand("AT+CPIN?\r", rxBuffer);
    return strstr(rxBuffer, "READY");
}

//
bool SIM7080G::ActivateAppNetwork(void) {
    if (GetAppNetworkStatus()) {
#if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("DEBUG START: ActivateAppNetwork(void)\n\tSIM7080G - APP Network is already active!\nDEBUG END: ActivateAppNetwork(void)\n");
#endif
        return false;
    }

    SendCommand("AT+CNACT=0,1\r");

    return GetAppNetworkStatus();  //Not the greatest, will do for now
}

//
bool SIM7080G::DeactivateAppNetwork(void) {
    if (!GetAppNetworkStatus()) {
#if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("DEBUG START: DeactivateAppNetwork(void)\n\tSIM7080G - APP Network is already inactive!\nDEBUG END: DeactivateAppNetwork(void)\n");
#endif
        return false;
    }

    SendCommand("AT+CNACT=0,0\r");

    return GetAppNetworkStatus();  //Same here...
}

//
uint8_t SIM7080G::GetAppNetworkStatus(void) {
    if(!SendCommand("AT+CNACT?\r", rxBuffer, 250)) {
        #if SIM7080G_DEBUG_LEVEL >= 1
        uartDebugInterface.printf("\tSIM70800G - GetAppNetworkStatus(void) No response from device!\n");
        #endif
        return SIM7080_INVALID_RETURN_VALUE;
    }
    char* startPtr = strchr(rxBuffer, ',');
    
    //Return if startPtr is null
    if(!startPtr)
        return SIM7080_INVALID_RETURN_VALUE;
    
    uint8_t status = CharToNmbr((startPtr + 1), 1);
    #if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("DEBUG START: GetAppNetworkStatus(void)\n");
    uartDebugInterface.printf("\tAPP Network 0 Status:%d\n", status);
    uartDebugInterface.printf("DEBUG END: GetAppNetworkStatus(void)\n");
    #elif SIM7080G_DEBUG_LEVEL == 1
    uartDebugInterface.printf("\tSIM7080G - APP Network status: %d\n", status);
#endif

    return status;
}

//
void SIM7080G::GetAppNetworkInfo(SIM7080G_APPN* info) {
    if(info == NULL)
        return;
    SendCommand("AT+CGNACT?\r", rxBuffer);
    info->statusx = GetAppNetworkStatus();
    info->pdidx = 0x00;
    char* startPtr = strchr(rxBuffer, '\"') + 1;
    size_t charCntr = (strchr(startPtr, '\"') - startPtr - 1);

    //Check against nullptr or wrong IP address length calculation
    if (startPtr == NULL || charCntr < 7 || charCntr > 15) //IP should be at leas 7 or at max 15 characters
        return;

    strncpy(info->ipv4, startPtr, charCntr);
    info->ipv4[charCntr] = '\0';
}

//
SIM7080G_APPN SIM7080G::GetAppNetworkInfo(void) {
    SIM7080G_APPN info;
    GetAppNetworkInfo(&info);
    return info;
}


//  #
//  #   IP applications
//  #

//
int SIM7080G::Ping4(const char* address, uint16_t pingCount, uint16_t packetSize, uint32_t timeout) {
    if (!address || !pingCount || !packetSize || !timeout)
        return SIM7080_INVALID_PARAMETER;       //Wrong parameters

    if (strlen(address) < 7 || strlen(address) > 15)
        return SIM7080_INVALID_PARAMETER;       //Bad IP address length

    if (!GetAppNetworkStatus())
        return SIM7080_INVALID_PARAMETER;       //APP network inactive

    //Check parameter values
    if(pingCount > 500)
        pingCount = 500;

    if (packetSize > 1400)
        packetSize = 1400;

    if (timeout > 60000)
        timeout = 60000;

    #if SIM7080G_DEBUG_LEVEL >= 1
    uartDebugInterface.printf("\tSIM7080G - Pinging %s with %u bytes of data...\n", address, packetSize);
    #endif

    char buffer[32 + strlen(address)] = { '\0' };
    sprintf(buffer, "AT+SNPING4=\"%s\",%u,%u,%u\r", address, pingCount, packetSize, timeout);
    SendCommand(buffer);

    uint16_t successful = 0;    //Successful ping count
    size_t bytesRecv = 0;       //Bytes received from module

    for(uint16_t i = 0; i < pingCount; i++){
        bytesRecv = Receive((uint8_t*)rxBuffer, 0, timeout + 100);
        
        #if SIM7080G_DEBUG_LEVEL >= 1
        uartDebugInterface.printf("\tSIM7080G -Ping 4: RX buffer: %s\n", rxBuffer);
        #endif

        //Continue loop if no reply sent
        if (!bytesRecv) {
            #if SIM7080G_DEBUG_LEVEL >= 1
            uartDebugInterface.printf("\tSIM7080G -Ping 4: No reply from module!\n");
            #endif
            continue;
        }

        char* startPtr = strrchr(rxBuffer, ',');
        
        //Continue loop if startPtr comes back as nullptr
        if (!startPtr) {
            #if SIM7080G_DEBUG_LEVEL >= 1
            uartDebugInterface.printf("\tSIM7080G -Ping 4: Wrong reply format!\n");
            #endif
            continue;
        }

        uint32_t roundTime = CharToNmbr(startPtr + 1);

        if (roundTime < timeout) {
            successful++;
            #if SIM7080G_DEBUG_LEVEL >= 1
            uartDebugInterface.printf("\tSIM7080G - Got ping reply! RTT: %u\n", roundTime);
            #endif
        }
        #if SIM7080G_DEBUG_LEVEL >= 1
        else
            uartDebugInterface.printf("\tSIM7080G - Ping timed out!\n");
        #endif
    }

    #if SIM7080G_DEBUG_LEVEL >= 1
    uartDebugInterface.printf("\tSIM7080G - Ping replies received: %u out of %u\n", successful, pingCount);
    #endif

    return successful;
}


//  #
//  #   HTTP(S) applications
//  #

//
bool SIM7080G::SetHTTPRequest(const SIM7080G_HTTPCONF httpConf, bool build) {
    char buffer[SIM7080G_HTTP_REQ_BUFFER] = { '\0' };   //Temporary buffer for configuration

    sprintf(buffer, "AT+SHCONF=\"URL\",\"%s\"\r", httpConf.url);
    if (!SendCommand(buffer))
        return false;
    buffer[0] = 0;

    sprintf(buffer, "AT+SHCONF=\"BODYLEN\",%u\r", httpConf.bodylen);
    if (!SendCommand(buffer))
        return false;
    buffer[0] = 0;

    sprintf(buffer, "AT+SHCONF=\"HEADERLEN\",%u\r", httpConf.headerlen);
    if (!SendCommand(buffer))
        return false;

    if (build)
        BuildHTTP();
    
    return true;
}

//
SIM7080G_HTTP_RESULT SIM7080G::SendHTTPRequest(const SIM7080G_HTTPCONF httpConf, char* dst) {
    char buffer[SIM7080G_HTTP_REQ_BUFFER] = { '\0' };

    if (dst == NULL)
        dst = rxBuffer;

    sprintf(buffer, "AT+SHREQ=\"%s\",%u\r", httpConf.url, httpConf.method);
    SendCommand(buffer, dst);

    SIM7080G_HTTP_RESULT httpResult;

    char* startPtr = strchr(dst, ',') + 1;
    httpResult.resultCode = CharToNmbr(startPtr, 3);
    httpResult.bytesReceived = CharToNmbr(strchr(startPtr, ',') + 1);

    return httpResult;
}

//
bool SIM7080G::BuildHTTP(void) {
    return SendCommand("AT+SHCONN\r");
}

//
uint8_t SIM7080G::GetHTTPStatus(void) {
    SendCommand("AT+SHSTATE?\r", rxBuffer);
    uint8_t httpStatus = *(strchr(rxBuffer, ' ') + 1) - '0';
#if SIM7080G_DEBUG_LEVEL >=1
    uartDebugInterface.printf("\tSIM7080G - Get HTTP status: %u\n", httpStatus);
#endif
    return httpStatus;
}

//
bool SIM7080G::ClearHTTPHeader(void) {
    return SendCommand("AT+SHCHEAD\r");
}

//
bool SIM7080G::AddHTTPHeaderContent(const SIM7080G_HTTP_HEADCONT headerContent) {
    return AddHTTPContent(headerContent.type, headerContent.value, "AT+SHAHEAD");
}

//
bool SIM7080G::SetHTTPBody(size_t length, uint16_t timeout) {
    char buffer[SIM7080G_HTTP_REQ_BUFFER] = { '\0' };

    sprintf(buffer, "AT+SHBOD=%u,%u\r", length, timeout);

    return SendCommand(buffer);
}

//
bool SIM7080G::ClearHTTPBody(void) {
    return SendCommand("AT+SHCPARA\r");
}

//
bool SIM7080G::AddHTTPBodyContent(const SIM7080G_HTTP_BODYCONT bodyContent) {
    return AddHTTPContent(bodyContent.type, bodyContent.value, "AT+SHPARA");
}


//  #
//  #   File Transfer Protocol (FTP)
//  #

//
bool SIM7080G::SetFTPPort(uint16_t port) {
    char buffer[32] = { '\0' };
    sprintf(buffer, "AT+FTPPORT=%u\r", port);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPMode(SIM7080G_FTP_MODE mode) {
    char buffer[16] = { '\0' };
    sprintf(buffer, "AT+FTPMODE=%u\r", mode);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPDataType(SIM7080G_FTP_DTYPE type) {
    char buffer[16] = { '\0' };
    sprintf(buffer, "AT+FTPTYPE=%u\r", type);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPCID(uint8_t pdpidx) {
    if (pdpidx > 4)
        return false;
    
    char buffer[16] = { '\0' };   //AT+FTPCID=
    sprintf(buffer, "AT+FTPCID=%u\r", pdpidx);
    return SendCommand(buffer);
}

//
//bool SIM7080G::SetFTPPutType(const char* type) {}

//
bool SIM7080G::SetFTPServer(const char* ip) {
    if(ip == NULL || strlen(ip) < 7 || strlen(ip) > 15)
        return false;
    char buffer[64] = { '\0' };
    sprintf(buffer, "AT+FTPSERV=\"%s\"\r", ip);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPUsername(const char* username) {
    if (username == NULL)
        return SendCommand("AT+FTPUN=\"\"\r");
    
    char buffer [16 + strlen(username)] = { '\0' };
    sprintf(buffer , "AT+FTPUN=\"%s\"\r", username);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPPassword(const char* password) {
    if (password == NULL)
        return SendCommand("AT+FTPPW=\"\"\r");
    
    char buffer [16 + strlen(password)] = { '\0' };
    sprintf(buffer , "AT+FTPPW=\"%s\"\r", password);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPDownFN(const char* filename) {
    if (filename == NULL)
        return false;
    
    char buffer[32 + strlen(filename)] = { '\0' };
    sprintf(buffer, "AT+FTPGETNAME=\"%s\"\r", filename);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPDownFP(const char* filePath) {
    if (filePath == NULL)
        return false;
    
    char buffer[32 + strlen(filePath)] = { '\0' };
    sprintf(buffer, "AT+FTPGETPATH=\"%s\"\r", filePath);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPUpFN(const char* filename) {
    if (filename == NULL)
        return false;
    
    char buffer[32 + strlen(filename)] = { '\0' };
    sprintf(buffer, "AT+FTPPUTNAME=\"%s\"\r", filename);
    return SendCommand(buffer);
}

//
bool SIM7080G::SetFTPUpFP(const char* filePath) {
    if (filePath == NULL)
        return false;
    
    char buffer[32 + strlen(filePath)] = { '\0' };
    sprintf(buffer, "AT+FTPPUTPATH=\"%s\"\r", filePath);
    return SendCommand(buffer);
}

//
SIM7080G_FTP_RESULT SIM7080G::FTPUpload(uint8_t* src, size_t length) {
    //Test given parameters
    if(!src || !length)
        return SIM_FTP_PAR_ERR;

    //If in another FTP session close it
    if(GetFTPState())
        CloseFTPSession();
    
    //Initiate FTP connection
    char buffer[128] = { '\0' };

    //Initiate the connection
    SendCommand("AT+FTPPUT=1\r");

    //Check if server responded
    if(!Receive((uint8_t*)buffer, 0, 78000))
        return SIM_FTP_TIMEOUT;

    //Process PUT response
    char* startPtr = strchr(buffer, ',') + 1;                                           //Get result code
    uint8_t responseCode = CharToNmbr(startPtr, strchr(startPtr, ',') - startPtr);      //...

    #if SIM7080G_DEBUG_LEVEL >= 2
     uartDebugInterface.printf("\tSIM7080G - FTP Upload: Init put response code %d, RX buffer: %s\n", responseCode, buffer);
    #endif

    //Return if connection unsuccessful
    if (responseCode > 1 && responseCode < 100)
        return (SIM7080G_FTP_RESULT)responseCode;

    //Received max length at once
    size_t chunkLength = CharToNmbr(strchr(startPtr, ',') + 1);
    size_t dataLength = length;
    size_t dataSent = 0;

    #if SIM7080G_DEBUG_LEVEL == 1
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: Uploading %u bytes of data...\n", length);
    #elif SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: ChunkLen: %u, Data len: %u\n", chunkLength, dataLength);
    #endif

    //Send data in segments if larger than maximum chunk size
    while(dataLength > chunkLength) {
        #if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Uploading chunk!\n");
        #endif

        //Initiate data transaction
        sprintf(buffer, "AT+FTPPUT=2,%u\r", chunkLength);
        if (!SendCommand(buffer, rxBuffer, 75000)) {
            CloseFTPSession();
            return SIM_FTP_TIMEOUT; //Connection timed out
        }

        //Check 
        if(CharToNmbr(strchr(rxBuffer, ',') + 1) != (length > chunkLength ? chunkLength : length))  {
            #if SIM7080G_DEBUG_LEVEL >= 2
            uartDebugInterface.printf("\tSIM7080G - FTP Upload: Requested and provided byte size mismatched! Provided: %u, requested: %u\n", chunkLength, CharToNmbr(strchr(rxBuffer, ',') + 1));
            #endif
            CloseFTPSession();
            return SIM_FTP_OTH_ERR;
        }

        //Send data to the server and update trackers
        Send(src + dataSent, chunkLength);
        dataSent += chunkLength;
        dataLength -= chunkLength;
        
        //Wait for confirmation
        if(!Receive((uint8_t*)rxBuffer, 0, 75000)) {
            CloseFTPSession();
            return SIM_FTP_TIMEOUT;
        }
        
        startPtr = strchr(rxBuffer, ',') + 1;
        responseCode = CharToNmbr(startPtr);

        #if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Status code: %u\n", rxBuffer[0] - '0');
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Put response code %d, bytes sent: %u\n", responseCode, dataSent);
        #endif
        #if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: RX buffer: \n%s\n", rxBuffer);
        #endif

        //Return if connection unsuccessful
        if (responseCode > 1 && responseCode < 100)
            return (SIM7080G_FTP_RESULT)responseCode;

        if(chunkLength != CharToNmbr(strchr(startPtr, ',') + 1)) {
            chunkLength = CharToNmbr(strchr(startPtr, ',') + 1);
            #if SIM7080G_DEBUG_LEVEL >= 2
            uartDebugInterface.printf("\tSIM7080G - FTP Upload: Data chunk length changed: %u\n", chunkLength);
            #endif
        }
    } // while(dataLength > chunkLength)

    #if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: Uploading last chunk!\n");
    #endif

    sprintf(buffer, "AT+FTPPUT=2,%u\r", dataLength);
    SendCommand(buffer, rxBuffer, 75000);

    if(CharToNmbr(strchr(rxBuffer, ',') + 1) != dataLength)  {
        #if SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Requested and provided byte size not matched! Req: %u, prov: %u\n", CharToNmbr(strchr(rxBuffer, ',') + 1), dataLength);
        #endif
    }

    #if SIM7080G_DEBUG_LEVEL >= 3
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: RX buffer: \n%s\n", rxBuffer);
    #endif

    //Send data to the server and update trackers
    Send(src + dataSent, dataLength);

    //Wait for confirmation
    if(!Receive((uint8_t*)rxBuffer, 0, 75000)) {
        CloseFTPSession();
        return SIM_FTP_TIMEOUT;
    }

    startPtr = strchr(rxBuffer, ',') + 1;
    responseCode = CharToNmbr(startPtr);

    #if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: Status code: %u\n", rxBuffer[0]);
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: Put response code %d, bytes sent: %u\n", responseCode, dataSent);
    #endif
    #if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: RX buffer: \n%s\n", rxBuffer);
    #endif

    //Return if connection unsuccessful
        if (responseCode > 1 && responseCode < 100)
            return (SIM7080G_FTP_RESULT)responseCode;
    
    //End FTP transaction
    SendCommand("AT+FTPPUT=2,0\r");

    //Wait for confirmation
    if(!Receive((uint8_t*)rxBuffer, 0, 75000)) {
        CloseFTPSession();
        return SIM_FTP_TIMEOUT;
    }

    //Get response code from received data
    responseCode = CharToNmbr(startPtr);

    #if SIM7080G_DEBUG_LEVEL >= 2
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: Put response code: %u\n", responseCode);
    #endif
    #if SIM7080G_DEBUG_LEVEL >= 3
    uartDebugInterface.printf("\tSIM7080G - FTP Upload: RX buffer: \n%s\n", rxBuffer);
    #endif

    //Return if connection unsuccessful
        if (responseCode > 0)
            return (SIM7080G_FTP_RESULT)responseCode;

    startPtr = strchr(rxBuffer, ' ') + 1;
    if (*startPtr == '1' &&  CharToNmbr(strchr(startPtr, ',') + 1) == 0) {
        #if SIM7080G_DEBUG_LEVEL == 1
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Successful!\n", length);
        #elif SIM7080G_DEBUG_LEVEL >= 2
        uartDebugInterface.printf("\tSIM7080G - FTP Upload: Session successful!\n");
        #endif
        return SIM_FTP_SUCCESS;
    }

    //Upload was not confirmed to be successful
    return SIM_FTP_UPL_ERR;
}

//Will be implemented later
//SIM7080G_FTP_RESULT SIM7080G::FTPDownload(uint8_t* dst, size_t* bytesReceived) {}

//Will be implemented later
//SIM7080G_FTP_RESULT SIM7080G::DeleteFTPFile(void) {}//

//Will be implemented later
//size_t SIM7080G::GetFTPFileSize(void) {}

//
uint8_t SIM7080G::GetFTPState(void) {
    SendCommand("AT+FTPSTATE\r", rxBuffer);
    char* startPtr = strchr(rxBuffer, ' ');
    if(!strchr(rxBuffer, ' '))
        return 0;
    return *(startPtr + 1) - '0';
}

//
//bool SIM7080G::MkFTPDir(void) {}

//
//bool SIM7080G::RmFTPDir(void) {}

//
void SIM7080G::CloseFTPSession() {
    SendCommand("AT+FTPQUIT\r");
}

//  #
//  #   GNSS Application
//  #

//
bool SIM7080G::PowerUpGNSS() { return GetGNSSPower() ? true : SendCommand("AT+CGNSPWR=1\r"); }

//
bool SIM7080G::PowerDownGNSS() { return GetGNSSPower() ? SendCommand("AT+CGNSPWR=0\r") : true; }

//
uint8_t SIM7080G::GetGNSSPower() {
    size_t bytesRecv = SendCommand("AT+CGNSPWR?\r", rxBuffer);
    uint8_t status = rxBuffer[bytesRecv - 5] - '0';
#if SIM7080G_DEBUG_LEVEL >= 1
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
    size_t bytesrecv = SendCommand("AT+CGNSINF\r", rxBuffer);

#if SIM7080G_DEBUG_LEVEL >= 1
    uartDebugInterface.printf("\tSIM7080G - GNSS update requested: %s\n", rxBuffer);
#endif
    
    uint8_t comas = 0;      //Track comas in response text
    uint8_t infCntr = 0;    //For indexing SIM7080G_GNSS arrays

    for (uint8_t i = 0; i < bytesrecv; i++) {
        if(rxBuffer[i] == ','){
            comas++;
            infCntr = 0;
            continue;
        }

        //
        switch (comas)
        {
        case 0:     //GNSS Run status
            dst->run = rxBuffer[i] - '0';
            break;

        case 2:     //UTC date & time
            dst->datetime[infCntr++] = rxBuffer[i];
            break;

        case 3:     //Latitude
            dst->latitude[infCntr++] = rxBuffer[i];
            break;

        case 4:     //Longitude
            dst->longitude[infCntr++] = rxBuffer[i];
            break;
        
        case 14:    //GPS Satellites in view
            dst->gpsSat = (rxBuffer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
            break;

        case 15:    //GNSS Satellites in view
            dst->gnssSat = (rxBuffer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
            break;

        case 16:    //GNSS Satellites in view
            dst->glonassSat = (rxBuffer[i] - '0') + (infCntr++ ? 1 : 0) * 10;
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

//
bool SIM7080G::GetGNSSLock(void) {
    SIM7080G_GNSS gnssInfo;
    GetGNSS(&gnssInfo);
    return gnssInfo.datetime[0] != 0 && (gnssInfo.latitude[0] == '\0' || strcmp(gnssInfo.latitude, "0.000000"));
}


//  #
//  #   Power
//  #

uint16_t SIM7080G::GetVBat(void) {
    SendCommand("AT+CBC\r", rxBuffer);
    uint16_t vBat = CharToNmbr(strchr(strchr(rxBuffer, ',') + 1, ',') + 1);
#if SIM7080G_DEBUG_LEVEL >= 1
    uartDebugInterface.printf("\tSIM7080G - Battery voltage: %u\n", vBat);
#endif
    return vBat;
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

//
void SIM7080G::EraseRXBuff(uint32_t value) {
    size_t rounds = this->uartMaxRecvSize / 4;
    //Erase buffer with 32 bit numbers for efficiency
    for(size_t i = 0; i < rounds; i+=4)
        rxBuffer[i] = value;
}

//
bool SIM7080G::AddHTTPContent(const char* type, const char* value, const char* command) {
    if (type == NULL || value == NULL || command == NULL)
        return false;
    
    char buffer[SIM7080G_HTTP_REQ_BUFFER] = { '\0' };

    sprintf(buffer, "%s=\"%s\",\"%s\"\r", command, type, value);
    
    return SendCommand(buffer);
}




