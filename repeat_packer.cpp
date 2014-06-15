#include <string>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <memory>
#include <cassert>
using namespace std;

#include "splitter.h"
#include "bit_tools.h"
#include "period.h"

RepData load_data (istream &in) {
    RepData data;
    unsigned long long count;
    unsigned int tab[256];
    memset (tab, 0, 256 * sizeof (unsigned int) );
    for (int i = '0'; i <= '9'; i++) tab[i] = i - '0';
    for (int i = 'A'; i <= 'F'; i++) tab[i] = i - 'A' + 10;
    for (int i = 'a'; i <= 'f'; i++) tab[i] = i - 'a' + 10;
    while (true) {
        unsigned long long bit_len, byte_len;
        in >> count >> bit_len >> byte_len;
        if (count == 0) {
            break;
        }
        string format, text;
        in >> format;
        getline (in, text);
        getline (in, text);
        if (format == "hex") {
            for (unsigned int i = 0; i < text.size(); i += 2) {
                text[i / 2] = (tab[ (unsigned char) text[i]] << 4) + tab[ (unsigned char) text[i + 1]];
            }
            text.resize (text.size() / 2);
        }
        RepItem item;
        if (bit_len == 0) {
            bit_len = 8ULL * text.size();
        } else if ( (bit_len / 8 < text.size() - 1) || (bit_len > 8ULL * text.size() ) ) {
            cerr << text.size() << " " << text << endl;
            fprintf (stderr, "bit_len is wrong!\n");
            exit (1);
        }
        if (byte_len == 0) {
            byte_len = text.size();
        } else if (byte_len != text.size() ) {
            fprintf (stderr, "byte_len is wrong!\n");
            exit (1);
        }
        item.bit_len = bit_len; 
        item.byte_len = byte_len;
        item.data = new unsigned char[text.size()];
        memcpy (item.data, text.c_str(), text.size() );
        data.items.push_back (make_pair (item, count) );
    }
    return data;
}

void save_data (ostream &out, RepData &data) {
    char tab[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (auto & item : data.items) {
        out << item.second << " "
            << item.first.bit_len << " "
            << item.first.byte_len << " " << endl
            << "hex" << endl;
        for (unsigned long long i = 0; i < item.first.byte_len; i++) {
            out << tab[item.first.data[i] >> 4] << tab[item.first.data[i] % 16];
        }
        out << endl;

    }
    out << 0 << endl;
}

void export_data (int fd, RepData &data) {

    unsigned long long buf_size = 10 * 1024 * 1024;
    unsigned long long buf_size_bits = buf_size * 8;
    auto output = new unsigned char[buf_size];
    unsigned long long written = 0;
    for (auto & item : data.items) {
        for (unsigned long long i = 0; i < item.second; i++) {
            if (written + item.first.bit_len >= buf_size_bits) {
                unsigned long long wr = 0;
                wr = write (fd, output, written / 8);
                fprintf(stderr, "written now: %lld\n", wr);
                assert (wr == written / 8);
                output[0] = output[written / 8];
                written %= 8;
            }
            assert (written + item.first.bit_len < buf_size_bits);
            auto ret = bit_copy (
                           item.first.data,
                           0ULL,
                           item.first.bit_len,
                           output,
                           written
                       );
            written += ret;

        }
    }
    unsigned long long wr = 0;
    wr = write (fd, output, written / 8);
    assert (wr == written / 8);
    output[0] = output[written / 8];
    written %= 8;
    // Write padding (we do not want there random stuff there - no particular reason)
    bit_copy ( (unsigned char*) "\00", 0ULL, 8ULL, output, written);
    if (written != 0) {
        wr = write (fd, output, 1);
    }
    delete output;
}

void make_bzip2_archive (RepData &data) {
    unsigned int crc32 = 0U;
    int cnt = 0;
    int it = 0;
    for (auto & item : data.items) {
        unsigned long long divide = 64 * item.first.crc32.size();
        unsigned long long count = item.second;
        if (divide > 0) {
            count %= (64 * item.first.crc32.size());
        }
        for (unsigned int i = 0; i < count; i++) {
            int c = 0;
            for (auto & crc : item.first.crc32) {
                crc32 = bz2_crc32_combine (crc32, crc);
                cnt++;
                c++;

            }
        }
        it++;
    }
    RepItem item;
    item.data = new unsigned char[4];
    memcpy (item.data, "BZh9", 4);
    item.bit_len = 32;
    item.byte_len = 4;
    data.items.insert (data.items.begin(), make_pair (item, 1ULL) );

    item.data = new unsigned char[10];
    memcpy (item.data, Splitter::eos_magic_string, 6);
    memcpy (item.data + 6, crc32_to_crc32_string (crc32), 4);
    item.bit_len = 80;
    item.byte_len = 10;
    data.items.push_back (make_pair (item, 1ULL) );
}

unsigned long long get_size (RepData &data) {
    unsigned long long acc = 0;
    for (auto & d : data.items) {
        acc += d.second * d.first.bit_len;
    }
    return acc;
}

int main (int, char **argv) {
    vector<string> args;
    bool binary_export = false;
    bool pack = true;
    for (argv += 1; *argv != nullptr; argv++) {
        string a (*argv);
        if (a == "--export") {
            binary_export = true;
        } else if (a == "--nopack") {
            pack = false;
        } else {
            args.push_back (string (*argv) );
        }
    }
    if (args.size() < 2) {
        cerr << "Not enough arguments" << endl;
        exit (1);
    }
    string input_file = args[0];
    string output_file = args[1];
    RepData data;
    if (input_file == "-") {
        data = load_data (cin);
    } else {
        ifstream in (input_file.c_str() );
        data = load_data (in);
        in.close();
    }
    if (pack) {
        auto data_before = get_size (data);
        data = compress_items (data);
        make_bzip2_archive (data);
        auto data_now = get_size (data);
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
    if (binary_export) {
        if (output_file == "-") {
            export_data (1, data);
        } else {
            int fd = open (output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            export_data (fd, data);
            close (fd);
        }
    } else {
        if (output_file == "-") {
            save_data (cout, data);
        } else {
            ofstream out (output_file.c_str() );
            save_data (out, data);
            out.close();
        }
    }
}
