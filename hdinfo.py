from tables  import (   
                        HEADER_VERSION_TABLE,
                        HEADER_LAYER_TABLE,
                        HEADER_BITRATE_TABLE,
                        HEADER_SAMPLE_RATE_TABLE,
                        HEADER_CHANNEL_MODE_TABLE,
                        HEADER_EMPHASIS_TABLE
                    )

def decode_frame_header(cur_frame, bitbfr, first_frame):
  
    cur_frame.hdr.mpeg_version   = HEADER_VERSION_TABLE[bitbfr.read_bits(1)]  
    cur_frame.hdr.layer          = HEADER_LAYER_TABLE[bitbfr.read_bits(2)]
    cur_frame.hdr.has_CRC        = not bitbfr.read_bits(1)
    cur_frame.hdr.bitrate        = HEADER_BITRATE_TABLE[bitbfr.read_bits(4)]
    cur_frame.hdr.smpl_rate      = HEADER_SAMPLE_RATE_TABLE[bitbfr.read_bits(2)]
    cur_frame.hdr.padding        = bitbfr.read_bits(1)
    bitbfr.read_bits(1)          #private bit
    cur_frame.hdr.channel_mode   = HEADER_CHANNEL_MODE_TABLE[bitbfr.read_bits(2)]
    cur_frame.hdr.mode_extention = bitbfr.read_bits(2)
    cur_frame.hdr.copyrighted    = bitbfr.read_bits(1)
    cur_frame.hdr.original       = bitbfr.read_bits(1)  
    cur_frame.hdr.emphasis       = HEADER_EMPHASIS_TABLE[bitbfr.read_bits(2)]  

    if(cur_frame.hdr.channel_mode == "mono"):         
        cur_frame.hdr.n_channels = 1
    else:
        cur_frame.hdr.n_channels = 2
    
    if ( first_frame ):
        return False 