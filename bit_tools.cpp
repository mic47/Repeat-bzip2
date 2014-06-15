#include "bit_tools.h"

string char_to_bit_string (unsigned char a) {
    string ret = "";
    for (int i = 0; i < 8; i++) {
        ret.push_back ('0' + (a % 2) );
        a /= 2;
    }
    reverse (ret.begin(), ret.end() );
    return ret;
}

string array_to_bit_string (unsigned char *array, int len) {
    string ret;
    for (int i = 0; i < len; i++) {
        ret = ret + char_to_bit_string (array[i]);
    }
    return ret;
}

void print_debug_buffer (unsigned char *buffer, int len, vector<int> pointers) {
    sort (pointers.begin(), pointers.end() );
    for (int i = 0; i < len; i++) {
        printf ("%8d", buffer[i]);
    }
    printf ("\n");
    if (pointers.size() > 0) {
        auto prev = 0;
        for (auto pointer : pointers) {
            for (; prev < pointer; prev++) {
                printf (" ");
            }
            printf ("*");
            prev++;
        }
        printf ("\n");
    }
    printf ("%s\n", array_to_bit_string (buffer, len).c_str() );

}

unsigned long long get_bit (
    unsigned char *stream,
    unsigned long long bit_index
) {
    auto index = bit_index / 8;
    auto offset = 7 - (bit_index % 8);
    return (stream[index] & (1 << offset) ) >> (offset);
}

void set_bit (
    unsigned char *stream,
    unsigned long long bit_index,
    unsigned long long bit
) {
    auto index = bit_index / 8;
    auto offset = 7 - (bit_index % 8);
    if (bit == 0) {
        stream[index] &= ~ (1 << (offset) );
    } else {
        stream[index] |= 1 << (offset);
    }
}


