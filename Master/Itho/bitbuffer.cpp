/** @file
    A two-dimensional bit buffer consisting of bytes.

    Copyright (C) 2015 Tommy Vestermark

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "bitbuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>

void bitbuffer_clear(bitbuffer_t *bits)
{
    memset(bits, 0, sizeof(*bits));
}

void bitbuffer_add_bit(bitbuffer_t *bits, int bit)
{
    if (bits->num_rows == 0)
        bits->free_row = bits->num_rows = 1; // Add first row automatically

    if (bits->bits_per_row[bits->num_rows - 1] == UINT16_MAX) {
        // Serial.printf("%s: Could not add more bits\n", __func__);
        return;
    }
    if (bits->bits_per_row[bits->num_rows - 1] == UINT16_MAX - 1) {
        Serial.printf("%s: Warning: row length limit (%u bits) reached\n", __func__, UINT16_MAX);
    }

    uint16_t col_index = bits->bits_per_row[bits->num_rows - 1] / 8;
    uint16_t bit_index = bits->bits_per_row[bits->num_rows - 1] % 8;
    if (bits->bits_per_row[bits->num_rows - 1] > 0
            && bits->bits_per_row[bits->num_rows - 1] % (BITBUF_COLS * 8) == 0) {
        // spill into next row
        // Serial.printf("%s: row spill [%d] to %d (%d)\n", __func__, bits->num_rows - 1, col_index, bits->free_row);
        if (bits->free_row == BITBUF_ROWS - 1) {
            Serial.printf("%s: Warning: row count limit (%d rows) reached\n", __func__, BITBUF_ROWS);
        }
        if (bits->free_row < BITBUF_ROWS) {
            bits->free_row++;
        }
        else {
            // Serial.printf("%s: Could not add more rows\n", __func__);
            return;
        }
    }
    uint8_t *b = bits->bb[bits->num_rows - 1];
    b[col_index] |= (bit << (7 - bit_index));
    bits->bits_per_row[bits->num_rows - 1]++;

/*
    // preamble compression
    if (bits->bits_per_row[bits->num_rows - 1] == 60 * 8) {
        uint8_t *b = bits->bb[bits->num_rows - 1];
        for (int i = 21; i < 60; ++i) {
            if (b[20] != b[i]) {
                return;
            }
        }
        // Serial.printf("%s: preamble compression\n", __func__);
        memset(&b[30], 0, 30);
        bits->bits_per_row[bits->num_rows - 1] = 30 * 8;
    }
*/
}

/// Set the width of the current (last) row by expanding or truncating as needed.
static void bitbuffer_set_width(bitbuffer_t *bits, uint16_t width)
{
    if (bits->num_rows == 0)
        bits->free_row = bits->num_rows = 1; // Add first row automatically

    unsigned remaining_rows = BITBUF_ROWS - bits->num_rows + 1;
    unsigned remaining_bits = remaining_rows * BITBUF_COLS * 8;
    if (width > remaining_bits) {
        // Serial.printf("%s: Could not add more bits\n", __func__);
        width = remaining_bits;
    }

    // clear bits when truncating
    if (bits->bits_per_row[bits->num_rows - 1] > width) {
        uint8_t *b = bits->bb[bits->num_rows - 1];
        unsigned clr_from = (width + 7) / 8;
        unsigned clr_end  = (bits->bits_per_row[bits->num_rows - 1] + 7) / 8;
        memset(&b[clr_from], 0, clr_end - clr_from);

        // note that width became strictly smaller, that way we don't overflow
        b[width / 8] &= 0xff00 >> (width % 8);
    }

    bits->bits_per_row[bits->num_rows - 1] = width;

    unsigned extra_rows = width == 0 ? 0 : (width - 1) / (BITBUF_COLS * 8);
    bits->free_row = bits->num_rows + extra_rows;
}

