/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2009 Red Hat, Inc.

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

#ifdef ONE_BYTE
#undef ONE_BYTE
#define FNAME(name) quic_one_##name
#define PIXEL one_byte_t
#endif

#ifdef FOUR_BYTE
#undef FOUR_BYTE
#define FNAME(name) quic_four_##name
#define PIXEL four_bytes_t
#endif

#define golomb_coding golomb_coding_8bpc
#define golomb_decoding golomb_decoding_8bpc
#define update_model update_model_8bpc
#define find_bucket find_bucket_8bpc
#define family family_8bpc

#define BPC 8
#define BPC_MASK 0xffU

#define _PIXEL_A ((unsigned int)curr[-1].a)
#define _PIXEL_B ((unsigned int)prev[0].a)

#ifdef RLE
#define RLE_PRED_IMP                                                       \
if (prev_row[i - 1].a == prev_row[i].a) {                                  \
    if (run_index != i && i > 2 && cur_row[i - 1].a == cur_row[i - 2].a) { \
        goto do_run;                                                       \
    }                                                                      \
}
#else
#define RLE_PRED_IMP
#endif

/*  a  */
static inline BYTE FNAME(decorrelate_0)(const PIXEL * const curr, const unsigned int bpc_mask)
{
    return family.xlatU2L[(unsigned)((int)curr[0].a - (int)_PIXEL_A) & bpc_mask];
}

static inline void FNAME(correlate_0)(PIXEL *curr, const BYTE correlate,
                                      const unsigned int bpc_mask)
{
    curr->a = (family.xlatL2U[correlate] + _PIXEL_A) & bpc_mask;
}


/*  (a+b)/2  */
static inline BYTE FNAME(decorrelate)(const PIXEL *const prev, const PIXEL * const curr,
                                      const unsigned int bpc_mask)
{
    return family.xlatU2L[(unsigned)((int)curr->a - (int)((_PIXEL_A + _PIXEL_B) >> 1)) & bpc_mask];
}


static inline void FNAME(correlate)(const PIXEL *prev, PIXEL *curr, const BYTE correlate,
                                    const unsigned int bpc_mask)
{
    curr->a = (family.xlatL2U[correlate] + (int)((_PIXEL_A + _PIXEL_B) >> 1)) & bpc_mask;
}


static void FNAME(compress_row0_seg)(Encoder *encoder, Channel *channel, int i,
                                     const PIXEL * const cur_row,
                                     const int end,
                                     const unsigned int waitmask,
                                     SPICE_GNUC_UNUSED const unsigned int bpc,
                                     const unsigned int bpc_mask)
{
    BYTE * const decorrelate_drow = channel->correlate_row;
    int stopidx;

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codeword, codewordlen;

        decorrelate_drow[0] = family.xlatU2L[cur_row->a];
        golomb_coding(decorrelate_drow[0], find_bucket(channel, decorrelate_drow[-1])->bestcode,
                      &codeword, &codewordlen);
        encode(encoder, codeword, codewordlen);

        if (channel->state.waitcnt) {
            channel->state.waitcnt--;
        } else {
            channel->state.waitcnt = (tabrand(&channel->state.tabrand_seed) & waitmask);
            update_model(&channel->state, find_bucket(channel, decorrelate_drow[-1]),
                         decorrelate_drow[i]);
        }
        stopidx = ++i + channel->state.waitcnt;
    } else {
        stopidx = i + channel->state.waitcnt;
    }

    while (stopidx < end) {
        for (; i <= stopidx; i++) {
            unsigned int codeword, codewordlen;
            decorrelate_drow[i] = FNAME(decorrelate_0)(&cur_row[i], bpc_mask);
            golomb_coding(decorrelate_drow[i],
                          find_bucket(channel, decorrelate_drow[i - 1])->bestcode, &codeword,
                          &codewordlen);
            encode(encoder, codeword, codewordlen);
        }

        update_model(&channel->state, find_bucket(channel, decorrelate_drow[stopidx - 1]),
                     decorrelate_drow[stopidx]);
        stopidx = i + (tabrand(&channel->state.tabrand_seed) & waitmask);
    }

    for (; i < end; i++) {
        unsigned int codeword, codewordlen;
        decorrelate_drow[i] = FNAME(decorrelate_0)(&cur_row[i], bpc_mask);
        golomb_coding(decorrelate_drow[i], find_bucket(channel, decorrelate_drow[i - 1])->bestcode,
                      &codeword, &codewordlen);
        encode(encoder, codeword, codewordlen);
    }
    channel->state.waitcnt = stopidx - end;
}

