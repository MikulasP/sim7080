#ifndef SIM7080G_H
#define SIM7080G_H

#include <stdio.h>
#include <Arduino.h>

//#define SIM7080G_DEBUG_ALL      //Debug every function in detail
//#define SIM7080G_DEBUG          //Normal debug messages from most of the functions
#define SIM7080G_VERBOSE        //Send control messages to debug interface

#define SIM7080G_HTTP_REQ_BUFFER    512     //HTTP request configuration buffer size


/**
 *  @brief SIM7080G Power states
*/
enum SIM7080G_PWR {
    SIM_PWDN,       //Power down
    SIM_PWUP,       //Power up
    SIM_SLEEP       //Hardware sleep
};

/**
 *  @brief SIM7080G APP network (mobile internet) info data structure
*/
struct SIM7080G_APPN {
    uint8_t pdidx = 0xFF;
    uint8_t statusx = 0xFF;
    char ipv4[16] = { '\0' };
};

/**
 *  @brief SIM7080G GNSS Data structure
*/
struct SIM7080G_GNSS {
    uint8_t run = 0;            //Run status
    //uint8_t fix;            //Fix status
    char datetime[19] = {'\0'};      //UTC date & time
    char latitude[11] = {'\0'};      //GNSS Latitude
    char longitude[12] = {'\0'};     //GNSS Longitude
    //char mslAltitude[9];    //MSL Altitude
    //char sog[7];            //speed Over Ground
    //char cog[7];            //Course Over Ground
    //uint8_t fixMode;        //Fix mode
    //char hdop[5];           //HDOP
    //char pdop[5];           //PDOP
    //char vdop[5];           //VDOP
    uint8_t gpsSat = 0;         //GPS Satellites in view
    uint8_t gnssSat = 0;        //GNSS Satellites in view
    uint8_t glonassSat = 0;     //GLONASS Satellites in view
    //uint8_t cn0Max;         //C/N0 Max
    //char hpa[7];            //HPA
    //char vpa[7];            //VPA

};

enum SIM7080G_HTTP_METHOD {SIM7080G_HTTP_GET = 1, SIM7080G_HTTP_PUT = 2, SIM7080G_HTTP_POST = 3};

/**
 *  @brief SIM7080G HTTP(S) configuration
*/
struct SIM7080G_HTTPCONF {
    char url[65] = { '\0' };                            //HTTP(S) URL (Max. 64 character supported by the SIM7080G module!)
    uint16_t timeout = 60;                              //HTTP(S) request timeout in seconds 30-1800 (Default is 60)
    uint16_t bodylen = 0;                               //HTTP(S) body length 0-4096
    uint16_t headerlen = 0;                             //HTTP(S) header length 0-350
    SIM7080G_HTTP_METHOD method = SIM7080G_HTTP_POST;   //GET = 1, PUT = 2, POST = 3
};

/**
 *  @brief SIM7080G HTTP(S) header content parameter
*/
struct SIM7080G_HTTP_HEADCONT {
    char* type;
    char* value;
};

/**
 *  @brief SIM7080G HTTP(S) body content parameter
*/
struct SIM7080G_HTTP_BODYCONT {
    char* type;
    char* value;
};

/**
 * 
*/
struct SIM7080G_HTTP_RESULT {
    uint16_t resultCode = 0;
    size_t bytesReceived = 0;
};

class SIM7080G {

    //Serial communication

    uint8_t uartTX = 0;                  //UART TX pin
    uint8_t uartRX = 0;                  //UART RX pin

    uint64_t uartBaudrate = 921600;             //UART Baudrate     (921600)
    HardwareSerial& uartInterface = Serial1;    //UART interface to use

    const static size_t uartMaxRecvSize = 4096; //Max number of bytes to receive (Must be divisible by 4)
    size_t uartRecvtimeout = 100;               //Wait this ammount of ms after last received byte before returning. ( used in Receive() )
                                                //if 0 timeout will be ignored

    uint32_t uartResponseTimeout = 50;    //Time to wait before reading response from device

    char rxBuffer[uartMaxRecvSize];
    //char txBuffer[100];

    //Power control
    int dtrKey = -1;                        //Send module to light sleep (active high) DEPRECATED IN V2!! (no dtr pin in PCB V2)
    uint8_t pwrKey = 0;                    //Power on/off the module

    //
    bool uartOpen = false;                      //UART interface state
    SIM7080G_PWR pwrState = SIM_PWDN;           //Power state

#if defined SIM7080G_DEBUG_ALL || defined SIM7080G_DEBUG || defined SIM7080G_VERBOSE

