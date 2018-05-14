/*
   Copyright (C) 2017 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "common/quic.h"

typedef struct {
    QuicUsrContext usr;
    GByteArray *dest;
} QuicData;

static SPICE_GNUC_NORETURN SPICE_GNUC_PRINTF(2, 3) void
quic_usr_error(QuicUsrContext *usr, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, fmt, ap);
    va_end(ap);

    g_assert_not_reached();
}

static SPICE_GNUC_PRINTF(2, 3) void
quic_usr_warn(QuicUsrContext *usr, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, fmt, ap);
    va_end(ap);
}

static void *quic_usr_malloc(QuicUsrContext *usr, int size)
{
    return g_malloc(size);
}


static void quic_usr_free(QuicUsrContext *usr, void *ptr)
{
    g_free(ptr);
}

static int quic_usr_more_space(QuicUsrContext *usr, uint32_t **io_ptr, int rows_completed)
{
    QuicData *quic_data = (QuicData *)usr;
    int initial_len = quic_data->dest->len;

    g_byte_array_set_size(quic_data->dest, quic_data->dest->len*2);

    *io_ptr = (uint32_t *)(quic_data->dest->data + initial_len);
    return (quic_data->dest->len - initial_len)/4;
}


static int quic_usr_more_lines(QuicUsrContext *usr, uint8_t **lines)
{
    g_return_val_if_reached(0);
}


static void init_quic_data(QuicData *quic_data)
{
    quic_data->usr.error = quic_usr_error;
    quic_data->usr.warn = quic_usr_warn;
    quic_data->usr.info = quic_usr_warn;
    quic_data->usr.malloc = quic_usr_malloc;
    quic_data->usr.free = quic_usr_free;
    quic_data->usr.more_space = quic_usr_more_space;
    quic_data->usr.more_lines = quic_usr_more_lines;
    quic_data->dest = g_byte_array_new();
}

static GByteArray *quic_encode_from_pixbuf(GdkPixbuf *pixbuf)
{
    QuicData quic_data;
    QuicContext *quic;
    int encoded_size;
    QuicImageType quic_type;

    init_quic_data(&quic_data);
    g_byte_array_set_size(quic_data.dest, 1024);

    quic = quic_create(&quic_data.usr);
    g_assert(quic != NULL);
    switch (gdk_pixbuf_get_n_channels(pixbuf)) {
        case 3:
            quic_type = QUIC_IMAGE_TYPE_RGB24;
            break;
        case 4:
            quic_type = QUIC_IMAGE_TYPE_RGBA;
            break;
        default:
            g_assert_not_reached();
    }
    encoded_size = quic_encode(quic, quic_type,
                               gdk_pixbuf_get_width(pixbuf),
                               gdk_pixbuf_get_height(pixbuf),
                               gdk_pixbuf_get_pixels(pixbuf),
                               gdk_pixbuf_get_height(pixbuf),
                               gdk_pixbuf_get_rowstride(pixbuf),
                               (uint32_t *)quic_data.dest->data,
                               quic_data.dest->len/sizeof(uint32_t));
    g_assert(encoded_size > 0);
    encoded_size *= 4;
    g_byte_array_set_size(quic_data.dest, encoded_size);
    quic_destroy(quic);

    return quic_data.dest;
}

static GdkPixbuf *quic_decode_to_pixbuf(GByteArray *compressed_data)
{
    QuicData quic_data;
    QuicContext *quic;
    GdkPixbuf *pixbuf;
    QuicImageType type;
    int width;
    int height;
    int status;

    init_quic_data(&quic_data);
    g_byte_array_free(quic_data.dest, TRUE);
    quic_data.dest = NULL;

    quic = quic_create(&quic_data.usr);
    g_assert(quic != NULL);

    status = quic_decode_begin(quic,
                               (uint32_t *)compressed_data->data, compressed_data->len/4,
                               &type, &width, &height);
    g_assert(status == QUIC_OK);

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                            (type == QUIC_IMAGE_TYPE_RGBA), 8,
                            width, height);
    status = quic_decode(quic, type,
                         gdk_pixbuf_get_pixels(pixbuf),
                         gdk_pixbuf_get_rowstride(pixbuf));
    g_assert(status == QUIC_OK);
    quic_destroy(quic);

    return pixbuf;
}

static void gdk_pixbuf_compare(GdkPixbuf *pixbuf_a, GdkPixbuf *pixbuf_b)
{
    int width = gdk_pixbuf_get_width(pixbuf_a);
    int height = gdk_pixbuf_get_height(pixbuf_a);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf_a);
    int x;
    int y;
    guint8 *pixels_a = gdk_pixbuf_get_pixels(pixbuf_a);
    guint8 *pixels_b = gdk_pixbuf_get_pixels(pixbuf_b);

    g_assert(width == gdk_pixbuf_get_width(pixbuf_b));
    g_assert(height == gdk_pixbuf_get_height(pixbuf_b));
    g_assert(n_channels == gdk_pixbuf_get_n_channels(pixbuf_b));
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            guint8 *p_a = pixels_a + y*gdk_pixbuf_get_rowstride(pixbuf_a) + x*n_channels;
            guint8 *p_b = pixels_b + y*gdk_pixbuf_get_rowstride(pixbuf_b) + x*n_channels;

            g_assert(p_a[0] == p_b[0]);
            g_assert(p_a[1] == p_b[1]);
            g_assert(p_a[2] == p_b[2]);
            if (gdk_pixbuf_get_has_alpha(pixbuf_a)) {
                g_assert(p_a[3] == p_b[3]);
            }
        }
    }
}

static GdkPixbuf *gdk_pixbuf_new_random(void)
{
    gboolean has_alpha = g_random_boolean();
    gint width = g_random_int_range(100, 2000);
    gint height = g_random_int_range(100, 2000);
    GdkPixbuf *random_pixbuf;
    guint i;
    guint8 *pixels;

    random_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, width, height);
    pixels = gdk_pixbuf_get_pixels(random_pixbuf);
    for (i = 0; i < gdk_pixbuf_get_byte_length(random_pixbuf); i++) {
        pixels[i] = g_random_int_range(0, 256);
    }

    return random_pixbuf;
}

static void test_pixbuf(GdkPixbuf *pixbuf)
{
    GdkPixbuf *uncompressed_pixbuf;
    GByteArray *compressed_data;
    g_assert(pixbuf != NULL);
    g_assert(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB);
    g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8);

    compressed_data = quic_encode_from_pixbuf(pixbuf);

    uncompressed_pixbuf = quic_decode_to_pixbuf(compressed_data);

    g_assert(gdk_pixbuf_get_byte_length(pixbuf) == gdk_pixbuf_get_byte_length(uncompressed_pixbuf));
    //g_assert(memcmp(gdk_pixbuf_get_pixels(pixbuf), gdk_pixbuf_get_pixels(uncompressed_pixbuf), gdk_pixbuf_get_byte_length(uncompressed_pixbuf)));
    gdk_pixbuf_compare(pixbuf, uncompressed_pixbuf);

    g_byte_array_free(compressed_data, TRUE);
    g_object_unref(uncompressed_pixbuf);

}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        for (int i = 1; i < argc; ++i) {
            GdkPixbuf *source_pixbuf;

            source_pixbuf = gdk_pixbuf_new_from_file(argv[i], NULL);
            test_pixbuf(source_pixbuf);
            g_object_unref(source_pixbuf);
        }
    } else if (argc == 1) {
        unsigned int count;

        for (count = 0; count < 50; count++) {
            GdkPixbuf *pixbuf = gdk_pixbuf_new_random();
            test_pixbuf(pixbuf);
            g_object_unref(pixbuf);
        }
    } else {
        g_assert_not_reached();
    }

    return 0;
}