void bitbuffer_add_row(bitbuffer_t *bits)
{
    if (bits->num_rows == 0)
        bits->free_row = bits->num_rows = 1; // Add first row automatically
    if (bits->free_row == BITBUF_ROWS - 1) {
        // Serial.printf("%s: Warning: row count limit (%d rows) reached\n", __func__, BITBUF_ROWS);
    }
    if (bits->free_row < BITBUF_ROWS) {
        bits->free_row++;
        bits->num_rows = bits->free_row;
    }
    else {
        bits->bits_per_row[bits->num_rows - 1] = 0; // Clear last row to handle overflow somewhat gracefully
        // Serial.printf("ERROR: bitbuffer:: Could not add more rows\n");    // Some decoders may add many rows...
    }
}

void bitbuffer_invert(bitbuffer_t *bits)
{
    for (unsigned row = 0; row < bits->num_rows; ++row) {
        if (bits->bits_per_row[row] > 0) {
            uint8_t *b = bits->bb[row];

            const unsigned last_col  = (bits->bits_per_row[row] - 1) / 8;
            const unsigned last_bits = ((bits->bits_per_row[row] - 1) % 8) + 1;
            for (unsigned col = 0; col <= last_col; ++col) {
                b[col] = ~b[col]; // Invert
            }
            b[last_col] ^= 0xFF >> last_bits; // Re-invert unused bits in last byte
        }
    }
}

void bitbuffer_nrzs_decode(bitbuffer_t *bits)
{
    for (unsigned row = 0; row < bits->num_rows; ++row) {
        if (bits->bits_per_row[row] > 0) {
            uint8_t *b = bits->bb[row];

            const unsigned last_col  = (bits->bits_per_row[row] - 1) / 8;
            const unsigned last_bits = ((bits->bits_per_row[row] - 1) % 8) + 1;

            int prev = 0;
            for (unsigned col = 0; col <= last_col; ++col) {
                int mask = (prev << 7) | b[col] >> 1;
                prev     = b[col];
                b[col]   = b[col] ^ ~mask;
            }
            b[last_col] &= 0xFF << (8 - last_bits); // Clear unused bits in last byte
        }
    }
}

void bitbuffer_nrzm_decode(bitbuffer_t *bits)
{
    for (unsigned row = 0; row < bits->num_rows; ++row) {
        if (bits->bits_per_row[row] > 0) {
            uint8_t *b = bits->bb[row];

            const unsigned last_col  = (bits->bits_per_row[row] - 1) / 8;
            const unsigned last_bits = ((bits->bits_per_row[row] - 1) % 8) + 1;

            int prev = 0;
            for (unsigned col = 0; col <= last_col; ++col) {
                int mask = (prev << 7) | b[col] >> 1;
                prev     = b[col];
                b[col]   = b[col] ^ mask;
            }
            b[last_col] &= 0xFF << (8 - last_bits); // Clear unused bits in last byte
        }
    }
}

void bitbuffer_extract_bytes(bitbuffer_t *bitbuffer, unsigned row,
        unsigned pos, uint8_t *out, unsigned len)
{
    uint8_t *bits = bitbuffer->bb[row];
    if (len == 0)
        return;
    if ((pos & 7) == 0) {
        memcpy(out, bits + (pos / 8), (len + 7) / 8);
    }
    else {
        unsigned shift = 8 - (pos & 7);
        unsigned bytes = (len + 7) >> 3;
        uint8_t *p = out;
        uint16_t word;
        pos = pos >> 3; // Convert to bytes

        word = bits[pos];

        while (bytes--) {
            word <<= 8;
            word |= bits[++pos];
            *(p++) = word >> shift;
        }
    }
    if (len & 7)
        out[(len - 1) / 8] &= 0xff00 >> (len & 7); // mask off bottom bits
}

// If we make this an inline function instead of a macro, it means we don't
// have to worry about using bit numbers with side-effects (bit++).
static inline uint8_t bit_at(const uint8_t *bytes, unsigned bit)
{
    return (uint8_t)(bytes[bit >> 3] >> (7 - (bit & 7)) & 1);
}

