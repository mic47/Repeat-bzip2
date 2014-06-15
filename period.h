#ifndef PERIOD_H
#define PERIOD_H

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

struct RepItem {
    unsigned char *data;
    unsigned long long bit_len;
    unsigned long long byte_len; // byte_len * 8>=bit_len
    vector<unsigned int> crc32;
};

struct RepData {
    pair<RepItem, unsigned long long> X, Y, Z;
    unsigned long long X2_count;
    vector<pair<RepItem, unsigned long long>> items;
};


RepData compress(RepData data);

RepData compress_items(RepData data);

#endif
