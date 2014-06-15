#include <map>
using namespace std;

#include "period.h"
#include "splitter.h"
#include "bit_tools.h"

const unsigned long long DATA_SIZE = 10 * 1024 * 1024;

void find_period (RepItem &data, vector<BZ2Block> &blocks) {
    Splitter splitter;
    unsigned long long total_bits_out = 0;
    blocks.resize (0);
    bool done = false;
    unsigned long long blocks_c = 0;
    while (1) {
        unsigned long long data_thresh = DATA_SIZE * 8;
        unsigned long long sent_data = 0;
        while (sent_data < data_thresh) {
            splitter.addData (data);
            sent_data += data.bit_len;
        }
        for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
            BZ2Block &block = *block_ptr;
            block_ptr = nullptr;
            blocks.push_back (block);
            blocks_c ++ ;
            total_bits_out += block.source_data_length * 8ULL;
            if (total_bits_out % data.bit_len == 0) {
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }

    }
}

bool find_period_with_offset (
    RepItem &data,
    unsigned long long offset_bit,
    unsigned long long count,
    vector<BZ2Block> &blocks,
    vector<BZ2Block> &period
) {
    Splitter splitter;
    unsigned long long total_bits_out = 0;
    blocks.resize (0);
    period.resize (0);
    bool done = false;
    bool found = false;
    unsigned long long blocks_c = 0;
    splitter.addData (data, offset_bit);
    unsigned long long maximum_bit_out = data.bit_len * count - offset_bit;
    unsigned long long cnt = 0;
    map<unsigned long long, unsigned int> psearch;
    while (cnt < count) {
        unsigned long long data_thresh = DATA_SIZE * 8;
        unsigned long long sent_data = 0;
        while ( (sent_data < data_thresh) && (cnt < count) ) {
            splitter.addData (data);
            sent_data += data.bit_len;
            cnt++;
        }
        for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
            BZ2Block &block = *block_ptr;
            block_ptr = nullptr;
            if (total_bits_out + 8ULL * block.source_data_length >= maximum_bit_out) {
                done = true;
                break;
            }
            blocks.push_back (block);
            blocks_c ++ ;
            total_bits_out += block.source_data_length * 8ULL;
            auto remainder = total_bits_out % data.bit_len;
            if (psearch.find (remainder) != psearch.end() ) {
                auto before = psearch[remainder];
                period.resize (before);
                copy (blocks.begin(), blocks.begin() + before, period.begin() );
                copy (blocks.begin() + before, blocks.end(), blocks.begin() );
                blocks.resize (blocks.size() - before);
                return true;
            }
            psearch[remainder] = blocks_c;
            if (remainder == 0) {
                done = true;
                found = true;
                break;
            }
        }
        if (done) {
            break;
        }

    }
    return found;
}

RepItem bz2blocks_to_repitem (vector<BZ2Block> &data) {
    RepItem item;
    unsigned long long output_len_bits = 0;
    for (auto & b : data) {
        output_len_bits += b.raw_data_bits;
    }
    auto output_len_bytes = output_len_bits / 8ULL;
    if (output_len_bits % 8 != 0) {
        output_len_bytes += 1;
    }
    item.data = new unsigned char[output_len_bytes];
    auto written_bits = 0ULL;
    for (auto & b : data) {
        auto prev = written_bits;
        written_bits += bit_copy (
                            b.raw_data,
                            0ULL,
                            b.raw_data_bits,
                            item.data,
                            written_bits
                        );
        BZ2Block block (item.data, prev, written_bits, 0);
        item.crc32.push_back (b.crc32);
    }
    item.bit_len = output_len_bits;
    item.byte_len = output_len_bytes;
    return item;
}


RepData compress (RepData data) {
    RepData output;
    vector<BZ2Block> X_period;
    find_period (data.X.first, X_period);
    unsigned long long source_len_bits = 0;
    for (auto & b : X_period) {
        source_len_bits += b.source_data_length;
    }
    source_len_bits *= 8ULL;
    auto inputs_in_period = source_len_bits / data.X.first.bit_len;
    auto X_count = data.X.second / inputs_in_period;
    if (inputs_in_period > data.X.second) {
        X_count = 0;
    }
    output.X.second = X_count;
    output.X.first = bz2blocks_to_repitem (X_period);
    vector<BZ2Block> Y_chunk;
    Splitter splitter;
    auto Y_output_prefix = 0ULL;
    for (auto i = 0LLU; i < data.X.second % inputs_in_period; i++) {
        splitter.addData (data.X.first);
        Y_output_prefix += data.X.first.bit_len;
    }
    splitter.addData (data.Y.first);
    Y_output_prefix += data.Y.first.bit_len;
    auto total_bits_out = 0ULL;
    bool done = false;
    while (1) {
        for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
            BZ2Block &block = *block_ptr;
            block_ptr = nullptr;
            Y_chunk.push_back (block);
            total_bits_out += block.source_data_length * 8ULL;
            auto additional_data = total_bits_out - Y_output_prefix;
            if (
                (total_bits_out >= Y_output_prefix) &&
                (additional_data % data.X.first.bit_len == 0)
            ) {
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
        unsigned long long data_thresh = DATA_SIZE * 8;
        unsigned long long sent_data = 0;
        while (sent_data < data_thresh) {
            splitter.addData (data.X.first);
            sent_data += data.X.first.bit_len;
        }
    }
    output.Y.second = 1;
    output.Y.first = bz2blocks_to_repitem (Y_chunk);
    auto X_in_Y_count = (total_bits_out - Y_output_prefix) / data.X.first.bit_len;
    // Compute Y2 count
    output.X2_count = (data.X2_count - X_in_Y_count) / inputs_in_period;
    splitter.reset();
    for (auto i = 0ULL; i < (output.X2_count - X_in_Y_count) % inputs_in_period; i++) {
        splitter.addData (data.X.first);
    }
    splitter.addData (data.Z.first);
    splitter.finishInput();
    vector<BZ2Block> Z_chunk;
    for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
        BZ2Block &block = *block_ptr;
        block_ptr = nullptr;
        Z_chunk.push_back (block);
    }

    output.Z.second = 1;
    output.Z.first = bz2blocks_to_repitem (Z_chunk);
    return output;

}