unsigned bitbuffer_search(bitbuffer_t *bitbuffer, unsigned row, unsigned start,
        const uint8_t *pattern, unsigned pattern_bits_len)
{
    uint8_t *bits = bitbuffer->bb[row];
    unsigned len  = bitbuffer->bits_per_row[row];
    unsigned ipos = start;
    unsigned ppos = 0; // cursor on init pattern

    while (ipos < len && ppos < pattern_bits_len) {
        if (bit_at(bits, ipos) == bit_at(pattern, ppos)) {
            ppos++;
            ipos++;
            if (ppos == pattern_bits_len)
                return ipos - pattern_bits_len;
        }
        else {
            ipos -= ppos;
            ipos++;
            ppos = 0;
        }
    }

    // Not found
    return len;
}

unsigned bitbuffer_manchester_decode(bitbuffer_t *inbuf, unsigned row, unsigned start,
        bitbuffer_t *outbuf, unsigned max)
{
    uint8_t *bits     = inbuf->bb[row];
    unsigned int len  = inbuf->bits_per_row[row];
    unsigned int ipos = start;

    if (max && len > start + (max * 2))
        len = start + (max * 2);

    while (ipos < len) {
        uint8_t bit1, bit2;

        bit1 = bit_at(bits, ipos++);
        bit2 = bit_at(bits, ipos++);

        if (bit1 == bit2)
            break;

        bitbuffer_add_bit(outbuf, bit2);
    }

    return ipos;
}

unsigned bitbuffer_differential_manchester_decode(bitbuffer_t *inbuf, unsigned row, unsigned start,
        bitbuffer_t *outbuf, unsigned max)
{
    uint8_t *bits     = inbuf->bb[row];
    unsigned int len  = inbuf->bits_per_row[row];
    unsigned int ipos = start;
    uint8_t bit1, bit2 = 0;

    if (max && len > start + (max * 2))
        len = start + (max * 2);

    // the first long pulse will determine the clock
    // if needed skip one short pulse to get in synch
    while (ipos < len) {
        bit1 = bit_at(bits, ipos++);
        bit2 = bit_at(bits, ipos++);
        uint8_t bit3 = bit_at(bits, ipos);

        if (bit1 != bit2) {
            if (bit2 != bit3) {
                bitbuffer_add_bit(outbuf, 0);
            }
            else {
                bit2 = bit1;
                ipos -= 1;
                break;
            }
        }
        else {
            bit2 = 1 - bit1;
            ipos -= 2;
            break;
        }
    }

    while (ipos < len) {
        bit1 = bit_at(bits, ipos++);
        if (bit1 == bit2)
            break; // clock missing, abort
        bit2 = bit_at(bits, ipos++);

        if (bit1 == bit2)
            bitbuffer_add_bit(outbuf, 1);
        else
            bitbuffer_add_bit(outbuf, 0);
    }

    return ipos;
}

static void print_bitrow(uint8_t const *bitrow, unsigned bit_len, unsigned highest_indent, int always_binary)
{
    unsigned row_len = 0;

    Serial.printf("{%2u} ", bit_len);
    for (unsigned col = 0; col < (bit_len + 7) / 8; ++col) {
        row_len += Serial.printf("%02x ", bitrow[col]);
    }
    // Print binary values also?
    if (always_binary || bit_len <= BITBUF_MAX_PRINT_BITS) {
        Serial.printf("%-*s: ", highest_indent > row_len ? highest_indent - row_len : 0, "");
        for (unsigned bit = 0; bit < bit_len; ++bit) {
            if (bitrow[bit / 8] & (0x80 >> (bit % 8))) {
                Serial.printf("1");
            }
            else {
                Serial.printf("0");
            }
            if ((bit % 8) == 7) // Add byte separators
                Serial.printf(" ");
        }
    }
    Serial.printf("\n");
}

static void print_bitbuffer(const bitbuffer_t *bits, int always_binary)
{
    unsigned highest_indent, indent_this_col, indent_this_row;
    unsigned col, row;

    /* Figure out the longest row of bit to get the highest_indent
     */
    highest_indent = sizeof("[dd] {dd} ") - 1;
    for (row = indent_this_row = 0; row < bits->num_rows; ++row) {
        for (col = indent_this_col = 0; col < (unsigned)(bits->bits_per_row[row] + 7) / 8; ++col) {
            indent_this_col += 2 + 1;
        }
        indent_this_row = indent_this_col;
        if (indent_this_row > highest_indent)
            highest_indent = indent_this_row;
    }

    Serial.printf("bitbuffer:: Number of rows: %u \n", bits->num_rows);
    for (row = 0; row < bits->num_rows; ++row) {
        Serial.printf("[%02u] ", row);
        print_bitrow(bits->bb[row], bits->bits_per_row[row], highest_indent, always_binary);
    }
    if (bits->num_rows >= BITBUF_ROWS) {
        Serial.printf("... Maximum number of rows reached. Message is likely truncated.\n");
    }
}

