#ifndef SIM7080G_H
#define SIM7080G_H

#include <Arduino.h>

//#define ARDUINO_DUE_TEST        //Will delete soon...

#define SIM7080G_VERBOSE        //Send debug messages to a secondary serial interface

//Module power states
enum SIM7080G_PWR {
    SIM_PWDN,       //Power down
    SIM_PWUP,       //Power up
    SIM_SLEEP       //Hardware sleep
};

struct SIM7080G_GNSS {
    uint8_t run;            //Run status
    //uint8_t fix;            //Fix status
    char datetime[19];      //UTC date & time
    char latitude[11];      //Latitude
    char longitude[12];     //Longitude
    //char mslAltitude[9];    //MSL Altitude
    //char sog[7];            //speed Over Ground
    //char cog[7];            //Course Over Ground
    //uint8_t fixMode;        //Fix mode
    //Reserved 1 field
    //char hdop[5];           //HDOP
    //char pdop[5];           //PDOP
    //char vdop[5];           //VDOP
    uint8_t gpsSat;         //GPS Satellites in view
    uint8_t gnssSat;        //GNSS Satellites in view
    uint8_t glonassSat;     //GLONASS Satellites in view
    //uint8_t cn0Max;         //C/N0 Max
    //char hpa[7];            //HPA
    //char vpa[7];            //VPA

};

class SIM7080G {

    //Serial communication
#ifndef ARDUINO_DUE_TEST
    const uint8_t uartTX = 47;                  //UART TX pin
    const uint8_t uartRX = 48;                  //UART RX pin
#endif
    const uint64_t uartBaudrate = 115200;       //UART Baudrate     (921600)
#ifndef ARDUINO_DUE_TEST
    HardwareSerial& uartInterface = Serial1;    //UART interface to use
#else
    HardwareSerial& uartInterface = Serial3;
#endif
    const static size_t uartMaxRecvSize = 1000; //Max number of bytes to receive (to prevent buffer overflow)
    //size_t uartRecvtimeout = 5000;            //Wait this ammount of ms after last received byte before returning. ( used in Receive() )
                                                //if 0 timeout will be ignored

    char rxBufer[uartMaxRecvSize];

    //Power control
#ifndef ARDUINO_DUE_TEST
    const uint8_t dtrKey = 14;                  //Send module to light sleep (active high)
    const uint8_t pwrKey = 21;                  //Power on/off the module
#else
    const uint8_t dtrKey = 2;                  //Send module to light sleep (active high)
    const uint8_t pwrKey = 3;                  //Power on/off the module
#endif
    //
    bool uartOpen = false;                      //UART interface state
    SIM7080G_PWR pwrState = SIM_PWDN;           //Power state

public:

    /**
     *  @brief Constructor
    */
    SIM7080G(void);
    //*OK

    //
    //  IO / Power control
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
    size_t SendCommand(char* command, char* response);
    //*OK

    /**
     *  @brief Send AT command and wait for ERROR or OK
     * 
     *  @param command      Char array containing the command with null terminator
     * 
     *  @return First character of response
    */
    char SendCommand(char* command);

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

    //  #
    //  #   Cellular communication
    //  #

    /**
     *  @brief Get cellular network registration status
     * 
     *  @return MSB: NRURC | LSB: Registration status (See SIM7080G AT Command Manual page 63)
    */
    uint16_t GetNetworkReg(void) const;
    //TODO

    /**
     *  @brief Get cellular signal quality report
     * 
     *  @return First 4 bit: rssi | last 4 bit: ber
    */
    uint8_t GetSignalQuality(void) const;
    //TODO

    /**
     *  @brief List available operators to serial debug interface
     * 
     *  @param debugInterface       
    */
    void GetCellOperators(HardwareSerial& debugInterface) const;
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
    size_t GetCellOperators(char* dst) const;
    //TODO

    /**
     *  @brief Select cellular operator to use
    */
    void SetCellOperator(char* opName);
    //TODO

    /**
     *  @brief Get cellular (phone) functionality
     * 
     *  @return Functionality code (See SIM7080G AT Command Manual page 70)
    */
    uint8_t GetCellFunction(void) const;
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
    void GetTime(char* dst) const;
    //TODO

    /**
     *  @brief Enter SIM PIN code
     * 
     *  @param pin          SIM PIN code
    */
    void EnterPIN(uint16_t pin);
    //TODO

    /**
     *  @brief Get SIM PIN status
    */
    bool GetPINStatus(void);
    //TODO

    /**
     *  @brief Activate APP network
    */
    void ActivateNetwork(void);
    //TODO

    /**
     *  @brief Deactivate APP network
    */
    void DeactivateNetwork(void);
    //TODO

    /**
     *  @brief Get APP network status
    */
    bool GetNetworkStatus(void) const;
    //TODO


    //  #
    //  #   HTTP(S) applications
    //  #



    //  #
    //  #   GNSS 
    //  #

    /**
     *  @brief Power up GNSS
    */
    uint8_t PowerUpGNSS(void);
    //TODO test

    /**
     *  @brief Power down GNSS
    */
    uint8_t PowerDownGNSS(void);
    //TODO test

    /**
     *  @brief Cold start GNSS
    */
    uint8_t ColdStartGNSS(void);
    //TODO test

    /**
     *  @brief Warm start GNSS
    */
    uint8_t WarmStartGNSS(void);
    //TODO test

    /**
     *  @brief Hot start GNSS
    */
    uint8_t HotStartGNSS(void);
    //TODO test

    /**
     *  @brief Get GNSS Information
     * 
     *  @param dst              Struct to store GNSS info
    */
    void GetGNSS(SIM7080G_GNSS* dst);
    //TODO test

    /**
     *  @brief Get GNSS Information
     * 
     *  @return GNSS Info
    */
    SIM7080G_GNSS GetGNSS(void);
    //TODO test

    //  #
    //  #   Power Info
    //  #

    /**
     *  @brief Get battery voltage.
     * 
     *  @return Battery voltage in mV.
    */
    uint16_t GetVBat(void) const;
    //TODO

    /**
     *  @brief Get battery charge percentage.
     * 
     *  @returns 0-100 battery charge percentage.
    */
    uint8_t GetPBat(void) const;
    //TODO

    

private:

    /**
     *  @brief Power cycle the module with PWRKEY pin
    */
    inline void PowerCycle(void);
    //*OK

};

#endif  //SIM7080G_H
