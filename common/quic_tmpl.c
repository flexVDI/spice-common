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

#define SET_a(pix, val) ((pix)->a = val)
#define GET_a(pix) ((pix)->a)

#define SAME_PIXEL(p1, p2)                                 \
     (GET_a(p1) == GET_a(p2))

#define _PIXEL_A(channel, curr) ((unsigned int)GET_##channel((curr) - 1))
#define _PIXEL_B(channel, prev) ((unsigned int)GET_##channel(prev))

#define RLE_PRED_IMP                                                       \
if (SAME_PIXEL(&prev_row[i - 1], &prev_row[i])) {                                   \
    if (run_index != i && i > 2 && SAME_PIXEL(&cur_row[i - 1], &cur_row[i - 2])) {  \
        goto do_run;                                                       \
    }                                                                      \
}

/*  a  */
static inline BYTE FNAME(decorrelate_0)(const PIXEL * const curr, const unsigned int bpc_mask)
{
    return family.xlatU2L[(unsigned)((int)GET_a(curr) - (int)_PIXEL_A(a, curr)) & bpc_mask];
}

static inline void FNAME(correlate_0)(PIXEL *curr, const BYTE correlate,
                                      const unsigned int bpc_mask)
{
    curr->a = (family.xlatL2U[correlate] + _PIXEL_A(a, curr)) & bpc_mask;
}


/*  (a+b)/2  */
static inline BYTE FNAME(decorrelate)(const PIXEL *const prev, const PIXEL * const curr,
                                      const unsigned int bpc_mask)
{
    return family.xlatU2L[(unsigned)((int)curr->a - (int)((_PIXEL_A(a, curr) + _PIXEL_B(a, prev)) >> 1)) & bpc_mask];
}


static inline void FNAME(correlate)(const PIXEL *prev, PIXEL *curr, const BYTE correlate,
                                    const unsigned int bpc_mask)
{
    curr->a = (family.xlatL2U[correlate] + (int)((_PIXEL_A(a, curr) + _PIXEL_B(a, prev)) >> 1)) & bpc_mask;
}

#define COMPRESS_ONE_ROW0_0(channel)                                 \
    channel->correlate_row[0] = family.xlatU2L[cur_row->a];          \
    golomb_coding(encoder, channel->correlate_row[0],                \
                  find_bucket(channel,                               \
                              channel->correlate_row[-1])->bestcode)

#define COMPRESS_ONE_ROW0(channel, index)                                               \
    channel->correlate_row[index] = FNAME(decorrelate_0)(&cur_row[index], bpc_mask);    \
    golomb_coding(encoder, channel->correlate_row[index],                               \
                  find_bucket(channel,                                                  \
                              channel->correlate_row[index - 1])->bestcode)

#define UPDATE_MODEL(index)                                                             \
    update_model(state, find_bucket(channel, channel->correlate_row[index - 1]),        \
                 channel->correlate_row[index]);

static void FNAME(compress_row0_seg)(Encoder *encoder, Channel *channel, int i,
                                     const PIXEL * const cur_row,
                                     const int end,
                                     const unsigned int waitmask,
                                     SPICE_GNUC_UNUSED const unsigned int bpc,
                                     const unsigned int bpc_mask)
{
    CommonState *state = &channel->state;
    int stopidx;

    spice_assert(end - i > 0);

    if (i == 0) {
        COMPRESS_ONE_ROW0_0(channel);

        if (state->waitcnt) {
            state->waitcnt--;
        } else {
            state->waitcnt = (tabrand(&state->tabrand_seed) & waitmask);
            UPDATE_MODEL(0);
        }
        stopidx = ++i + state->waitcnt;
    } else {
        stopidx = i + state->waitcnt;
    }

    while (stopidx < end) {
        for (; i <= stopidx; i++) {
            COMPRESS_ONE_ROW0(channel, i);
        }

        UPDATE_MODEL(stopidx);
        stopidx = i + (tabrand(&state->tabrand_seed) & waitmask);
    }

    for (; i < end; i++) {
        COMPRESS_ONE_ROW0(channel, i);
    }
    state->waitcnt = stopidx - end;
}

#undef COMPRESS_ONE_ROW0_0
#undef COMPRESS_ONE_ROW0

