#include <cstdio>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;

#include "bit_tools.h"
#include "splitter.h"

bool simulate_bit_copy (
    unsigned char *input_buffer,
    int input_start,
    int input_end,
    unsigned char *output_buffer,
    int output_start,
    int buffer_len = 8
) {
    string input = array_to_bit_string (input_buffer, buffer_len);
    string output = array_to_bit_string (output_buffer, buffer_len);
    unsigned char *original_output = new unsigned char[buffer_len];
    memcpy (original_output, output_buffer, buffer_len);
    string correct_after = output;
    copy (input.begin() + input_start, input.begin() + input_end, correct_after.begin() + output_start);
    bit_copy (input_buffer, input_start, input_end, output_buffer, output_start);

    output = array_to_bit_string (output_buffer, buffer_len);

    if (output != correct_after ) {
        printf ("\nError: %d %d %d\n", input_start, input_end, output_start);
        printf ("Source:\n");
        print_debug_buffer (input_buffer, buffer_len, {input_start, input_end});
        printf ("Before:\n");
        print_debug_buffer (original_output, buffer_len, {output_start});
        printf ("Output/correct:\n");
        print_debug_buffer (output_buffer, buffer_len, {output_start, output_start + input_end - input_start});
        printf ("%s\n", correct_after.c_str() );
        return false;
    }
    return true;
}

int total = 0;
int skip = -1;
bool test_bit_copy_instance (int c_start, int c_end, int c_dest, const int buffer_len = 3 * 8) {
    assert (c_start >= 0);
    assert (c_start < 8 * buffer_len);
    assert (c_end >= 0);
    assert (c_end <= 8 * buffer_len);
    assert (c_dest >= 0);
    assert (c_start <= c_end);
    assert (c_dest + c_end - c_start <= 8 * buffer_len);
    unsigned char input_buffer[buffer_len];
    unsigned char output_buffer[buffer_len];
    for (int i = 0; i < buffer_len; i++) {
        input_buffer[i] = rand();
    }
    for (int i = 0; i < buffer_len; i++) {
        output_buffer[i] = rand();
    }
    if (total++ < skip) return true;
    bool ret = simulate_bit_copy (input_buffer, c_start, c_end, output_buffer, c_dest, buffer_len);
    if (skip >= 0 && total >= skip) return false;
    return ret;
}

void test_bit_copy() {
    const int buffer_len = 8;
    for (unsigned int i = 0; i < 100000; i++) {
        int bbuffer_len = buffer_len * 8;
        int start = rand() % bbuffer_len;
        int stop = start + rand() % (bbuffer_len - start);
        int dest = rand() % (bbuffer_len - stop + start);
        if (!test_bit_copy_instance (start, stop, dest, buffer_len) ) {
            printf ("Ended at instance %d\n", i);
            exit (1);
        }
    }
    printf ("TESTING bit_copy() WAS SUCCESSFUL\n");
}

void test_crc32_madness() {
    unsigned char *data = (unsigned char*) "ipsc2014\n";
    unsigned long long len = strlen ( (char*) data);
    Splitter splitter;
    unsigned long long how_much = 899981 * 2 - 1000;
    unsigned long long written = 0;
    while (written + len < how_much) {
        written += len;
        splitter.addData (data, len);
    }
    splitter.finishInput();
    vector<BZ2Block> blocks;
    int blocks_c = 0;
    for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
        BZ2Block &block = *block_ptr;
        block_ptr = nullptr;
        blocks.push_back (block);
        blocks_c ++ ;
    }
    cerr << "STREAM STUFF" << endl;
    cerr << array_to_bit_string (splitter.crc32_stream, 4) << endl;

    printf ("B1: 0x%8x\n", blocks[0].crc32);
    printf ("B2: 0x%8x\n", blocks[1].crc32);
    cerr << array_to_bit_string (crc32_to_crc32_string (bz2_crc32_combine (blocks[0].crc32, blocks[1].crc32) ), 4) << endl;
    printf ("Combined crc: %8x\n", bz2_crc32_combine (blocks[0].crc32, blocks[1].crc32) );
    printf ("%d\n", blocks_c);
    printf ("0x%08x\n", blocks[0].crc32);
    printf ("0x%08x\n", crc32_combine_count (blocks[0].crc32, 1) );
    printf ("0x%08x\n", bz2_crc32_combine (0, blocks[0].crc32) );
    printf ("0x%08x\n", bz2_crc32_combine (blocks[0].crc32, blocks[0].crc32) );
    printf ("0x%08x\n", crc32_combine_count (blocks[0].crc32, 2) );
    printf ("0x%08x\n", bz2_crc32_combine (blocks[0].crc32, bz2_crc32_combine (blocks[0].crc32, blocks[0].crc32)) );
    printf ("0x%08x\n", bz2_crc32_combine (bz2_crc32_combine ( blocks[0].crc32, blocks[0].crc32), blocks[0].crc32) );
    printf ("0x%08x\n", crc32_combine_count (blocks[0].crc32, 3) );
}

int main() {
    test_bit_copy();
    test_crc32_madness();
}
