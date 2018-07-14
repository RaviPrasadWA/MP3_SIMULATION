from tables import (
    sfbtable_l,
    sfbtable_s,
    slen
)
from huffman_decode import process_main_data_huffman

def get_main_info(cur_frame, bitbfr):
    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):
            # Get scale factors
            part2_start = bitbfr.get_pos()
            if (cur_frame.side_info.window_switching_flag[ch][gr] == 1 and
                        cur_frame.side_info.block_type[ch][gr] == 2):
                if (cur_frame.side_info.mixed_block_flag[ch][gr]):
                    # mixed blocks
                    print "mixed scale blocks not supported yet"
                else:
                    # short blocks
                    for i in xrange(2):
                        for sfb in xrange(sfbtable_s[i], sfbtable_s[i + 1]):
                            for window in xrange(3):
                                cur_frame.main_data.scalefac_s[ch][gr][window][sfb] = bitbfr.read_bits(
                                    slen[i][cur_frame.side_info.scalefac_compress[ch][gr]])

                    sfb = 12
                    for window in xrange(3):
                        cur_frame.main_data.scalefac_s[ch][gr][window][sfb] = 0;

            else:
                # long blocks
                for i in xrange(4):
                    if ((cur_frame.side_info.scfsi[ch][i] == 0) or (gr == 0)):
                        for sfb in xrange(sfbtable_l[i], sfbtable_l[i + 1]):
                            if (i < 2):
                                k = 0
                            else:
                                k = 1
                            cur_frame.main_data.scalefac_l[ch][gr][sfb] = bitbfr.read_bits(
                                slen[k][cur_frame.side_info.scalefac_compress[ch][gr]])
                cur_frame.main_data.scalefac_l[ch][gr][22] = 0

            cur_frame, bitbfr = process_main_data_huffman(ch, gr, part2_start, cur_frame, bitbfr)
    return (cur_frame, bitbfr)
