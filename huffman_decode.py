from tables import (
    SSLIMIT,
    SBLIMIT,
    sfBandIndex_l
)
from huffman import ht_list


def process_main_data_huffman(ch, gr, part2_start, cur_frame, bitbfr):
    # calculate region boundries
    if (cur_frame.side_info.window_switching_flag[ch][gr] == 1 and cur_frame.side_info.block_type[ch][gr] == 2):
        # short block
        region1Start = 36;
        region2Start = 576;
    else:
        # long block
        region1Start = sfBandIndex_l[cur_frame.hdr.smpl_rate][cur_frame.side_info.region0_count[ch][gr] + 1]
        region2Start = sfBandIndex_l[cur_frame.hdr.smpl_rate][
            cur_frame.side_info.region0_count[ch][gr] + cur_frame.side_info.region1_count[ch][gr] + 2]

    # read big value area
    for i in xrange(0, cur_frame.side_info.big_values[ch][gr] * 2, 2):
        if (i < region1Start):
            ht_idx = cur_frame.side_info.table_select[ch][gr][0]
        elif (i < region2Start):
            ht_idx = cur_frame.side_info.table_select[ch][gr][1]
        else:
            ht_idx = cur_frame.side_info.table_select[ch][gr][2]
        ht = ht_list[ht_idx]
        v, w, x, y, bitbfr = huffman_decoder(ht, bitbfr)
        cur_frame.main_data.q_vals[gr][ch][i / SSLIMIT][i % SSLIMIT] = x
        cur_frame.main_data.q_vals[gr][ch][(i + 1) / SSLIMIT][(i + 1) % SSLIMIT] = y

    # read count1 area
    idx = cur_frame.side_info.big_values[ch][gr] * 2
    ht = ht_list[cur_frame.side_info.count1table_select[ch][gr] + 32]  # 32 is offset to count1 tables
    while (
                (bitbfr.get_pos() < (part2_start + cur_frame.side_info.part2_3_length[ch][gr])) and (
                        idx < SBLIMIT * SSLIMIT)):
        v, w, x, y, bitbfr = huffman_decoder(ht, bitbfr)
        cur_frame.main_data.q_vals[gr][ch][idx / SSLIMIT][idx % SSLIMIT] = v;
        cur_frame.main_data.q_vals[gr][ch][(idx + 1) / SSLIMIT][(idx + 1) % SSLIMIT] = w
        cur_frame.main_data.q_vals[gr][ch][(idx + 2) / SSLIMIT][(idx + 2) % SSLIMIT] = x
        cur_frame.main_data.q_vals[gr][ch][(idx + 3) / SSLIMIT][(idx + 3) % SSLIMIT] = y
        idx += 4
    return (cur_frame, bitbfr)


def huffman_decoder(ht, bitbfr):
    MXOFF = 250
    v = w = x = y = 0

    # check for empty tree
    if (ht.treelen == 0):
        return (0, 0, 0, 0)

    # run through huffman tree
    success = False
    pos = 0
    nbits = 0
    while (pos < ht.treelen and nbits < 32):
        # check for end of tree
        if (ht.values[pos][0] == 0):
            x = ht.values[pos][1] >> 4;
            y = ht.values[pos][1] & 0xf;
            success = True
            break

        # get more bits to transverse tree
        bit = bitbfr.read_bits(1)
        while (ht.values[pos][bit] >= MXOFF):
            pos += ht.values[pos][bit]
        pos += ht.values[pos][bit]

        nbits += 1

    if (not success):
        print "Failure during huffman decode"
        return (0, 0, 0, 0, bitbfr)

    # read sign bits
    if (ht.tbl_type == "quad"):
        v = (y >> 3) & 1;
        w = (y >> 2) & 1;
        x = (y >> 1) & 1;
        y = y & 1;

        if (v != 0):
            if (bitbfr.read_bits(1)):
                v = -v
        if (w != 0):
            if (bitbfr.read_bits(1)):
                w = -w
        if (x != 0):
            if (bitbfr.read_bits(1)):
                x = -x
        if (y != 0):
            if (bitbfr.read_bits(1)):
                y = -y
    else:
        # process escape encodings
        if (ht.linbits > 0):
            if ((ht.xlen - 1) == x):
                x += bitbfr.read_bits(ht.linbits)
        if (x != 0):
            if (bitbfr.read_bits(1)):
                x = -x

        if (ht.linbits > 0):
            if ((ht.ylen - 1) == y):
                y += bitbfr.read_bits(ht.linbits)
        if (y != 0):
            if (bitbfr.read_bits(1)):
                y = -y

    return (v, w, x, y, bitbfr)
