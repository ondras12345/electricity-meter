#include "Print_utils.h"

/**
 * Dump data in HEX.
 *
 * @param data data to dump
 * @param len length of data
 * @param offset Offset to be added to printed addresses.
 *               This offset is NOT added to actual data addresses.
 */
void Print_hexdump(Print *response, const uint8_t * data, size_t len, size_t offset)
{
    if (len == 0) return;
    response->print("        ");
    for (size_t i = offset; i < offset+16; i++)
    {
        response->printf(" %x ", i % 16);
    }

    for (size_t i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            response->printf("\r\n%08x", i+offset);
        }
        response->printf(" %02x", data[i]);
        if (i % 16 == 15 || i == len-1)
        {
            for (uint8_t k = 15 - (i % 16); k > 0; k--) response->print("   ");
            response->print("  ");
            for (size_t j = i & ~0x0F; j <= i; j++)
            {
                uint8_t c = data[j];
                if (isprint(c)) response->write(c);
                else response->write('.');
            }
        }
    }
}
