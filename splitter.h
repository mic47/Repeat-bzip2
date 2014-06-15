#ifndef SPLITTER_H
#define SPLITTER_H
#include <string>
#include <algorithm>
#include <deque>

#include <bzlib.h>
using namespace std;
#include "period.h"

class BZ2Block {
    public:
        unsigned char *raw_data;
        unsigned long long raw_data_bits;
        unsigned long long raw_data_bytes;
        unsigned char *crc32_string;
        unsigned int crc32; 
        unsigned long long source_begin_byte;
        unsigned long long source_end_byte;
        unsigned long long source_data_length;
        BZ2Block();
        BZ2Block (
            unsigned char *data,
            unsigned long long start,
            unsigned long long stop,
            unsigned long long position = 0
        );
        BZ2Block(const BZ2Block &data);
        BZ2Block operator=(const BZ2Block &data);
        ~BZ2Block();
};

unsigned char* crc32_to_crc32_string (unsigned int crc32);
unsigned int bz2_crc32_combine (unsigned int crc1, unsigned int crc2);
unsigned int crc32_combine_count (unsigned int crc32_data, unsigned long long data_count); 

class Splitter {
    private:
        bz_stream *stream;
        unsigned char *input_buffer;
        unsigned char *output_buffer;
        unsigned int buffer_size;
        unsigned long long input_offset;
        bool finished;

        vector<BZ2Block> blocks;
        unsigned long long blocks_index;
        unsigned long long bit_index;
        unsigned long long last_index;
        unsigned long long window;
        unsigned long long position;

    public:
        unsigned char crc32_stream[4];
        Splitter();
        ~Splitter();
        void reset();
        void addData (string &data);
        void addData (const unsigned char *data, size_t length);
        void addData (RepItem data, unsigned long long offset = 0);
        void finishInput();
        void process(); 
        BZ2Block *getData();

        static unsigned char *compressed_magic_string;
        static unsigned char *eos_magic_string;
        unsigned long long compressed_magic;
        unsigned long long eos_magic;

        /* helper methods */

        static unsigned long long magic_string_to_magic_int (
            unsigned char *str
        );
        static pair<unsigned char*, size_t> get_bzip2_archive_from_block (
            unsigned char *stream,
            unsigned long long from,
            unsigned long long to
        );

};

#endif
