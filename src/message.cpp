#include "message.h"

uint16_t checksum16(const uint8_t* buf, uint32_t len) {
    uint32_t sum = 0;
    for (uint32_t j = 0; j < len - 1; j += 2) {
    sum += *((uint16_t*)(&buf[j]));
    }
    if ((len & 1) != 0) {
    sum += buf[len - 1];
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum = (sum >> 16) + (sum & 0xFFFF);
    return uint16_t(~sum);
}