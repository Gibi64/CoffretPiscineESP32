#pragma once
#ifndef CSERIALCLIENT_H
#define CSERIALCLIENT_H

#if defined(_WIN32) || defined(_WINDOWS)
#define TSERIAL_WINDOWS
#include <windows.h>
#elif defined(ESP_PLATFORM)
#define TSERIAL_ESPIDF
#include "driver/uart.h"
#else
#define TSERIAL_POSIX
#include <termios.h>
#endif

enum charParity {
    spNONE,
    spEVEN,
    spODD
};

class CSerialClient {
private:
    charParity parityMode;
    char port[32];
    int rate;

#if defined(TSERIAL_WINDOWS)
    HANDLE serial_handle;
#elif defined(TSERIAL_POSIX)
    int serial_handle;
#elif defined(TSERIAL_ESPIDF)
    uart_port_t serial_handle;
#endif

public:
    CSerialClient();
    ~CSerialClient();

    int connect(char* port_arg, int rate_arg, char parity = 'N', int number_of_bytes = 8, int stop_bit = 1);
    void disconnect(void);

    void sendChar(char data);
    void sendArray(char* buffer, int len);

    char getChar(void);
    int getArray(char* buffer, int len);

    int getNbrOfBytes(void);
    int PurgeReceiveBuffer(void);
    int PurgeTransmitBuffer(void);
};

#endif // CSERIALCLIENT_H
