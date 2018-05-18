#include <stdint.h>

#ifndef _H_TEST_MARSHALLERS

typedef struct {
    uint32_t data_size;
    uint8_t dummy_byte;
    uint64_t *data;
} SpiceMsgMainShortDataSubMarshall;

typedef struct {
    int8_t *name;
} SpiceMsgMainArrayMessage;

typedef struct {
    uint16_t n;
} SpiceMsgMainZeroes;

typedef struct SpiceMsgChannels {
    uint32_t num_of_channels;
    uint16_t channels[0];
} SpiceMsgChannels;

typedef struct {
    uint32_t dummy[2];
    uint8_t data[0];
} SpiceMsgMainLenMessage;

#endif /* _H_TEST_MARSHALLERS */

