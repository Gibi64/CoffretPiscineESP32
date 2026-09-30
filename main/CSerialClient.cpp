#include "CSerialClient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(TSERIAL_POSIX)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

/* -------------------------------------------------------------------- */
/* Constructeur                                                         */
/* -------------------------------------------------------------------- */
CSerialClient::CSerialClient()
{
    parityMode = spNONE;
    port[0] = 0;
    rate = 0;

#if defined(TSERIAL_WINDOWS)
    serial_handle = INVALID_HANDLE_VALUE;
#elif defined(TSERIAL_POSIX)
    serial_handle = -1;
#elif defined(TSERIAL_ESPIDF)
    serial_handle = UART_NUM_MAX;
#endif
}

/* -------------------------------------------------------------------- */
/* Destructeur & Déconnexion                                            */
/* -------------------------------------------------------------------- */
CSerialClient::~CSerialClient()
{
    disconnect();
}

void CSerialClient::disconnect(void)
{
#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_handle);
        serial_handle = INVALID_HANDLE_VALUE;
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        close(serial_handle);
        serial_handle = -1;
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        uart_driver_delete(serial_handle);
        serial_handle = UART_NUM_MAX;
    }
#endif
}

/* -------------------------------------------------------------------- */
/* Connexion / Ouverture du port                                       */
/* -------------------------------------------------------------------- */
int CSerialClient::connect(char* port_arg, int rate_arg, char parity, int number_of_bytes, int stop_bit)
{
    disconnect();

    if (!port_arg) return 16;
#ifdef _WIN32
    strncpy_s(port, sizeof(port), port_arg, _TRUNCATE);
#else
    strncpy(port, port_arg, sizeof(port) - 1);
#endif
    port[sizeof(port) - 1] = '\0';
    rate = rate_arg;

#if defined(TSERIAL_WINDOWS)
    DCB dcb;
    COMMTIMEOUTS cto = { 0xFFFFFFFF, 5, 10, 5, 10 };

    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate = rate;

    switch (parity) {
    case 'E': dcb.Parity = EVENPARITY; dcb.fParity = 1; break;
    case 'O': dcb.Parity = ODDPARITY;  dcb.fParity = 1; break;
    default:  dcb.Parity = NOPARITY;  dcb.fParity = 0; break;
    }

    dcb.StopBits = (stop_bit == 2) ? TWOSTOPBITS : ONESTOPBIT;
    dcb.ByteSize = (number_of_bytes == 7) ? 7 : 8;

    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fBinary = 1;

    serial_handle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, NULL, NULL);

    if (serial_handle == INVALID_HANDLE_VALUE) return 8;

    if (!SetCommMask(serial_handle, 0) ||
        !SetCommTimeouts(serial_handle, &cto) ||
        !SetCommState(serial_handle, &dcb)) {
        disconnect();
        return 4;
    }

    SetupComm(serial_handle, 4096, 4096);
    return 0;

#elif defined(TSERIAL_POSIX)
    serial_handle = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_handle < 0) return 8;

    fcntl(serial_handle, F_SETFL, 0);

    struct termios options;
    tcgetattr(serial_handle, &options);

    // Baudrate setup
    speed_t speed = B9600;
    switch (rate) {
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    default:     speed = B9600;   break;
    }
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // Parité
    if (parity == 'E') {
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
    }
    else if (parity == 'O') {
        options.c_cflag |= PARENB;
        options.c_cflag |= PARODD;
    }
    else {
        options.c_cflag &= ~PARENB;
    }

    // Stop bits
    if (stop_bit == 2) options.c_cflag |= CSTOPB;
    else               options.c_cflag &= ~CSTOPB;

    // Taille octet
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= (number_of_bytes == 7) ? CS7 : CS8;

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    // Timeouts
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1; // 100 ms timeout

    tcflush(serial_handle, TCIFLUSH);
    if (tcsetattr(serial_handle, TCSANOW, &options) != 0) {
        disconnect();
        return 4;
    }
    return 0;

