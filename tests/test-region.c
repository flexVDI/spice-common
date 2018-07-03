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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <spice/macros.h>

#include <common/region.h>

static int slow_region_test(const QRegion *rgn, const QRegion *other_rgn, int query)
{
    pixman_region32_t intersection;
    int res;

    pixman_region32_init(&intersection);
    pixman_region32_intersect(&intersection,
                              (pixman_region32_t *)rgn,
                              (pixman_region32_t *)other_rgn);

    res = 0;

    if (query & REGION_TEST_SHARED &&
        pixman_region32_not_empty(&intersection)) {
        res |= REGION_TEST_SHARED;
    }

    if (query & REGION_TEST_LEFT_EXCLUSIVE &&
        !pixman_region32_equal(&intersection, (pixman_region32_t *)rgn)) {
        res |= REGION_TEST_LEFT_EXCLUSIVE;
    }

    if (query & REGION_TEST_RIGHT_EXCLUSIVE &&
        !pixman_region32_equal(&intersection, (pixman_region32_t *)other_rgn)) {
        res |= REGION_TEST_RIGHT_EXCLUSIVE;
    }

    pixman_region32_fini(&intersection);

    return res;
}


static int rect_is_valid(const SpiceRect *r)
{
    if (r->top > r->bottom || r->left > r->right) {
        return FALSE;
    }
    return TRUE;
}

static void rect_set(SpiceRect *r, int32_t top, int32_t left, int32_t bottom, int32_t right)
{
    r->top = top;
    r->left = left;
    r->bottom = bottom;
    r->right = right;
    g_assert_true(rect_is_valid(r));
}

static void random_region(QRegion *reg)
{
    int i;
    int num_rects;
    int x, y, w, h;
    SpiceRect _r;
    SpiceRect *r = &_r;

    region_clear(reg);

    num_rects = rand() % 20;
    for (i = 0; i < num_rects; i++) {
        x = rand()%100;
        y = rand()%100;
        w = rand()%100;
        h = rand()%100;
        rect_set(r,
                 x, y,
                 x+w, y+h);
        region_add(reg, r);
    }
}

static void test(const QRegion *r1, const QRegion *r2, int *expected)
{
    g_debug("r1 is_empty %s [%s]",
            region_is_empty(r1) ? "TRUE" : "FALSE",
            (region_is_empty(r1) == *expected) ? "OK" : "ERR");
    g_assert_cmpint(region_is_empty(r1), ==, *expected);
    expected++;
    g_debug("r2 is_empty %s [%s]",
            region_is_empty(r2) ? "TRUE" : "FALSE",
            (region_is_empty(r2) == *expected) ? "OK" : "ERR");
    g_assert_cmpint(region_is_empty(r2), ==, *expected);
    expected++;
    g_debug("is_equal %s [%s]",
            region_is_equal(r1, r2) ? "TRUE" : "FALSE",
            (region_is_equal(r1, r2) == *expected) ? "OK" : "ERR");
    g_assert_cmpint(region_is_equal(r1, r2), ==, *expected);
    expected++;
    g_debug("intersects %s [%s]",
            region_intersects(r1, r2) ? "TRUE" : "FALSE",
            (region_intersects(r1, r2) == *expected) ? "OK" : "ERR");
    g_assert_cmpint(region_intersects(r1, r2), ==, *expected);
    expected++;
    g_debug("contains %s [%s]",
            region_contains(r1, r2) ? "TRUE" : "FALSE",
            (region_contains(r1, r2) == *expected) ? "OK" : "ERR");
    g_assert_cmpint(region_contains(r1, r2), ==, *expected);
    expected++;
}

enum {
    EXPECT_R1_EMPTY,
    EXPECT_R2_EMPTY,
    EXPECT_EQUAL,
    EXPECT_SECT,
    EXPECT_CONT,
};

