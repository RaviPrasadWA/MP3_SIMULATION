def get_side_info(cur_frame, bitbfr):
    cur_frame.side_info.main_data_begin = bitbfr.read_bits(9)

    if (cur_frame.hdr.n_channels == 1):
        cur_frame.side_info.private_bits = bitbfr.read_bits(5)
    else:
        cur_frame.side_info.private_bits = bitbfr.read_bits(3)

    for ch in xrange(cur_frame.hdr.n_channels):
        for i in xrange(4):
            cur_frame.side_info.scfsi[ch][i] = bitbfr.read_bits(1)

    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):
            cur_frame.side_info.part2_3_length[ch][gr] = bitbfr.read_bits(12)
            cur_frame.side_info.big_values[ch][gr] = bitbfr.read_bits(9)
            cur_frame.side_info.global_gain[ch][gr] = bitbfr.read_bits(8)
            cur_frame.side_info.scalefac_compress[ch][gr] = bitbfr.read_bits(4)
            cur_frame.side_info.window_switching_flag[ch][gr] = bitbfr.read_bits(1)

            if (cur_frame.side_info.window_switching_flag[ch][gr]):
                cur_frame.side_info.block_type[ch][gr] = bitbfr.read_bits(2)
                cur_frame.side_info.mixed_block_flag[ch][gr] = bitbfr.read_bits(1)

                for i in xrange(2):
                    cur_frame.side_info.table_select[ch][gr][i] = bitbfr.read_bits(5)

                for i in xrange(3):
                    cur_frame.side_info.subblock_gain[ch][gr][i] = bitbfr.read_bits(3)

                if (cur_frame.side_info.block_type[ch][gr] == 2 and cur_frame.side_info.mixed_block_flag[ch][gr] == 0):
                    cur_frame.side_info.region0_count[ch][gr] = 8
                else:
                    cur_frame.side_info.region0_count[ch][gr] = 7
                cur_frame.side_info.region1_count[ch][gr] = 20 - cur_frame.side_info.region0_count[ch][gr]
            else:
                for i in xrange(3):
                    cur_frame.side_info.table_select[ch][gr][i] = bitbfr.read_bits(5)
                cur_frame.side_info.region0_count[ch][gr] = bitbfr.read_bits(4)
                cur_frame.side_info.region1_count[ch][gr] = bitbfr.read_bits(3)
                cur_frame.side_info.block_type[ch][gr] = 0
            cur_frame.side_info.preflag[ch][gr] = bitbfr.read_bits(1)
            cur_frame.side_info.scalefac_scale[ch][gr] = bitbfr.read_bits(1)
            cur_frame.side_info.count1table_select[ch][gr] = bitbfr.read_bits(1)
    return (cur_frame, bitbfr)