RepData compress_items (RepData data) {
    RepData output;
    unsigned long long input_offset_bit = 0;
    for (unsigned int i = 0; i < data.items.size(); i++) {
        vector<BZ2Block> blocks;
        vector<BZ2Block> period;
        auto found =
            find_period_with_offset (
                data.items[i].first,
                input_offset_bit,
                data.items[i].second,
                blocks,
                period
            );
        if (period.size() > 0) {
            auto period_data = 0ULL;
            for (auto & block : period) {
                period_data += 8ULL * block.source_data_length;
            }
            period_data += input_offset_bit;
            data.items[i].second -= period_data / data.items[i].first.bit_len;
            input_offset_bit = period_data % data.items[i].first.bit_len;

            output.items.push_back (
                make_pair (
                    bz2blocks_to_repitem (period),
                    1ULL
                )
            );
        }

        if (found) {
            unsigned long long source_len_bits = 0;
            for (auto & b : blocks) {
                source_len_bits += b.source_data_length;
            }
            source_len_bits *= 8ULL;
            auto inputs_in_period = source_len_bits / data.items[i].first.bit_len;
            auto X_count = data.items[i].second / inputs_in_period;
            if (
                (data.items[i].second % inputs_in_period == 0) &&
                (input_offset_bit > 0)
            ) {
                X_count--;
            }
            data.items[i].second -= inputs_in_period * X_count;
            output.items.push_back (
                make_pair (
                    bz2blocks_to_repitem (blocks),
                    X_count
                )
            );
            blocks.resize (0);
        }
        auto packed_data = 0ULL;
        for (auto & block : blocks) {
            packed_data += 8ULL * block.source_data_length;
        }
        packed_data += input_offset_bit;
        data.items[i].second -= packed_data / data.items[i].first.bit_len;
        input_offset_bit = packed_data % data.items[i].first.bit_len;

        Splitter splitter;
        auto data_to_recover = 0ULL;
        if (data.items[i].second > 0) {
            splitter.addData (data.items[i].first, input_offset_bit);
            data_to_recover += data.items[i].first.bit_len - input_offset_bit;
        }
        input_offset_bit = 0;
        data.items[i].second;
        for (auto j = 1ULL; j < data.items[i].second; j++) {
            splitter.addData (data.items[i].first);
            data_to_recover += data.items[i].first.bit_len;
        }
        auto total_bits_out = 0ULL;
        bool done = false;
        bool finished = false;
        unsigned int next_index = i + 1;
        unsigned long long next_count = 0;
        while (finished || total_bits_out < data_to_recover) {
            for (BZ2Block *block_ptr = splitter.getData(); block_ptr != nullptr; block_ptr = splitter.getData() ) {
                BZ2Block &block = *block_ptr;
                block_ptr = nullptr;
                blocks.push_back (block);
                total_bits_out += block.source_data_length * 8ULL;
                if (total_bits_out >= data_to_recover) {
                    done = true;
                    break;
                }
            }
            if (finished) {
                break;
            }
            if (done) {
                break;
            }

            unsigned long long data_thresh = DATA_SIZE * 8ULL;
            unsigned long long sent_data = 0;
            if (next_index >= data.items.size() ) {
                splitter.finishInput();
                finished = true;
                continue;
            }
            while (sent_data < data_thresh) {
                if (next_count >= data.items[next_index].second) {
                    next_count = 0;
                    next_index ++;
                }
                if (next_index >= data.items.size() ) {
                    splitter.finishInput();
                    finished = true;
                    break;
                }
                splitter.addData (data.items[next_index].first);
                sent_data += data.items[next_index].first.bit_len;
                next_count++;
            }
        }
        if (blocks.size() > 0) {
            output.items.push_back (
                make_pair (
                    bz2blocks_to_repitem (blocks),
                    1ULL
                )
            );
        }
        if (finished) {
            break;
        }
        auto excess_bits = total_bits_out - data_to_recover;
        next_index = i + 1;
        next_count = 0;
        input_offset_bit = 0;
        while (excess_bits > 0) {
            if (next_count > data.items[next_index].second) {
                next_index += 1;
                next_count = 0;
            }
            if (excess_bits >= data.items[next_index].first.bit_len) {
                excess_bits -= data.items[next_index].first.bit_len;
                data.items[next_index].second--;
            } else {
                input_offset_bit = excess_bits;
                excess_bits = 0;
            }
            next_count++;
        }
        blocks.resize (0);
    }
    return output;
}
