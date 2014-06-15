#include <bzlib.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <zlib.h>
using namespace std;

#include "splitter.h"
#include "bit_tools.h"

unsigned char *Splitter::compressed_magic_string = (unsigned char*) "\x31\x41\x59\x26\x53\x59";
unsigned char *Splitter::eos_magic_string = (unsigned char*) "\x17\x72\x45\x38\x50\x90";

unsigned int bz2_crc32_combine (unsigned int crc1, unsigned int crc2) {
    crc1 = (crc1 << 1) | (crc1 >> 31);
    crc1 ^= crc2;
    return crc1;
}

unsigned int crc32_combine_count (
    unsigned int crc32_data,
    unsigned long long data_count
) {
    unsigned int accu = 0;
    for (unsigned long long i = 0; i < data_count; i++) {
        accu = bz2_crc32_combine (accu, crc32_data);
    }
    return accu;
    auto one = crc32_data;
    for (; data_count > 0; data_count /= 2) {
        if (data_count % 2 == 1) {
            accu = bz2_crc32_combine (accu, one);
        }
        one = bz2_crc32_combine (one, one);
    }
    return accu;
}


unsigned char* crc32_to_crc32_string (unsigned int crc32) {
    auto out = new unsigned char[4];
    for (int i = 0; i < 4; i++) {
        out[i] = crc32 >> 24;
        crc32 <<= 8;
    }
    return out;
}

BZ2Block::BZ2Block() {
    this->raw_data = nullptr;
    this->crc32_string = nullptr;
    this->raw_data_bits = 0;
    this->raw_data_bytes = 0;
    this->crc32 = 0;
    this->source_begin_byte = 0;
    this->source_end_byte = 0;
    this->source_data_length = 0;
}

BZ2Block::BZ2Block (
    unsigned char *data,
    unsigned long long start,
    unsigned long long stop,
    unsigned long long position
) {
    this->raw_data_bits = stop - start;
    this->raw_data_bytes = this->raw_data_bits / 8;
    if (this->raw_data_bits % 8 > 0) {
        this->raw_data_bytes++;
    }
    this->raw_data = new unsigned char[this->raw_data_bytes];
    bit_copy (data, start, stop, this->raw_data, 0LLU);
    this->crc32_string = new unsigned char[4];
    bit_copy (this->raw_data, 48LLU, 80LLU, this->crc32_string, 0LLU);
    auto compressed_stream = Splitter::get_bzip2_archive_from_block (
                                 this->raw_data,
                                 0,
                                 this->raw_data_bits
                             );
    unsigned int uncompressed_data_len = 50 * 1024 * 1024;
    char *uncompressed_data = new char[uncompressed_data_len];
    int ret = BZ2_bzBuffToBuffDecompress (
                  uncompressed_data,
                  &uncompressed_data_len,
                  (char*) compressed_stream.first,
                  compressed_stream.second,
                  0,
                  0
              );
    if (ret != BZ_OK) {
        fprintf (stderr, "R %d\n", ret);
        //TODO
        int a = 10;
        int b = 0;
        fprintf (stderr, "%d\n", a / b);
        exit (2);
    }
    this->source_begin_byte = position;
    this->source_end_byte = position + uncompressed_data_len;
    this->source_data_length = uncompressed_data_len;
    this->crc32 =
        (unsigned int) this->crc32_string[0] << 24 |
        (unsigned int) this->crc32_string[1] << 16 |
        (unsigned int) this->crc32_string[2] << 8 |
        (unsigned int) this->crc32_string[3];
    delete uncompressed_data;
    delete compressed_stream.first;
}

BZ2Block::BZ2Block (const BZ2Block &data) {
    this->raw_data = new unsigned char[data.raw_data_bytes];
    memcpy (this->raw_data, data.raw_data, data.raw_data_bytes);
    this->raw_data_bytes = data.raw_data_bytes;
    this->raw_data_bits = data.raw_data_bits;
    this->crc32_string = new unsigned char[4];
    memcpy (this->crc32_string, data.crc32_string, 4);
    this->crc32 = data.crc32;
    this->source_begin_byte = data.source_begin_byte;
    this->source_end_byte = data.source_end_byte;
    this->source_data_length = data.source_data_length;
}

BZ2Block BZ2Block::operator= (const BZ2Block &_data) {
    BZ2Block data (_data);
    swap (this->raw_data, data.raw_data);
    swap (this->raw_data_bits, data.raw_data_bits);
    swap (this->raw_data_bytes, data.raw_data_bytes);
    swap (this->crc32_string, data.crc32_string);
    swap (this->crc32, data.crc32);
    swap (this->source_begin_byte, data.source_begin_byte);
    swap (this->source_end_byte, data.source_end_byte);
    swap (this->source_data_length, data.source_data_length);
    return *this;
}

BZ2Block::~BZ2Block() {
    if (this->raw_data != nullptr) {
        delete this->raw_data;
        this->raw_data = nullptr;
    }
    if (this->crc32_string != nullptr) {
        delete this->crc32_string;
        this->crc32_string = nullptr;
    }
}

Splitter::Splitter() {
    this->stream = nullptr;
    this->input_buffer = this->output_buffer = nullptr;
    this->buffer_size = 1024 * 1024 * 1024;
    this->reset();
    this->compressed_magic =
        magic_string_to_magic_int (this->compressed_magic_string);
    this->eos_magic =
        magic_string_to_magic_int (this->eos_magic_string);
}

Splitter::~Splitter() {
    if (this->stream != nullptr) {
        BZ2_bzCompressEnd (this->stream);
        this->stream = nullptr;
    }
    if (this->input_buffer != nullptr) {
        delete this->input_buffer;
        this->input_buffer = nullptr;
    }
    if (this->output_buffer != nullptr) {
        delete this->output_buffer;
        this->output_buffer = nullptr;
    }
    this->blocks.resize (0);
}

