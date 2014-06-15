#ifndef BIT_TOOLS_H
#define BIT_TOOLS_H

#include <string>
#include <algorithm>
#include <vector>
using namespace std;

string char_to_bit_string (unsigned char a);

string array_to_bit_string (unsigned char *array, int len);

void print_debug_buffer (unsigned char *buffer, int len, vector<int> pointers = {});

unsigned long long get_bit (
    unsigned char *stream,
    unsigned long long bit_index
);

void set_bit (
    unsigned char *stream,
    unsigned long long bit_index,
    unsigned long long bit
);

template <typename T>
void bit_copy_to_aligned_buffer (
    unsigned char *input_buffer,
    T input_start,
    T input_end,
    unsigned char *output_buffer,
    T output_segment
) {
    auto start_segment = input_start / 8;
    auto start_offset = input_start % 8;
    auto end_segment = input_end / 8;
    auto end_offset = input_end % 8;
    unsigned char first_mask = (1 << (8 - start_offset) ) - 1;
    unsigned char second_mask = ~first_mask;
    auto iter_end = end_segment - 1;
    auto len = (input_end - input_start) / 8;
    auto len_offset = (input_end - input_start) % 8;
    output_buffer[output_segment + len] &= ( (1 << (8 - len_offset) ) - 1);
    if (end_offset >= start_offset) {
        iter_end += 1;
    }
    auto buffer_segment = output_segment;
    for (auto input_index = start_segment; input_index < iter_end; input_index++) {
        // I can do assignment since I am using whole segment
        output_buffer[buffer_segment++] =
            ( (input_buffer[input_index] & first_mask) << start_offset) |
            ( (input_buffer[input_index + 1] & second_mask) >> (8 - start_offset) );
    }
    auto end_shift = 8 - end_offset;
    unsigned char end_mask = (~ ( (1 << end_shift) - 1) );
    if (end_offset >= start_offset) {
        output_buffer[buffer_segment++] |=
            (input_buffer[end_segment] & (first_mask & end_mask) ) << start_offset;
    } else {
        auto prefix = ( (input_buffer[end_segment - 1] & first_mask) << (start_offset) );
        auto suffix = ( (input_buffer[end_segment] & end_mask) >> (8 - start_offset) ); //(end_shift - 1) );
        output_buffer[buffer_segment++] |= prefix | suffix;
    }
}

template <typename T>
T bit_copy (
    unsigned char *input_buffer,
    T input_start,
    T input_end,
    unsigned char *output_buffer,
    T output_start
) {

    unsigned char *buffer = nullptr;
    auto len = (input_end - input_start) / 8;
    auto buffer_segment = output_start / 8;
    if (output_start % 8 == 0) {
        buffer = output_buffer;
    } else {
        buffer = new unsigned char[len + 1];
        buffer_segment = 0;
    }
    // Clear buffer
    // We only need to clear last bits, because full segments will be overwritten
    // Copy to the buffer
    bit_copy_to_aligned_buffer (
        input_buffer,
        input_start,
        input_end,
        buffer,
        buffer_segment
    );
    if (output_start % 8 == 0) {
        return input_end - input_start;
    }
    // copy from buffer to output
    auto output_segment = output_start / 8;
    auto output_offset = output_start % 8;
    auto output_init_len = 8 - output_offset;
    unsigned char output_init_mask = (1 << output_init_len) - 1;
    auto bit_len = input_end - input_start;
    //toto prepise cely segment
    if (output_init_len > bit_len) {
        output_init_mask -= (1 << (output_init_len - bit_len) ) - 1;
    }
    output_buffer[output_segment] &= ~output_init_mask;
    output_buffer[output_segment] |=
        (buffer[buffer_segment] & (output_init_mask << output_offset) ) >> output_offset;
    output_segment++;
    if (output_init_len < bit_len) {
        bit_copy_to_aligned_buffer (
            buffer,
            output_init_len,
            bit_len,
            output_buffer,
            output_segment
        );
    }
    delete buffer;
    buffer = nullptr;
    return input_end - input_start;
}


#endif