static void FNAME(compress_row0)(Encoder *encoder, Channel *channel, const PIXEL *cur_row,
                                 unsigned int width)
{
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    int pos = 0;

    while ((DEFwmimax > (int)channel->state.wmidx) && (channel->state.wmileft <= width)) {
        if (channel->state.wmileft) {
            FNAME(compress_row0_seg)(encoder, channel, pos, cur_row, pos + channel->state.wmileft,
                                     bppmask[channel->state.wmidx], bpc, bpc_mask);
            width -= channel->state.wmileft;
            pos += channel->state.wmileft;
        }

        channel->state.wmidx++;
        set_wm_trigger(&channel->state);
        channel->state.wmileft = DEFwminext;
    }

    if (width) {
        FNAME(compress_row0_seg)(encoder, channel, pos, cur_row, pos + width,
                                 bppmask[channel->state.wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)channel->state.wmidx) {
            channel->state.wmileft -= width;
        }
    }

    spice_assert((int)channel->state.wmidx <= DEFwmimax);
    spice_assert(channel->state.wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

static void FNAME(compress_row_seg)(Encoder *encoder, Channel *channel, int i,
                                    const PIXEL * const prev_row,
                                    const PIXEL * const cur_row,
                                    const int end,
                                    const unsigned int waitmask,
                                    SPICE_GNUC_UNUSED const unsigned int bpc,
                                    const unsigned int bpc_mask)
{
    BYTE * const decorrelate_drow = channel->correlate_row;
    int stopidx;
#ifdef RLE
    int run_index = 0;
    int run_size;
#endif

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codeword, codewordlen;

        decorrelate_drow[0] = family.xlatU2L[(unsigned)((int)cur_row->a -
                                                       (int)prev_row->a) & bpc_mask];

        golomb_coding(decorrelate_drow[0],
                      find_bucket(channel, decorrelate_drow[-1])->bestcode,
                      &codeword,
                      &codewordlen);
        encode(encoder, codeword, codewordlen);

        if (channel->state.waitcnt) {
            channel->state.waitcnt--;
        } else {
            channel->state.waitcnt = (tabrand(&channel->state.tabrand_seed) & waitmask);
            update_model(&channel->state, find_bucket(channel, decorrelate_drow[-1]),
                         decorrelate_drow[0]);
        }
        stopidx = ++i + channel->state.waitcnt;
    } else {
        stopidx = i + channel->state.waitcnt;
    }
    for (;;) {
        while (stopidx < end) {
            for (; i <= stopidx; i++) {
                unsigned int codeword, codewordlen;
                RLE_PRED_IMP;
                decorrelate_drow[i] = FNAME(decorrelate)(&prev_row[i], &cur_row[i], bpc_mask);
                golomb_coding(decorrelate_drow[i],
                              find_bucket(channel, decorrelate_drow[i - 1])->bestcode, &codeword,
                              &codewordlen);
                encode(encoder, codeword, codewordlen);
            }

            update_model(&channel->state, find_bucket(channel, decorrelate_drow[stopidx - 1]),
                         decorrelate_drow[stopidx]);
            stopidx = i + (tabrand(&channel->state.tabrand_seed) & waitmask);
        }

        for (; i < end; i++) {
            unsigned int codeword, codewordlen;
            RLE_PRED_IMP;
            decorrelate_drow[i] = FNAME(decorrelate)(&prev_row[i], &cur_row[i], bpc_mask);
            golomb_coding(decorrelate_drow[i], find_bucket(channel,
                                                          decorrelate_drow[i - 1])->bestcode,
                          &codeword, &codewordlen);
            encode(encoder, codeword, codewordlen);
        }
        channel->state.waitcnt = stopidx - end;

        return;

#ifdef RLE
do_run:
        run_index = i;
        channel->state.waitcnt = stopidx - i;
        run_size = 0;

        while (cur_row[i].a == cur_row[i - 1].a) {
            run_size++;
            if (++i == end) {
                encode_channel_run(encoder, channel, run_size);
                return;
            }
        }
        encode_channel_run(encoder, channel, run_size);
        stopidx = i + channel->state.waitcnt;
#endif
    }
}

static void FNAME(compress_row)(Encoder *encoder, Channel *channel,
                                const PIXEL * const prev_row,
                                const PIXEL * const cur_row,
                                unsigned int width)

{
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)channel->state.wmidx) && (channel->state.wmileft <= width)) {
        if (channel->state.wmileft) {
            FNAME(compress_row_seg)(encoder, channel, pos, prev_row, cur_row,
                                    pos + channel->state.wmileft, bppmask[channel->state.wmidx],
                                    bpc, bpc_mask);
            width -= channel->state.wmileft;
            pos += channel->state.wmileft;
        }

        channel->state.wmidx++;
        set_wm_trigger(&channel->state);
        channel->state.wmileft = DEFwminext;
    }

    if (width) {
        FNAME(compress_row_seg)(encoder, channel, pos, prev_row, cur_row, pos + width,
                                bppmask[channel->state.wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)channel->state.wmidx) {
            channel->state.wmileft -= width;
        }
    }

    spice_assert((int)channel->state.wmidx <= DEFwmimax);
    spice_assert(channel->state.wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

static void FNAME(uncompress_row0_seg)(Encoder *encoder, Channel *channel, int i,
                                       BYTE * const correlate_row,
                                       PIXEL * const cur_row,
                                       const int end,
                                       const unsigned int waitmask,
                                       SPICE_GNUC_UNUSED const unsigned int bpc,
                                       const unsigned int bpc_mask)
{
    int stopidx;

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codewordlen;

        correlate_row[0] = (BYTE)golomb_decoding(find_bucket(channel,
                                                             correlate_row[-1])->bestcode,
                                                 encoder->io_word, &codewordlen);
        cur_row[0].a = (BYTE)family.xlatL2U[correlate_row[0]];
        decode_eatbits(encoder, codewordlen);

        if (channel->state.waitcnt) {
            --channel->state.waitcnt;
        } else {
            channel->state.waitcnt = (tabrand(&channel->state.tabrand_seed) & waitmask);
            update_model(&channel->state, find_bucket(channel, correlate_row[-1]),
                         correlate_row[0]);
        }
        stopidx = ++i + channel->state.waitcnt;
    } else {
        stopidx = i + channel->state.waitcnt;
    }

    while (stopidx < end) {
        struct s_bucket * pbucket = NULL;

        for (; i <= stopidx; i++) {
            unsigned int codewordlen;

            pbucket = find_bucket(channel, correlate_row[i - 1]);
            correlate_row[i] = (BYTE)golomb_decoding(pbucket->bestcode, encoder->io_word,
                                                     &codewordlen);
            FNAME(correlate_0)(&cur_row[i], correlate_row[i], bpc_mask);
            decode_eatbits(encoder, codewordlen);
        }

        update_model(&channel->state, pbucket, correlate_row[stopidx]);

        stopidx = i + (tabrand(&channel->state.tabrand_seed) & waitmask);
    }

    for (; i < end; i++) {
        unsigned int codewordlen;

        correlate_row[i] = (BYTE)golomb_decoding(find_bucket(channel,
                                                             correlate_row[i - 1])->bestcode,
                                                 encoder->io_word, &codewordlen);
        FNAME(correlate_0)(&cur_row[i], correlate_row[i], bpc_mask);
        decode_eatbits(encoder, codewordlen);
    }
    channel->state.waitcnt = stopidx - end;
}