#elif defined(TSERIAL_ESPIDF)
    int uart_num = atoi(port); // Sous ESP-IDF "0", "1", ou "2"
    serial_handle = (uart_port_t)uart_num;

    uart_config_t uart_config = {};
    uart_config.baud_rate = rate;

    switch (number_of_bytes) {
    case 7:  uart_config.data_bits = UART_DATA_7_BITS; break;
    default: uart_config.data_bits = UART_DATA_8_BITS; break;
    }

    switch (parity) {
    case 'E': uart_config.parity = UART_PARITY_EVEN; break;
    case 'O': uart_config.parity = UART_PARITY_ODD;  break;
    default:  uart_config.parity = UART_PARITY_DISABLE; break;
    }

    uart_config.stop_bits = (stop_bit == 2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    if (uart_param_config(serial_handle, &uart_config) != ESP_OK) return 4;

    // Pins par défaut si non spécifiés autrement
    int tx_pin = (serial_handle == UART_NUM_1) ? 17 : UART_PIN_NO_CHANGE;
    int rx_pin = (serial_handle == UART_NUM_1) ? 16 : UART_PIN_NO_CHANGE;
    uart_set_pin(serial_handle, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    if (uart_driver_install(serial_handle, 2048, 2048, 0, NULL, 0) != ESP_OK) {
        serial_handle = UART_NUM_MAX;
        return 8;
    }
    return 0;
#endif
}

/* -------------------------------------------------------------------- */
/* Émission                                                             */
/* -------------------------------------------------------------------- */
void CSerialClient::sendChar(char data)
{
    sendArray(&data, 1);
}

void CSerialClient::sendArray(char* buffer, int len)
{
    if (!buffer || len <= 0) return;

    PurgeTransmitBuffer();
    PurgeReceiveBuffer();

#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        DWORD result;
        WriteFile(serial_handle, buffer, len, &result, NULL);
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        write(serial_handle, buffer, len);
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        uart_write_bytes(serial_handle, buffer, len);
    }
#endif
}

/* -------------------------------------------------------------------- */
/* Réception                                                            */
/* -------------------------------------------------------------------- */
char CSerialClient::getChar(void)
{
    char c = 0;
    getArray(&c, 1);
    return c;
}

int CSerialClient::getArray(char* buffer, int len)
{
    if (!buffer || len <= 0) return 0;

#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        DWORD read_nbr = 0;
        ReadFile(serial_handle, buffer, len, &read_nbr, NULL);
        return (int)read_nbr;
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        ssize_t read_nbr = read(serial_handle, buffer, len);
        return (read_nbr > 0) ? (int)read_nbr : 0;
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        int read_nbr = uart_read_bytes(serial_handle, (uint8_t*)buffer, len, pdMS_TO_TICKS(100));
        return (read_nbr > 0) ? read_nbr : 0;
    }
#endif

    return 0;
}

/* -------------------------------------------------------------------- */
/* État des tampons                                                     */
/* -------------------------------------------------------------------- */
int CSerialClient::getNbrOfBytes(void)
{
#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        COMSTAT status;
        DWORD etat;
        if (ClearCommError(serial_handle, &etat, &status)) {
            return status.cbInQue;
        }
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        int bytes = 0;
        if (ioctl(serial_handle, FIONREAD, &bytes) >= 0) {
            return bytes;
        }
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        size_t size = 0;
        if (uart_get_buffered_data_len(serial_handle, &size) == ESP_OK) {
            return (int)size;
        }
    }
#endif

    return 0;
}

/* -------------------------------------------------------------------- */
/* Purge                                                                */
/* -------------------------------------------------------------------- */
int CSerialClient::PurgeReceiveBuffer(void)
{
#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        PurgeComm(serial_handle, PURGE_RXCLEAR);
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        tcflush(serial_handle, TCIFLUSH);
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        uart_flush_input(serial_handle);
    }
#endif
    return 0;
}

int CSerialClient::PurgeTransmitBuffer(void)
{
#if defined(TSERIAL_WINDOWS)
    if (serial_handle != INVALID_HANDLE_VALUE) {
        PurgeComm(serial_handle, PURGE_TXCLEAR);
    }
#elif defined(TSERIAL_POSIX)
    if (serial_handle >= 0) {
        tcflush(serial_handle, TCOFLUSH);
    }
#elif defined(TSERIAL_ESPIDF)
    if (serial_handle < UART_NUM_MAX) {
        // En ESP-IDF, il n'y a pas d'API de purge TX explicite : on attend la fin de l'émission
        uart_wait_tx_done(serial_handle, pdMS_TO_TICKS(100));
    }
#endif
    return 0;
}