void Splitter::reset() {
    if (this->stream != nullptr) {
        BZ2_bzCompressEnd (this->stream); // We ignore the errors
        this->stream = nullptr;
    }
    if (this->input_buffer != nullptr) {
        delete this->input_buffer;
        this->input_buffer = nullptr;
    }
    if (this->output_buffer != nullptr) {
        delete this->output_buffer;
        this->output_buffer = nullptr;
    }
    this->stream = new bz_stream;
    this->stream->bzalloc = nullptr;
    this->stream->bzfree = nullptr;
    this->stream->opaque = nullptr;

    auto ret = BZ2_bzCompressInit (this->stream, 9, 0, 0);
    if (ret != BZ_OK) {
        switch (ret) {
        case BZ_CONFIG_ERROR:
            cerr << "The bzip2 library has been miscompiled" << endl;
            break;
        case BZ_PARAM_ERROR:
            cerr << "BZIP2: Wrong ini parameters" << endl;
            break;
        case BZ_MEM_ERROR:
            cerr << "BZIP2: Not enough memory" << endl;
            break;
        default:
            cerr << "BZIP2: Unknown error: " << ret << endl;
        }
        exit (3);
    }
    this->input_buffer = new unsigned char[this->buffer_size];
    this->stream->next_in = (char*) this->input_buffer;
    this->stream->avail_in = 0;
    this->output_buffer = new unsigned char[this->buffer_size];
    this->stream->next_out = (char*) this->output_buffer;
    this->stream->avail_out = this->buffer_size;
    this->input_offset = 0;
    this->bit_index = 0;
    // We assume that block does not start at 0
    this->last_index = 0;
    this->window = 0;
    this->position = 0;
    this->blocks_index = 0;
    this->blocks.resize (0);
    this->finished = false;
}

void Splitter::addData (string &data) {
    this->addData ( (unsigned char *) data.c_str(), data.size() );
}

void Splitter::addData (const unsigned char *data, size_t length) {
    if (this->input_offset == 0) {
        memcpy (
            this->stream->next_in + this->stream->avail_in,
            data,
            length
        );
        this->stream->avail_in += length;
    } else {
        RepItem item;
        item.data = (unsigned char*) data;
        item.bit_len = length * 8;
        item.byte_len = length;
        this->addData (item);
    }
}

void Splitter::addData (RepItem item, unsigned long long offset) {
    auto output_start = this->input_offset + 8LLU * this->stream->avail_in;
    if ( (item.bit_len + output_start ) / 8 > buffer_size) {
        fprintf (stderr, "Buffer is full!\n");
        exit (1 / 0);
    }
    auto read = bit_copy (
                    item.data,
                    offset,
                    item.bit_len,
                    (unsigned char*) this->stream->next_in,
                    output_start
                );
    auto bits_now = output_start + read;
    this->stream->avail_in = bits_now / 8LLU;
    this->input_offset = bits_now % 8;
}

void Splitter::process() {
    if (!this->finished) {
        BZ2_bzCompress (this->stream, BZ_RUN);
    }
    if (window == Splitter::eos_magic) {
        return;
    }
    unsigned long long output_len = this->stream->total_in_hi32;
    output_len <<= 32;
    output_len += this->stream->total_out_lo32;
    output_len *= 8;
    unsigned long long count = 0;
    for (; this->bit_index <= output_len; this->bit_index ++) {
        if (
            this->window == Splitter::compressed_magic ||
            this->window == Splitter::eos_magic
        ) {
            auto hit = this->bit_index - 48;
            if (last_index > 0) {
                this->blocks.push_back (
                    BZ2Block (
                        this->output_buffer,
                        last_index,
                        hit,
                        this->position
                    )
                );
                this->position += this->blocks.rbegin()->source_data_length;
                count++;
            }
            last_index = hit;
            if (this->window == Splitter::eos_magic) {
                bit_copy (
                    this->output_buffer,
                    this->bit_index,
                    this->bit_index + 32,
                    this->crc32_stream,
                    0ULL
                );
                break;
            }

        }
        window >>= 1;
        window += get_bit (this->output_buffer, bit_index) << 47;
    }
}

void Splitter::finishInput() {
    while (BZ2_bzCompress (this->stream, BZ_FINISH) != BZ_STREAM_END);
}

BZ2Block* Splitter::getData() {
    this->process();
    if (this->blocks.size() <= this->blocks_index) {
        return nullptr;
    }
    return &this->blocks[this->blocks_index++];
}

unsigned long long Splitter::magic_string_to_magic_int (unsigned char *str) {
    unsigned long long output = 0;
    for (int index = 0; index < 48; index++) {
        auto bit = get_bit (str, index);
        output >>= 1;
        output += bit << 47;
    }
    return output;
}

pair<unsigned char*, size_t> Splitter::get_bzip2_archive_from_block (
    unsigned char *stream,
    unsigned long long from,
    unsigned long long to)
{
    auto bits = 32 + to - from + 48 + 32;
    auto bytes = bits / 8 + ( ( (bits % 8) > 0) ? 1 : 0);
    unsigned char *out = new unsigned char[bytes];
    out[bytes - 1] = 0; // padding
    // Header
    memcpy (out, "BZh9", 4);
    unsigned long long copied = 32;
    // Data
    copied += bit_copy (stream, from, to, out, copied);
    // End of stream
    copied += bit_copy (Splitter::eos_magic_string, 0ULL, 48ULL, out, copied);
    // CRC 32
    copied += bit_copy (stream, from + 48ULL, from + 80ULL, out, copied);
    return make_pair (out, bytes);
}
