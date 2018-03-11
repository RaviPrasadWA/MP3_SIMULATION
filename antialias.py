from numpy import sqrt
from tables import SBLIMIT, SSLIMIT

def antialias_samples(cur_frame):
    cs = [ 1.0/sqrt(1.0 + ci*ci) for ci in antialias_Ci]
    ca = [  ci/sqrt(1.0 + ci*ci) for ci in antialias_Ci]
    ro   = cur_frame.decoded_data.reordered_vals
    aa   = cur_frame.decoded_data.antialiased_vals               

    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):          
            for sb in xrange(SBLIMIT):
                for ss in xrange(SSLIMIT): 
                    aa[gr][ch][sb][ss] = ro[gr][ch][sb][ss]
            if  ((cur_frame.side_info.window_switching_flag[ch][gr] and (cur_frame.side_info.block_type[ch][gr] == 2)) and (not cur_frame.side_info.mixed_block_flag[ch][gr])):
                continue
            if ( cur_frame.side_info.window_switching_flag[ch][gr] and cur_frame.side_info.mixed_block_flag[ch][gr] and (cur_frame.side_info.block_type[ch][gr] == 2)):
                sblim = 1
            else:
                sblim = SBLIMIT-1

            for sb in xrange(sblim):   
                for ss in xrange(8):       
                    bu = ro[gr][ch][sb][17-ss];
                    bd = ro[gr][ch][sb+1][ss];
                    aa[gr][ch][sb][17-ss] = (bu * cs[ss]) - (bd * ca[ss])
                    aa[gr][ch][sb+1][ss]  = (bd * cs[ss]) + (bu * ca[ss])