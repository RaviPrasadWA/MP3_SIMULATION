from numpy import zeros, roll, int32

def subband_synthesis(sbs_buf, sbs_filter, data_in, ch):
    pcm = zeros( (32), dtype=int32)        
    sbs_buf[ch] = roll(sbs_buf[ch], 64)
    bfr = sbs_buf[ch]

    for i in xrange(64):
        bfr[i] = sum(data_in * sbs_filter[i])            

    for j in xrange(32):
        total = 0
        for i in xrange(16):
            k = j + i*32
            idx = (k + ( ((i+1)>>1) * 64) )
            total += sbs_window[k] * bfr[idx]                              
        pcm[j] = min(max(total * SCALE, -SCALE), SCALE-1)            
    return pcm