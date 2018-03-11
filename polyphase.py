from subband import subband_synthesis
from numpy import double
from tables import SBLIMIT

def polyphase_synthesis(sbs_buf, sbs_filter, cur_frame, pcm):
    data_in  = cur_frame.decoded_data.hybrid_vals  
    data_out = pcm
    bfr = zeros( (SBLIMIT), dtype=double)

    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):
            for ss in xrange(18):
                for sb in xrange(SBLIMIT):
                    if ((ss%2) and (sb%2)):
                        cur_frame.decoded_data.hybrid_vals[gr][ch][sb][ss] = -cur_frame.decoded_data.hybrid_vals[gr][ch][sb][ss]        
            for ss in xrange(18):
                for sb in xrange(SBLIMIT):
                    bfr[sb] = data_in[gr][ch][sb][ss]
                pcm_bfr = subband_synthesis(sbs_buf, sbs_filter, bfr, ch)
                data_out[ch].extend(pcm_bfr)