static void test_region(void)
{
    QRegion _r1, _r2, _r3;
    QRegion *r1 = &_r1;
    QRegion *r2 = &_r2;
    QRegion *r3 = &_r3;
    SpiceRect _r;
    SpiceRect *r = &_r;
    int expected[5];
    int i, j;

    region_init(r1);
    region_init(r2);

    g_debug("dump r1 empty rgn [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = TRUE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = TRUE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clone(r3, r1);
    g_debug("dump r3 clone rgn [%s]", region_is_valid(r3) ? "VALID" : "INVALID");
    //region_dump(r3, "");
    expected[EXPECT_R1_EMPTY] = TRUE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = TRUE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r3, expected);
    region_destroy(r3);
    g_debug("\n");

    rect_set(r, 0, 0, 100, 100);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r1);
    rect_set(r, 0, 0, 0, 0);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = TRUE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = TRUE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    rect_set(r, -100, -100, 0, 0);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r1);
    rect_set(r, -100, -100, 100, 100);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");


    region_clear(r1);
    region_clear(r2);

    rect_set(r, 100, 100, 200, 200);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    rect_set(r, 300, 300, 400, 400);
    region_add(r1, r);
    g_debug("dump r1 [%s]", region_is_valid(r1) ? "VALID" : "INVALID");
    //region_dump(r1, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = TRUE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    rect_set(r, 500, 500, 600, 600);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = FALSE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, 100, 100, 200, 200);
    region_add(r2, r);
    rect_set(r, 300, 300, 400, 400);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = TRUE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, 100, 100, 200, 200);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, -2000, -2000, -1000, -1000);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = FALSE;
    expected[EXPECT_CONT] = FALSE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, -2000, -2000, 1000, 1000);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = FALSE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, 150, 150, 175, 175);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r2, expected);
    g_debug("\n");

    region_clear(r2);

    rect_set(r, 150, 150, 350, 350);
    region_add(r2, r);
    g_debug("dump r2 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = FALSE;
    test(r1, r2, expected);
    g_debug("\n");

    region_and(r2, r1);
    g_debug("dump r2 and r1 [%s]", region_is_valid(r2) ? "VALID" : "INVALID");
    //region_dump(r2, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = FALSE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = FALSE;
    test(r2, r1, expected);
    g_debug("\n");


    region_clone(r3, r1);
    g_debug("dump r3 clone rgn [%s]", region_is_valid(r3) ? "VALID" : "INVALID");
    //region_dump(r3, "");
    expected[EXPECT_R1_EMPTY] = FALSE;
    expected[EXPECT_R2_EMPTY] = FALSE;
    expected[EXPECT_EQUAL] = TRUE;
    expected[EXPECT_SECT] = TRUE;
    expected[EXPECT_CONT] = TRUE;
    test(r1, r3, expected);
    g_debug("\n");

    j = 0;
    for (i = 0; i < 100000; i++) {
        int res1, res2, test;
        int tests[] = {
            REGION_TEST_LEFT_EXCLUSIVE,
            REGION_TEST_RIGHT_EXCLUSIVE,
            REGION_TEST_SHARED,
            REGION_TEST_LEFT_EXCLUSIVE | REGION_TEST_RIGHT_EXCLUSIVE,
            REGION_TEST_LEFT_EXCLUSIVE | REGION_TEST_SHARED,
            REGION_TEST_RIGHT_EXCLUSIVE | REGION_TEST_SHARED,
            REGION_TEST_LEFT_EXCLUSIVE | REGION_TEST_RIGHT_EXCLUSIVE | REGION_TEST_SHARED
        };

        random_region(r1);
        random_region(r2);

        for (test = 0; test < 7; test++) {
            res1 = region_test(r1, r2, tests[test]);
            res2 = slow_region_test(r1, r2, tests[test]);
            if (res1 != res2) {
                g_warning("Error in region_test %d, got %d, expected %d, query=%d",
                          j, res1, res2, tests[test]);
                g_debug("r1:");
                region_dump(r1, "");
                g_debug("r2:");
                region_dump(r2, "");
            }
            g_assert_cmpint(res1, ==, res2);
            j++;
        }
    }

    region_destroy(r3);
    region_destroy(r1);
    region_destroy(r2);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/spice-common/region", test_region);

    return g_test_run();
}