static void FNAME(compress_row0)(Encoder *encoder, Channel *channel, const PIXEL *cur_row,
                                 unsigned int width)
{
    CommonState *state = &channel->state;
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    int pos = 0;

    while ((DEFwmimax > (int)state->wmidx) && (state->wmileft <= width)) {
        if (state->wmileft) {
            FNAME(compress_row0_seg)(encoder, channel, pos, cur_row, pos + state->wmileft,
                                     bppmask[state->wmidx], bpc, bpc_mask);
            width -= state->wmileft;
            pos += state->wmileft;
        }

        state->wmidx++;
        set_wm_trigger(state);
        state->wmileft = DEFwminext;
    }

    if (width) {
        FNAME(compress_row0_seg)(encoder, channel, pos, cur_row, pos + width,
                                 bppmask[state->wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)state->wmidx) {
            state->wmileft -= width;
        }
    }

    spice_assert((int)state->wmidx <= DEFwmimax);
    spice_assert(state->wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

#define COMPRESS_ONE_0(channel) \
    channel->correlate_row[0] = family.xlatU2L[(unsigned)((int)GET_a(cur_row) -                 \
                                                          (int)GET_a(prev_row) ) & bpc_mask];   \
    golomb_coding(encoder, channel->correlate_row[0],                                           \
                  find_bucket(channel, channel->correlate_row[-1])->bestcode)

#define COMPRESS_ONE(channel, index)                                                                  \
     channel->correlate_row[index] = FNAME(decorrelate)(&prev_row[index], &cur_row[index], bpc_mask); \
     golomb_coding(encoder, channel->correlate_row[index],                                            \
                  find_bucket(channel, channel->correlate_row[index - 1])->bestcode)

static void FNAME(compress_row_seg)(Encoder *encoder, Channel *channel, int i,
                                    const PIXEL * const prev_row,
                                    const PIXEL * const cur_row,
                                    const int end,
                                    const unsigned int waitmask,
                                    SPICE_GNUC_UNUSED const unsigned int bpc,
                                    const unsigned int bpc_mask)
{
    CommonState *state = &channel->state;
    int stopidx;
    int run_index = 0;
    int run_size;

    spice_assert(end - i > 0);

    if (i == 0) {
        COMPRESS_ONE_0(channel);

        if (state->waitcnt) {
            state->waitcnt--;
        } else {
            state->waitcnt = (tabrand(&state->tabrand_seed) & waitmask);
            update_model(state, find_bucket(channel, channel->correlate_row[-1]),
                         channel->correlate_row[0]);
        }
        stopidx = ++i + state->waitcnt;
    } else {
        stopidx = i + state->waitcnt;
    }
    for (;;) {
        while (stopidx < end) {
            for (; i <= stopidx; i++) {
                RLE_PRED_IMP;
                COMPRESS_ONE(channel, i);
            }

            UPDATE_MODEL(stopidx);
            stopidx = i + (tabrand(&state->tabrand_seed) & waitmask);
        }

        for (; i < end; i++) {
            RLE_PRED_IMP;
            COMPRESS_ONE(channel, i);
        }
        state->waitcnt = stopidx - end;

        return;

do_run:
        run_index = i;
        state->waitcnt = stopidx - i;
        run_size = 0;

        while (SAME_PIXEL(&cur_row[i], &cur_row[i - 1])) {
            run_size++;
            if (++i == end) {
                encode_channel_run(encoder, channel, run_size);
                return;
            }
        }
        encode_channel_run(encoder, channel, run_size);
        stopidx = i + state->waitcnt;
    }
}

static void FNAME(compress_row)(Encoder *encoder, Channel *channel,
                                const PIXEL * const prev_row,
                                const PIXEL * const cur_row,
                                unsigned int width)

{
    CommonState *state = &channel->state;
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)state->wmidx) && (state->wmileft <= width)) {
        if (state->wmileft) {
            FNAME(compress_row_seg)(encoder, channel, pos, prev_row, cur_row,
                                    pos + state->wmileft, bppmask[state->wmidx],
                                    bpc, bpc_mask);
            width -= state->wmileft;
            pos += state->wmileft;
        }

        state->wmidx++;
        set_wm_trigger(state);
        state->wmileft = DEFwminext;
    }

    if (width) {
        FNAME(compress_row_seg)(encoder, channel, pos, prev_row, cur_row, pos + width,
                                bppmask[state->wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)state->wmidx) {
            state->wmileft -= width;
        }
    }

    spice_assert((int)state->wmidx <= DEFwmimax);
    spice_assert(state->wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

#define UNCOMPRESS_PIX_START(row) do { } while (0)

#define UNCOMPRESS_ONE_ROW0_0(channel)                                                                  \
    channel->correlate_row[0] = (BYTE)golomb_decoding(find_bucket(channel,                              \
                                                                  channel->correlate_row[-1])->bestcode,\
                                                      encoder->io_word, &codewordlen);                  \
    SET_a(&cur_row[0], (BYTE)family.xlatL2U[channel->correlate_row[0]]);                                \
    decode_eatbits(encoder, codewordlen);

#define UNCOMPRESS_ONE_ROW0(channel)                                                                            \
    channel->correlate_row[i] = (BYTE)golomb_decoding(find_bucket(channel,                                      \
                                                                  channel->correlate_row[i - 1])->bestcode,     \
                                                                  encoder->io_word,                             \
                                                                  &codewordlen);                                \
    FNAME(correlate_0)(&cur_row[i], channel->correlate_row[i], bpc_mask);                                                \
    decode_eatbits(encoder, codewordlen);

static void FNAME(uncompress_row0_seg)(Encoder *encoder, Channel *channel, int i,
                                       PIXEL * const cur_row,
                                       const int end,
                                       const unsigned int waitmask,
                                       SPICE_GNUC_UNUSED const unsigned int bpc,
                                       const unsigned int bpc_mask)
{
    CommonState *state = &channel->state;
    int stopidx;

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codewordlen;

        UNCOMPRESS_PIX_START(&cur_row[i]);
        UNCOMPRESS_ONE_ROW0_0(channel);

        if (state->waitcnt) {
            --state->waitcnt;
        } else {
            state->waitcnt = (tabrand(&state->tabrand_seed) & waitmask);
            UPDATE_MODEL(0);
        }
        stopidx = ++i + state->waitcnt;
    } else {
        stopidx = i + state->waitcnt;
    }

    while (stopidx < end) {

        for (; i <= stopidx; i++) {
            unsigned int codewordlen;

            UNCOMPRESS_PIX_START(&cur_row[i]);
            UNCOMPRESS_ONE_ROW0(channel);
        }

        UPDATE_MODEL(stopidx);
        stopidx = i + (tabrand(&state->tabrand_seed) & waitmask);
    }

    for (; i < end; i++) {
        unsigned int codewordlen;

        UNCOMPRESS_PIX_START(&cur_row[i]);
        UNCOMPRESS_ONE_ROW0(channel);
    }
    state->waitcnt = stopidx - end;
}