void bitbuffer_print(const bitbuffer_t *bits)
{
    print_bitbuffer(bits, 0);
}

void bitbuffer_debug(const bitbuffer_t *bits)
{
    print_bitbuffer(bits, 1);
}

void bitrow_print(uint8_t const *bitrow, unsigned bit_len)
{
    print_bitrow(bitrow, bit_len, 0, 0);
}

void bitrow_debug(uint8_t const *bitrow, unsigned bit_len)
{
    print_bitrow(bitrow, bit_len, 0, 1);
}

int bitrow_snprint(uint8_t const *bitrow, unsigned bit_len, char *str, unsigned size)
{
    if (bit_len == 0 && size > 0) {
        str[0] = '\0';
    }
    int len = 0;
    for (unsigned i = 0; size > (unsigned)len && i < (bit_len + 7) / 8; ++i) {
        len += snprintf(str + len, size - len, "%02x", bitrow[i]);
    }
    return len;
}

void bitbuffer_parse(bitbuffer_t *bits, const char *code)
{
    const char *c;
    int data  = 0;
    int width = -1;

    bitbuffer_clear(bits);

    for (c = code; *c; ++c) {

        if (*c == ' ') {
            continue;
        }
        else if (*c == '0' && (*(c + 1) == 'x' || *(c + 1) == 'X')) {
            ++c;
            continue;
        }
        else if (*c == '{') {
            if (width >= 0) {
                bitbuffer_set_width(bits, width);
            }
            if (bits->num_rows > 0) {
                bitbuffer_add_row(bits);
            }

            width = strtol(c + 1, (char **)&c, 0);
            if (width > BITBUF_MAX_ROW_BITS)
                width = BITBUF_MAX_ROW_BITS;
            if (!*c)
                break; // no closing brace and end of string
            continue;
        }
        else if (*c == '/') {
            if (width >= 0) {
                bitbuffer_set_width(bits, width);
                width = -1;
            }
            bitbuffer_add_row(bits);
            continue;
        }
        else if (*c >= '0' && *c <= '9') {
            data = *c - '0';
        }
        else if (*c >= 'A' && *c <= 'F') {
            data = *c - 'A' + 10;
        }
        else if (*c >= 'a' && *c <= 'f') {
            data = *c - 'a' + 10;
        }
        bitbuffer_add_bit(bits, data >> 3 & 0x01);
        bitbuffer_add_bit(bits, data >> 2 & 0x01);
        bitbuffer_add_bit(bits, data >> 1 & 0x01);
        bitbuffer_add_bit(bits, data >> 0 & 0x01);
    }
    if (width >= 0) {
        bitbuffer_set_width(bits, width);
    }
}

int compare_rows(bitbuffer_t *bits, unsigned row_a, unsigned row_b)
{
    return (bits->bits_per_row[row_a] == bits->bits_per_row[row_b]
            && !memcmp(bits->bb[row_a], bits->bb[row_b],
                    (bits->bits_per_row[row_a] + 7) / 8));
}

unsigned count_repeats(bitbuffer_t *bits, unsigned row)
{
    unsigned cnt = 0;
    for (int i = 0; i < bits->num_rows; ++i) {
        if (compare_rows(bits, row, i)) {
            ++cnt;
        }
    }
    return cnt;
}

int bitbuffer_find_repeated_row(bitbuffer_t *bits, unsigned min_repeats, unsigned min_bits)
{
    for (int i = 0; i < bits->num_rows; ++i) {
        if (bits->bits_per_row[i] >= min_bits &&
                count_repeats(bits, i) >= min_repeats) {
            return i;
        }
    }
    return -1;
}
