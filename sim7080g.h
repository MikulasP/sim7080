#ifndef SIM7080G_H
#define SIM7080G_H

#include <Arduino.h>

enum SIM7080G_PWR { SIM_PWDN, SIM_PWUP, SIM_SLEEP };

class SIM7080G {

    //Serial communication
    const uint8_t uartTX = 47;              //UARt TX pin
    const uint8_t uartRX = 48;              //UART RX pin
    const uint32_t uartBaudrate = 921600;   //UART Baudrate
    HardwareSerial uartInterface = Serial2;   //UART interface to use

    size_t uartMaxRecvSize = 2000;          //Max number of bytes to receive (to prevent buffer overflow)
    //size_t uartRecvtimeout = 5000;          //Wait this ammount of ms after last received byte before returning. ( used in Receive() )
                                            //if 0 timeout will be ignored

    //Power control
    const uint8_t dtrKey = 14;              //Send module to light sleep (active high)
    const uint8_t pwrKey = 21;              //Power on/off the module

    //
    bool uartOpen = false;                  //UART interface state
    SIM7080G_PWR pwrState = SIM_PWDN;        //Power state

public:
    
    //
    //  IO / Power control
    //

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
     *  @return ...
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
     * 
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

private:

    /**
     *  @brief Power cycle the module with PWRKEY pin
    */
    inline void PowerCycle(void);
    //*OK

};

#endif  //SIM7080G_H