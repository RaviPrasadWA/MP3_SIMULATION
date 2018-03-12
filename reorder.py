from tables import SSLIMIT, SBLIMIT

def reorder_samples(cur_frame):
    smpl_rate = cur_frame.hdr.smpl_rate
    lr   = cur_frame.decoded_data.lr_vals
    ro   = cur_frame.decoded_data.reordered_vals

    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):
            if (cur_frame.side_info.window_switching_flag[ch][gr] and (cur_frame.side_info.block_type[ch][gr] == 2)):
                if (cur_frame.side_info.mixed_block_flag[ch][gr]):
                    for sb in xrange(2):
                        for ss in xrange(SSLIMIT):
                            ro[gr][ch][sb][ss] = lr[gr][ch][sb][ss];

                    sfb_start = sfBandIndex_s[smpl_rate][3]
                    sfb_lines = sfBandIndex_s[smpl_rate][4] - sfb_start

                    for sfb in xrange(3,13):
                        for window in xrange(3):
                            for freq in xrange(sfb_lines):
                                src_line = sfb_start*3 + window*sfb_lines + freq 
                                des_line = (sfb_start*3) + window + (freq*3)
                                ro[gr][ch][des_line/SSLIMIT][des_line%SSLIMIT] = lr[gr][ch][src_line/SSLIMIT][src_line%SSLIMIT]
                    
                        sfb_start = sfBandIndex_s[smpl_rate][sfb]
                        sfb_lines = sfBandIndex_s[smpl_rate][sfb+1] - sfb_start
                               
                       
                    else: 
                        #pure short                
                        sfb_start=0
                        sfb_lines=sfBandIndex_s[smpl_rate][1] 
                        for sfb in xrange(13): 
                            for window in xrange(3):
                                for freq in xrange(sfb_lines):
                                    src_line = sfb_start*3 + window*sfb_lines + freq 
                                    des_line = (sfb_start*3) + window + (freq*3)
                                    ro[gr][ch][des_line/SSLIMIT][des_line%SSLIMIT] = lr[gr][ch][src_line/SSLIMIT][src_line%SSLIMIT]

                            sfb_start=sfBandIndex_s[smpl_rate][sfb]
                            sfb_lines=sfBandIndex_s[smpl_rate][sfb+1] - sfb_start
                else:
                    for sb in xrange(SBLIMIT):
                        for ss in xrange(SSLIMIT): 
                            ro[gr][ch][sb][ss] = lr[gr][ch][sb][ss]
            else:
                for sb in xrange(SBLIMIT):
                    for ss in xrange(SSLIMIT):
                        ro[gr][ch][sb][ss] = lr[gr][ch][sb][ss]