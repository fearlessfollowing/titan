#ifndef _SPS_PARSER_H_
#define _SPS_PARSER_H_

#include <stdint.h>

class sps_parser
{
public:
    int32_t parse(const uint8_t* sps, uint32_t size);

private:
    int32_t U(uint32_t bits, const uint8_t *buff, uint32_t &bitpos);
    int32_t Ue(const uint8_t* buff, uint32_t len, uint32_t &bitpos);
};

#endif