static void FNAME(uncompress_row0)(Encoder *encoder, Channel *channel,
                                   PIXEL * const cur_row,
                                   unsigned int width)

{
    CommonState *state = &channel->state;
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)state->wmidx) && (state->wmileft <= width)) {
        if (state->wmileft) {
            FNAME(uncompress_row0_seg)(encoder, channel, pos, cur_row,
                                       pos + state->wmileft, bppmask[state->wmidx],
                                       bpc, bpc_mask);
            pos += state->wmileft;
            width -= state->wmileft;
        }

        state->wmidx++;
        set_wm_trigger(state);
        state->wmileft = DEFwminext;
    }

    if (width) {
        FNAME(uncompress_row0_seg)(encoder, channel, pos, cur_row, pos + width,
                                   bppmask[state->wmidx], bpc, bpc_mask);
        if (DEFwmimax > (int)state->wmidx) {
            state->wmileft -= width;
        }
    }

    spice_assert((int)state->wmidx <= DEFwmimax);
    spice_assert(state->wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

#define UNCOMPRESS_ONE_0(channel) \
    channel->correlate_row[0] = (BYTE)golomb_decoding(find_bucket(channel,                              \
                                                                  channel->correlate_row[-1])->bestcode,\
                                                      encoder->io_word, &codewordlen);                  \
    SET_a(&cur_row[0], (family.xlatL2U[channel->correlate_row[0]] +                                     \
                  GET_a(prev_row)) & bpc_mask);                                                         \
    decode_eatbits(encoder, codewordlen);

#define UNCOMPRESS_ONE(channel)                                                                                 \
    channel->correlate_row[i] = (BYTE)golomb_decoding(find_bucket(channel,                                      \
                                                                  channel->correlate_row[i - 1])->bestcode,     \
                                                      encoder->io_word, &codewordlen);                          \
    FNAME(correlate)(&prev_row[i], &cur_row[i], channel->correlate_row[i], bpc_mask);                           \
    decode_eatbits(encoder, codewordlen);

static void FNAME(uncompress_row_seg)(Encoder *encoder, Channel *channel,
                                      const PIXEL * const prev_row,
                                      PIXEL * const cur_row,
                                      int i,
                                      const int end,
                                      SPICE_GNUC_UNUSED const unsigned int bpc,
                                      const unsigned int bpc_mask)
{
    CommonState *state = &channel->state;
    const unsigned int waitmask = bppmask[state->wmidx];
    int stopidx;
    int run_index = 0;
    int run_end;

    spice_assert(end - i > 0);

    if (i == 0) {
        unsigned int codewordlen;

        UNCOMPRESS_PIX_START(&cur_row[i]);
        UNCOMPRESS_ONE_0(channel);

        if (state->waitcnt) {
            --state->waitcnt;
        } else {
            state->waitcnt = (tabrand(&state->tabrand_seed) & waitmask);
            UPDATE_MODEL(0);
        }
        stopidx = ++i + state->waitcnt;
    } else {
        stopidx = i + state->waitcnt;
    }
    for (;;) {
        while (stopidx < end) {
            for (; i <= stopidx; i++) {
                unsigned int codewordlen;
                RLE_PRED_IMP;

                UNCOMPRESS_PIX_START(&cur_row[i]);
                UNCOMPRESS_ONE(channel);
            }

            UPDATE_MODEL(stopidx);

            stopidx = i + (tabrand(&state->tabrand_seed) & waitmask);
        }

        for (; i < end; i++) {
            unsigned int codewordlen;
            RLE_PRED_IMP;
            UNCOMPRESS_PIX_START(&cur_row[i]);
            UNCOMPRESS_ONE(channel);
        }

        state->waitcnt = stopidx - end;

        return;

do_run:
        state->waitcnt = stopidx - i;
        run_index = i;
        run_end = i + decode_channel_run(encoder, channel);

        for (; i < run_end; i++) {
            UNCOMPRESS_PIX_START(&cur_row[i]);
            SET_a(&cur_row[i], GET_a(&cur_row[i - 1]));
        }

        if (i == end) {
            return;
        }

        stopidx = i + state->waitcnt;
    }
}

static void FNAME(uncompress_row)(Encoder *encoder, Channel *channel,
                                  const PIXEL * const prev_row,
                                  PIXEL * const cur_row,
                                  unsigned int width)

{
    CommonState *state = &channel->state;
    const unsigned int bpc = BPC;
    const unsigned int bpc_mask = BPC_MASK;
    unsigned int pos = 0;

    while ((DEFwmimax > (int)state->wmidx) && (state->wmileft <= width)) {
        if (state->wmileft) {
            FNAME(uncompress_row_seg)(encoder, channel, prev_row, cur_row, pos,
                                      pos + state->wmileft, bpc, bpc_mask);
            pos += state->wmileft;
            width -= state->wmileft;
        }

        state->wmidx++;
        set_wm_trigger(state);
        state->wmileft = DEFwminext;
    }

    if (width) {
        FNAME(uncompress_row_seg)(encoder, channel, prev_row, cur_row, pos,
                                  pos + width, bpc, bpc_mask);
        if (DEFwmimax > (int)state->wmidx) {
            state->wmileft -= width;
        }
    }

    spice_assert((int)state->wmidx <= DEFwmimax);
    spice_assert(state->wmidx <= 32);
    spice_assert(DEFwminext > 0);
}

#undef PIXEL
#undef FNAME
#undef _PIXEL_A
#undef _PIXEL_B
#undef SAME_PIXEL
#undef RLE_PRED_IMP
#undef golomb_coding
#undef golomb_decoding
#undef update_model
#undef find_bucket
#undef family
#undef BPC
#undef BPC_MASK
#undef UPDATE_MODEL
#undef UNCOMPRESS_PIX_START
#undef COMPRESS_ONE_0
#undef COMPRESS_ONE
#undef UNCOMPRESS_ONE_ROW0
#undef UNCOMPRESS_ONE_ROW0_0
#undef UNCOMPRESS_ONE_0
#undef UNCOMPRESS_ONE