static void FNAME(uncompress_row0)(Encoder *encoder, Channel *channel,
                                   PIXEL * const cur_row,
                                   unsigned int width)

{
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    BYTE * const correlate_row = channel->correlate_row;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)channel->state.wmidx) && (channel->state.wmileft <= width)) {
        if (channel->state.wmileft) {
            FNAME(uncompress_row0_seg)(encoder, channel, pos, correlate_row, cur_row,
                                       pos + channel->state.wmileft, bppmask[channel->state.wmidx],
                                       bpc, bpc_mask);
            pos += channel->state.wmileft;
            width -= channel->state.wmileft;
        }

        channel->state.wmidx++;
        set_wm_trigger(&channel->state);
        channel->state.wmileft = DEFwminext;
    }

    if (width) {
        FNAME(uncompress_row0_seg)(encoder, channel, pos, correlate_row, cur_row, pos + width,
                                   bppmask[channel->state.wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)channel->state.wmidx) {
            channel->state.wmileft -= width;
        }
    }

    spice_assert((int)channel->state.wmidx <= DEFwmimax);
    spice_assert(channel->state.wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

static void FNAME(uncompress_row_seg)(Encoder *encoder, Channel *channel,
                                      BYTE *correlate_row,
                                      const PIXEL * const prev_row,
                                      PIXEL * const cur_row,
                                      int i,
                                      const int end,
                                      SPICE_GNUC_UNUSED const unsigned int bpc,
                                      const unsigned int bpc_mask)
{
    const unsigned int waitmask = bppmask[channel->state.wmidx];
    int stopidx;
#ifdef RLE
    int run_index = 0;
    int run_end;
#endif

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codewordlen;

        correlate_row[0] = (BYTE)golomb_decoding(find_bucket(channel, correlate_row[-1])->bestcode,
                                                             encoder->io_word, &codewordlen);
        cur_row[0].a = (family.xlatL2U[correlate_row[0]] + prev_row[0].a) & bpc_mask;
        decode_eatbits(encoder, codewordlen);

        if (channel->state.waitcnt) {
            --channel->state.waitcnt;
        } else {
            channel->state.waitcnt = (tabrand(&channel->state.tabrand_seed) & waitmask);
            update_model(&channel->state, find_bucket(channel, correlate_row[-1]),
                         correlate_row[0]);
        }
        stopidx = ++i + channel->state.waitcnt;
    } else {
        stopidx = i + channel->state.waitcnt;
    }
    for (;;) {
        while (stopidx < end) {
            struct s_bucket * pbucket = NULL;

            for (; i <= stopidx; i++) {
                unsigned int codewordlen;
                RLE_PRED_IMP;
                pbucket = find_bucket(channel, correlate_row[i - 1]);
                correlate_row[i] = (BYTE)golomb_decoding(pbucket->bestcode, encoder->io_word,
                                                         &codewordlen);
                FNAME(correlate)(&prev_row[i], &cur_row[i], correlate_row[i], bpc_mask);
                decode_eatbits(encoder, codewordlen);
            }

            update_model(&channel->state, pbucket, correlate_row[stopidx]);

            stopidx = i + (tabrand(&channel->state.tabrand_seed) & waitmask);
        }

        for (; i < end; i++) {
            unsigned int codewordlen;
            RLE_PRED_IMP;
            correlate_row[i] = (BYTE)golomb_decoding(find_bucket(channel,
                                                                 correlate_row[i - 1])->bestcode,
                                                     encoder->io_word, &codewordlen);
            FNAME(correlate)(&prev_row[i], &cur_row[i], correlate_row[i], bpc_mask);
            decode_eatbits(encoder, codewordlen);
        }

        channel->state.waitcnt = stopidx - end;

        return;

#ifdef RLE
do_run:
        channel->state.waitcnt = stopidx - i;
        run_index = i;
        run_end = i + decode_channel_run(encoder, channel);

        for (; i < run_end; i++) {
            cur_row[i].a = cur_row[i - 1].a;
        }

        if (i == end) {
            return;
        }

        stopidx = i + channel->state.waitcnt;
#endif
    }
}