    //UART debug interface
    //HardwareSerial& uartDebugInterface = Serial;
    HWCDC& uartDebugInterface = Serial;

#endif

public:

    /**
     *  @brief Constructor
    */
    SIM7080G(uint8_t rx, uint8_t tx, uint8_t pwr, int dtr = -1, bool openUART = true);
    //*OK

    //
    //  IO / Power control
    //

    /**
     * 
    */
    void SetDTR(int dtr);
    //

    /**
     *  @brief Power up module with PWR pin
    */
    void PowerUp(void);
    //*OK

    /**
     *  @brief Power down module with PWR pin
    */
    void PowerDown(void);
    //*OK

    /**
     *  @brief Get the module's power state
     * 
     *  @return Power state of the module
    */
    SIM7080G_PWR GetPowerState(void) const;
    //*OK

    /**
     *  @brief Reboot module
    */
    void Reboot(void);
    //*OK

    /**
     *  @brief Put the module to sleep mode with dtr pin
    */
    void EnterSleep(void);
    //*OK

    /**
     *  @brief Wake the module from sleep mode with dtr pin
    */
    void LeaveSleep(void);
    //*OK

    //
    //  UART 
    //

    /**
     *  @brief Open serial interface
    */
    void OpenUART(void);
    //*OK

    /**
     *  @brief Close serial interface
    */
    void CloseUART(void);
    //*OK

    /**
     *  @brief Flush data from UART FIFO
    */
    void FlushUART(void);
    //*OK

    /**
     * @brief Get nuber of bytes in the UART RX FIFO
     * 
     * @return Number of bytes in the RX FIFO
    */
    size_t AvailableUART(void);
    //*OK

    /**
     *  @brief Get serial interface status
     * 
     *  @return true: Serial open | false: Serial closed
    */
    bool GetUART(void) const;
    //*OK

    /**
     *  @brief Get the specified baudrate
     * 
     *  @return Serial baudrate
    */
    uint64_t GetBaudrate(void) const;
    //*OK

    /**
     *  @brief Sent AT command to the module
     * 
     *  @param command      Char array containing the command with null terminator
     *  @param response     Char array to store the response (if nullptr response will be discarded)
     * 
     *  @return Number of bytes received as response (including null terminator)
    */
    size_t SendCommand(const char* command, char* response, uint32_t recvTimeout = 0);
    //*OK

    /**
     *  @brief Send AT command and wait for ERROR or OK
     * 
     *  @param command      Char array containing the command with null terminator
     * 
     *  @return 
    */
    bool SendCommand(const char* command, uint32_t recvTimeout = 0);
    //*OK

    /**
     *  @brief Send len number of bytes to the module
     * 
     *  @param src          Array of data to send
     *  @param len          Number of bytes to send
    */
    void Send(uint8_t* src, size_t len);
    //*OK

    /**
     *  @brief Receive len number of bytes from the module
     * 
     *  @param dst          Array to store the received data
     *  @param len          Number of bytes to receive (If 0 all available data will be read)
     *  
     *  @return Number of bytes actually received
    */
    size_t Receive(uint8_t* dst, size_t len = 0);
    //*OK

    /**
     *  @brief Test UART communication with device.
     * 
     *  @return True: Communcation OK | False: No response from device
    */
    bool TestUART(void);
    //*OK

    /**
     * 
    */
    void SetTAResponseFormat(bool textResponse = false);
    //*OK

    //  #
    //  #   Cellular communication
    //  #

    /**
     *  @brief Get cellular network registration status
     * 
     *  @return Registration status (See SIM7080G AT Command Manual page 63)
    */
    uint8_t GetNetworkReg(void);
    //TODO

    /**
     *  @brief Get cellular signal quality report
     * 
     *  @return RSSI
    */
    uint8_t GetSignalQuality(void);
    //*OK

    /**
     *  @brief List available operators to serial debug interface
     * 
     *  @param debugInterface       
    */
    void GetCellOperators(HardwareSerial& debugInterface);
    //TODO

    /**
     *  @brief List available operators to software serial debug interface
    */
    //void GetCellOperators(SoftwareSerial& debugInterface) const;
    //TODO

    /**
     *  @brief List available operators to software serial debug interface
     * 
     *  @param dst                  Char array to store the results
     * 
     *  @return Number of bytes written to dst
    */
    size_t GetCellOperators(char* dst);
    //TODO

    /**
     *  @brief Select cellular operator to use
    */
    void SetCellOperator(const char* opName);
    //TODO

