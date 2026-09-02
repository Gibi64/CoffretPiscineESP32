#pragma once
#include <stdint.h>

void modbus_set_bits_from_byte(uint8_t* dest, int idx, const uint8_t value);
void modbus_set_bits_from_bytes(uint8_t* dest,
    int idx,
    unsigned int nb_bits,
    const uint8_t* tab_byte);

uint8_t modbus_get_byte_from_bits(const uint8_t* src, int idx, unsigned int nb_bits);
float modbus_get_float_abcd(const uint16_t* src);
float modbus_get_float_dcba(const uint16_t* src);
float modbus_get_float_badc(const uint16_t* src);
float modbus_get_float_cdab(const uint16_t* src);
/* DEPRECATED - Get a float from 4 bytes in sort of Modbus format */
float modbus_get_float(const uint16_t* src);


void modbus_set_float_abcd(float f, uint16_t* dest);
void modbus_set_float_dcba(float f, uint16_t* dest);
void modbus_set_float_badc(float f, uint16_t* dest);
void modbus_set_float_cdab(float f, uint16_t* dest);

/* DEPRECATED - Set a float to 4 bytes in a sort of Modbus format! */
void modbus_set_float(float f, uint16_t* dest);