static void FNAME(uncompress_row)(Encoder *encoder, Channel *channel,
                                  const PIXEL * const prev_row,
                                  PIXEL * const cur_row,
                                  unsigned int width)

{
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    BYTE * const correlate_row = channel->correlate_row;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)channel->state.wmidx) && (channel->state.wmileft <= width)) {
        if (channel->state.wmileft) {
            FNAME(uncompress_row_seg)(encoder, channel, correlate_row, prev_row, cur_row, pos,
                                      pos + channel->state.wmileft, bpc, bpc_mask);
            pos += channel->state.wmileft;
            width -= channel->state.wmileft;
        }

        channel->state.wmidx++;
        set_wm_trigger(&channel->state);
        channel->state.wmileft = DEFwminext;
    }

    if (width) {
        FNAME(uncompress_row_seg)(encoder, channel, correlate_row, prev_row, cur_row, pos,
                                  pos + width, bpc, bpc_mask);
        if (DEFwmimax > (int)channel->state.wmidx) {
            channel->state.wmileft -= width;
        }
    }

    spice_assert((int)channel->state.wmidx <= DEFwmimax);
    spice_assert(channel->state.wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

#undef PIXEL
#undef FNAME
#undef _PIXEL_A
#undef _PIXEL_B
#undef RLE_PRED_IMP
#undef golomb_coding
#undef golomb_decoding
#undef update_model
#undef find_bucket
#undef family
#undef BPC
#undef BPC_MASK
