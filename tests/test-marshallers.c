#include <glib.h>
#include <string.h>

#include "common/marshaller.h"
#include "generated_test_marshallers.h"

#ifndef g_assert_true
#define g_assert_true g_assert
#endif

static uint8_t expected_data[] = { 123, /* dummy byte */
                                   0x02, 0x00, 0x00, 0x00, /* data_size */
                                   0x09, 0x00, 0x00, 0x00, /* data offset */
                                   0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12, /* data */
                                   0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0xfe, /* data */
};

typedef void (*message_destructor_t)(uint8_t *message);
uint8_t * spice_parse_msg(uint8_t *message_start, uint8_t *message_end,
                          uint32_t channel, uint16_t message_type, int minor,
                          size_t *size_out, message_destructor_t *free_message);

int main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    SpiceMarshaller *marshaller;
    SpiceMsgMainShortDataSubMarshall *msg;
    size_t len, msg_len;
    int free_res;
    uint8_t *data;
    message_destructor_t free_message;

    msg = g_new0(SpiceMsgMainShortDataSubMarshall, 1);
    msg->data = g_new(uint64_t, 2);
    msg->dummy_byte = 123;
    msg->data_size = 2;
    msg->data[0] = 0x1234567890abcdef;
    msg->data[1] = 0xfedcba0987654321;

    marshaller = spice_marshaller_new();
    spice_marshall_msg_main_ShortDataSubMarshall(marshaller, msg);
    spice_marshaller_flush(marshaller);
    data = spice_marshaller_linearize(marshaller, 0, &len, &free_res);
    g_assert_cmpint(len, ==, G_N_ELEMENTS(expected_data));
    g_assert_true(memcmp(data, expected_data, len) == 0);

    g_free(msg->data);
    g_free(msg);

    // test demarshaller
    msg = (SpiceMsgMainShortDataSubMarshall *) spice_parse_msg(data, data + len, 1, 1, 0,
                                                               &msg_len, &free_message);

    g_assert_nonnull(msg);
    g_assert_cmpint(msg->dummy_byte, ==, 123);
    g_assert_cmpint(msg->data_size, ==, 2);
    g_assert_nonnull(msg->data);
    g_assert_cmpint(msg->data[0], ==, 0x1234567890abcdef);
    g_assert_cmpint(msg->data[1], ==, 0xfedcba0987654321);

    free_message((uint8_t *) msg);

    if (free_res) {
        free(data);
    }
    spice_marshaller_reset(marshaller);

    SpiceMsgMainZeroes msg_zeroes = { 0x0102 };

    spice_marshall_msg_main_Zeroes(marshaller, &msg_zeroes);
    spice_marshaller_flush(marshaller);
    data = spice_marshaller_linearize(marshaller, 0, &len, &free_res);
    g_assert_cmpint(len, ==, 7);
    g_assert_true(memcmp(data, "\x00\x02\x01\x00\x00\x00\x00", 7) == 0);
    if (free_res) {
        free(data);
    }

    spice_marshaller_destroy(marshaller);

    return 0;
}