    /**
     *  @brief Get cellular (phone) functionality
     * 
     *  @return Functionality code (See SIM7080G AT Command Manual page 70)
    */
    uint8_t GetCellFunction(void);
    //TODO

    /**
     *  @brief Set cellular (phone) functionality
    */
    void SetCellFunction(uint8_t functionCode);
    //TODO

    /**
     *  @brief Get chip time
     * 
     *  @param dst          Char array to store time and date (min 21 characters long, including \0)
    */
    void GetTime(char* dst);
    //TODO

    /**
     *  @brief Enter SIM PIN code
     * 
     *  @param pin          SIM PIN code
    */
    bool EnterPIN(const char* pin, bool force = false);
    //*OK

    /**
     *  @brief Get SIM PIN status
    */
    bool GetPINStatus(void);
    //*OK

    /**
     *  @brief Activate APP network
    */
    bool ActivateAppNetwork(void);
    //*OK

    /**
     *  @brief Deactivate APP network
    */
    bool DeactivateAppNetwork(void);
    //*OK

    /**
     *  @brief Get APP network status
    */
    uint8_t GetAppNetworkStatus(void);
    //*OK

    /**
     *  @brief Get App Network details
     * 
     *  @param info Pointer to SIM7080G_APPN struct to store APP network details
    */
    void GetAppNetworkInfo(SIM7080G_APPN* info);
    //*OK

    /**
     *  @brief Get App Network details
     * 
     *  @returns SIM7080G_APPN struct containing APP network details
    */
    SIM7080G_APPN GetAppNetworkInfo(void);
    //*OK


    //  #
    //  #   HTTP(S) applications
    //  #

    /**
     * 
    */
    bool SetHTTPRequest(const SIM7080G_HTTPCONF httpConf, bool build = true);
    //TODO Test

    /**
     * 
    */
    SIM7080G_HTTP_RESULT SendHTTPRequest(const SIM7080G_HTTPCONF httpConf, char* dst = NULL);
    //TODO Test

    /**
     * 
    */
    bool BuildHTTP(void);
    //TODO Test

    /**
     * 
    */
    uint8_t GetHTTPStatus(void);
    //TODO Test

    /**
     * 
    */
    bool ClearHTTPHeader(void);
    //TODO Test

    /**
     * 
    */
    bool AddHTTPHeaderContent(const SIM7080G_HTTP_HEADCONT headerContent);
    //TODO Test

    /**
     * 
    */
    bool SetHTTPBody(size_t length = 0, uint16_t timeout = 10);
    //TODO Test

    /**
     * 
    */
    bool ClearHTTPBody(void);
    //TODO Test

    /**
     * 
    */
    bool AddHTTPBodyContent(const SIM7080G_HTTP_BODYCONT bodyContent);
    //TODO Test


    //  #
    //  #   GNSS 
    //  #

    /**
     *  @brief Power up GNSS
    */
    bool PowerUpGNSS(void);
    //*OK

    /**
     *  @brief Power down GNSS
    */
    bool PowerDownGNSS(void);
    //*OK

    /**
     *  @brief Get GNSS power state
     * 
     *  @returns GNSS power state
    */
    uint8_t GetGNSSPower(void);
    //*OK

    /**
     *  @brief Cold start GNSS
    */
    bool ColdStartGNSS(void);
    //*OK

    /**
     *  @brief Warm start GNSS
    */
    bool WarmStartGNSS(void);
    //*OK

    /**
     *  @brief Hot start GNSS
    */
    bool HotStartGNSS(void);
    //*OK

    /**
     *  @brief Get GNSS Information
     * 
     *  @param dst              Struct to store GNSS info
    */
    void GetGNSS(SIM7080G_GNSS* dst);
    //*OK

    /**
     *  @brief Get GNSS Information
     * 
     *  @return GNSS Info
    */
    SIM7080G_GNSS GetGNSS(void);
    //*OK

    //  #
    //  #   Power Info
    //  #

    /**
     *  @brief Get battery voltage.
     * 
     *  @return Battery voltage in mV.
    */
    uint16_t GetVBat(void);
    //TODO Test

private:

    /**
     *  @brief Power cycle the module with PWRKEY pin
    */
    inline void PowerCycle(void);
    //*OK

    /**
     *  @brief Erase RX Buffer
     * 
     *  @param value            Erase buffer with this value
    */
    void EraseRXBuff(uint32_t value = 0x04040404);

    /**
     * 
    */
    bool AddHTTPContent(const char* type, const char* value, const char* command);

};

#endif  //SIM7080G_H
