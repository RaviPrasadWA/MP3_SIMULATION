from numpy import zeros, cos, double
from tables import SSLIMIT, SBLIMIT

def inv_mdct(data_in, blk_type):
    out = zeros( (36), dtype=double)
    if(blk_type == 2):
        tmp = zeros( (12), dtype=double)
        N=12
        for i in xrange(3):
            for p in xrange(N):             
                total = 0.0
                for m in xrange(N/2):
                    total += data_in[i+3*m] * cos( PI/(2*N)*(2*p+1+N/2)*(2*m+1) )
                tmp[p] = total * self.mdct_win[blk_type][p]               
            for p in xrange(N):
                out[6*i+p+6] += tmp[p]
    else:
        cos_tbl = self.mdct_cos_tbl_2
        win     = self.mdct_win[blk_type]
        for p in xrange(36):
            out[p] = sum(data_in * cos_tbl[p]) * win[p] 
    return out


def hybrid_synthesis(cur_frame, hybrid_prevblck):
    data_in  = cur_frame.decoded_data.antialiased_vals
    data_out = cur_frame.decoded_data.hybrid_vals

    for gr in xrange(2):
        for ch in xrange(cur_frame.hdr.n_channels):        
            for sb in xrange(SBLIMIT):
                if(cur_frame.side_info.window_switching_flag[ch][gr] and cur_frame.side_info.mixed_block_flag[ch][gr] and sb<2):
                    blk_type = 0
                else:
                    blk_type = cur_frame.side_info.block_type[ch][gr]
                rawout = inv_mdct( data_in[gr][ch][sb], blk_type)
                for ss in xrange(SSLIMIT):       
                    data_out[gr][ch][sb][ss] = rawout[ss] + hybrid_prevblck[ch][sb][ss]
                    hybrid_prevblck[ch][sb][ss] = rawout[ss+18]


