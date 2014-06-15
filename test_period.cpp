#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <ctime>

#include "period.h"
#include "bit_tools.h"
#include "splitter.h"

void print_data (RepData &data) {
    unsigned long long buf_size = 10 * 1024 * 1024;
    unsigned long long buf_size_bits = buf_size * 8;
    auto output = new unsigned char[buf_size];
    write (1, "BZh9", 4);
    unsigned long long written = 0;
    unsigned int crc32 = 0U;
    time_t start = time (nullptr);
    fprintf (stderr, "Computing start: %ld\n", start);
    for (unsigned long long i = 0; i < data.X.second; i++) {
        if (written + data.X.first.bit_len > buf_size_bits) {
            write (1, output, written / 8);
            output[0] = output[written / 8];
            written %= 8;
        }
        written += bit_copy (
                       data.X.first.data,
                       0ULL,
                       data.X.first.bit_len,
                       output,
                       written
                   );
        for (auto & c : data.X.first.crc32) {
            crc32 = bz2_crc32_combine (crc32, c);
        }
    }
    write (1, output, written / 8);
    output[0] = output[written / 8];
    written %= 8;
    for (unsigned long long i = 0; i < data.Y.second; i++) {
        if (written + data.Y.first.bit_len > buf_size_bits) {
            write (1, output, written / 8);
            output[0] = output[written / 8];
            written %= 8;
        }
        written += bit_copy (
                       data.Y.first.data,
                       0ULL,
                       data.Y.first.bit_len,
                       output,
                       written
                   );
        for (auto & c : data.Y.first.crc32) {
            crc32 = bz2_crc32_combine (crc32, c);
        }

    }
    write (1, output, written / 8);
    output[0] = output[written / 8];
    written %= 8;
    for (unsigned long long i = 0; i < data.X2_count; i++) {
        if (written + data.X.first.bit_len > buf_size_bits) {
            write (1, output, written / 8);
            output[0] = output[written / 8];
            written %= 8;
        }
        written += bit_copy (
                       data.X.first.data,
                       0ULL,
                       data.X.first.bit_len,
                       output,
                       written
                   );
        for (auto & c : data.X.first.crc32) {
            crc32 = bz2_crc32_combine (crc32, c);
        }
    }
    write (1, output, written / 8);
    output[0] = output[written / 8];
    written %= 8;
    for (unsigned long long i = 0; i < data.Z.second; i++) {
        if (written + data.Z.first.bit_len > buf_size_bits) {
            write (1, output, written / 8);
            output[0] = output[written / 8];
            written %= 8;
        }
        written += bit_copy (
                       data.Z.first.data,
                       0ULL,
                       data.Z.first.bit_len,
                       output,
                       written
                   );
        for (auto & c : data.Z.first.crc32) {
            crc32 = bz2_crc32_combine (crc32, c);
        }
    }
    write (1, output, written / 8);
    output[0] = output[written / 8];
    written %= 8;
    written += bit_copy (
                   Splitter::eos_magic_string,
                   0ULL,
                   48ULL,
                   output,
                   written
               );
    fprintf (stderr, "crc32: 0x%08x\n", crc32);
    written += bit_copy (
                   crc32_to_crc32_string (crc32),
                   0ULL,
                   32ULL,
                   output,
                   written
               );
    bit_copy ( (unsigned char*) "\00", 0ULL, 8ULL, output, written);
    write (1, output, written / 8);
    output[0] = output[written / 8];
    written %= 8;
    if (written != 0) {
        write (1, output, 1);
    }
    fprintf (stderr, "COMPUTING CRC32 TOOK: %ld seconds\n", time (nullptr) - start);
}

int main () {
    unsigned long long base_size = 1024ULL * 1024 * 1024 * 1024;
    RepData data;
    data.X.first.data = (unsigned char*) "\00\00\00\00\00\00\00\00\00";
    data.X.first.byte_len = 9;// strlen ( (char*) data.X.first.data);
    data.X.first.data = (unsigned char*) "ipsc2014\n";
    data.X.first.byte_len = strlen ( (char*) data.X.first.data);
    data.X.first.bit_len = 8 * data.X.first.byte_len;
    data.X.second =  base_size * 1024 * 78;
    data.Y.first.data = (unsigned char*) "badger badger\n";
    data.Y.first.byte_len = strlen ( (char*) data.Y.first.data);
    data.Y.first.bit_len = 8 * data.Y.first.byte_len;
    data.Y.second = 1;
    data.Z.first.data = (unsigned char*) "mushroom\n";
    data.Z.first.byte_len = strlen ( (char*) data.Z.first.data);
    data.Z.first.bit_len = 8 * data.Z.first.byte_len;
    data.Z.second = 1;
    data.X2_count = base_size * 512 * 78;
    for (int i = 0; i < 1; i++) {
        unsigned long long data_before = (data.X.second + data.X2_count) * data.X.first.bit_len + data.Y.second * data.Y.first.bit_len + data.Z.second * data.Z.first.bit_len;
        fprintf (stderr, "X: %llu %llu x %llu\n", data.X.first.bit_len, data.X.first.byte_len, data.X.second);
        fprintf (stderr, "Y: %llu %llu x %llu\n", data.Y.first.bit_len, data.Y.first.byte_len, data.Y.second);
        fprintf (stderr, "X: %llu %llu x %llu\n", data.X.first.bit_len, data.X.first.byte_len, data.X2_count);
        fprintf (stderr, "Z: %llu %llu x %llu\n", data.Z.first.bit_len, data.Z.first.byte_len, data.Z.second);
        data = compress (data);
        fprintf (stderr, "X: %llu %llu x %llu\n", data.X.first.bit_len, data.X.first.byte_len, data.X.second);
        fprintf (stderr, "Y: %llu %llu x %llu\n", data.Y.first.bit_len, data.Y.first.byte_len, data.Y.second);
        fprintf (stderr, "X: %llu %llu x %llu\n", data.X.first.bit_len, data.X.first.byte_len, data.X2_count);
        fprintf (stderr, "Z: %llu %llu x %llu\n", data.Z.first.bit_len, data.Z.first.byte_len, data.Z.second);
        unsigned long long data_now = (data.X.second + data.X2_count) * data.X.first.bit_len + data.Y.second * data.Y.first.bit_len + data.Z.second * data.Z.first.bit_len;
        fprintf (stderr, "%llu %llu\n%llu %llu\n", data_before, data_now, data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu KB -> %llu KB\n", data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu MB -> %llu MB\n", data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu GB -> %llu GB\n", data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu TB -> %llu TB\n", data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu PB -> %llu PB\n", data_before / 8, data_now / 8);
        data_before /= 1024;
        data_now /= 1024;
        fprintf (stderr, "%llu EB -> %llu EB\n", data_before / 8, data_now / 8);
    }
    print_data (data);
}
