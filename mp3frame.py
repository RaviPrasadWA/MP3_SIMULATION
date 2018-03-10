from numpy import zeros, int32, double

class HEADER:

    def __init__(self):
        self.mpeg_version = 0
        self.layer = 0
        self.has_CRC = False
        self.bitrate = 0
        self.sample_rate = 0
        self.padding = False
        self.channel_mode = "unknown"
        self.mode_extention = "unknown"
        self.copyrighted = False
        self.original = False
        self.emphasis = "unknown"
        self.n_channels = 0


class FRAME_SIDE_INFO:

    def __init__(self):
        self.main_data_begin        = 0
        self.private_bits           = 0
        self.scfsi                  = zeros( (2,4), dtype=int32)
        self.part2_3_length         = zeros( (2,2), dtype=int32)
        self.big_values             = zeros( (2,2), dtype=int32)
        self.global_gain            = zeros( (2,2), dtype=int32)
        self.scalefac_compress      = zeros( (2,2), dtype=int32)
        self.window_switching_flag  = zeros( (2,2), dtype=int32)
        self.block_type             = zeros( (2,2), dtype=int32)
        self.mixed_block_flag       = zeros( (2,2), dtype=int32)
        self.table_select           = zeros( (2,2,3), dtype=int32)
        self.subblock_gain          = zeros( (2,2,3), dtype=int32)
        self.region0_count          = zeros( (2,2), dtype=int32)
        self.region1_count          = zeros( (2,2), dtype=int32)
        self.preflag                = zeros( (2,2), dtype=int32)
        self.scalefac_scale         = zeros( (2,2), dtype=int32)
        self.count1table_select     = zeros( (2,2), dtype=int32)


class MAIN_DATA:

    def __init__(self):
        self.scalefac_s    = zeros( (2,2,3,13), dtype=int32)
        self.scalefac_l    = zeros( (2,2,23), dtype=int32)
        self.q_vals        = zeros( (2,2,32,18), dtype=int32)


class FRAME_DECODED_DATA:

    def __init__(self):
        self.dq_vals          = zeros( (2,2,32,18), dtype=double)
        self.lr_vals          = zeros( (2,2,32,18), dtype=double)
        self.reordered_vals   = zeros( (2,2,32,18), dtype=double)
        self.antialiased_vals = zeros( (2,2,32,18), dtype=double)
        self.hybrid_vals      = zeros( (2,2,32,18), dtype=double)

class MP3_FRAME:

    def __init__(self):
        self.hdr          = HEADER()
        self.side_info    = FRAME_SIDE_INFO()
        self.main_data    = MAIN_DATA()
        self.decoded_data = FRAME_DECODED_DATA()