from tables import INTENSITY_STEREO_BIT, MID_SIDE_STEREO_BIT, SBLIMIT, SSLIMIT

def process_stereo(cur_frame):
    has_intensity_stereo = (cur_frame.hdr.mode_extention & INTENSITY_STEREO_BIT)
    has_mid_side_stereo = (cur_frame.hdr.mode_extention & MID_SIDE_STEREO_BIT)
    lr      = cur_frame.decoded_data.lr_vals
    dq_vals = cur_frame.decoded_data.dq_vals

    for gr in xrange(2):
        if (cur_frame.hdr.n_channels==2): 
            is_pos = [7 for dummy in xrange(576)]
            if(has_intensity_stereo):
                is_ratio = process_intensity_stereo(cur_frame, gr, is_pos)

                for sb in xrange(SBLIMIT):
                    for ss in xrange(SSLIMIT):
                        i = (sb*18)+ss
                        if ( is_pos[i] == 7 ): 
                            if (has_mid_side_stereo ): 
                                lr[gr][0][sb][ss] = (dq_vals[gr][0][sb][ss]+dq_vals[gr][1][sb][ss])/1.41421356
                                lr[gr][1][sb][ss] = (dq_vals[gr][0][sb][ss]-dq_vals[gr][1][sb][ss])/1.41421356
                            else: 
                                lr[gr][0][sb][ss] = dq_vals[gr][0][sb][ss]
                                lr[gr][1][sb][ss] = dq_vals[gr][1][sb][ss]
                        elif (has_intensity_stereo ): 
                            lr[gr][0][sb][ss] = dq_vals[gr][0][sb][ss] * (is_ratio[i]/(1+is_ratio[i]))
                            lr[gr][1][sb][ss] = dq_vals[gr][0][sb][ss] * (1/(1+is_ratio[i])) 
                        else: 
                            print "[x]: Error in stereo"                                                   
                     
        else:
            for sb in xrange(SBLIMIT):
                for ss in xrange(SSLIMIT): 
                    lr[gr][0][sb][ss] = dq_vals[gr][0][sb][ss]

def process_intensity_stereo(cur_frame, gr, is_pos):
    ch = 0  
    if (cur_frame.side_info.window_switching_flag[ch][gr] and (cur_frame.side_info.block_type[ch][gr] == 2)):
        is_ratio = process_intensity_stereo__non_standard_window(cur_frame, gr, is_pos)
    else:
        is_ratio = process_intensity_stereo__standard_window(cur_frame, gr, is_pos)
    return is_ratio


def process_intensity_stereo__standard_window(cur_frame, gr, is_pos):
    smpl_rate = cur_frame.hdr.smpl_rate        
    i  = 31
    ss = 17
    sb = 0;

    while ( i >= 0 ):
        if ( self.decoded_data.dq_vals[1][i][ss] != 0.0 ):
            sb = i*18+ss
            i  = -1
        else:
            ss-=1
            if ( ss < 0 ):
                i -= 1
                ss = 17
    i = 0

    while ( sfBandIndex_l[smpl_rate][i] <= sb ):
        i += 1

    sfb = i
    i = sfBandIndex_l[smpl_rate][i]
    for sfb in xrange(sfb,21):
        sb = sfBandIndex_l[smpl_rate][sfb+1] - sfBandIndex_l[smpl_rate][sfb]
        for sb in xrange(sb, 0, -1):
            is_pos[i] =  cur_frame.main_data.scalefac_l[1][gr][sfb]
            if ( is_pos[i] != 7 ):
                is_ratio[i] = tan( is_pos[i] * (PI / 12))
            i+=1

    sfb = sfBandIndex_l[smpl_rate][20]
    for sb in xrange(576 - sfBandIndex_l[smpl_rate][21],0,-1):
        is_pos[i]   = is_pos[sfb]
        is_ratio[i] = is_ratio[sfb]
        i += 1
    return is_ratio

def process_intensity_stereo__non_standard_window():
    pass