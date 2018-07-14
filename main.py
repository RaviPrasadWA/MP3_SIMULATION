import time
from numpy import   ( 	
						zeros, 
						double,
						sin,
						cos,
						modf
					)
from stdvals import PI, FRAME_CNT_LIMIT
from bitframe import BitFrame
from mp3frame import MP3_FRAME
from hdinfo import decode_frame_header
from syncword import find_next_syncword


class MP3:

	def __init__(self, filename):
		f = open(filename, "rb")
		file_data = f.read()
		f.close()

		self.bitbfr = BitFrame(file_data)

		self.hybrid_prevblck = zeros( (2,32,18), dtype=double)
		self.sbs_bufOffset   = [64, 64]        
		self.sbs_buf         = zeros( (2,1024), dtype=double)
		self.sbs_filter      = zeros( (64,32), dtype=double)
		self.pcm             = [ [], [] ]

		self.first_frame = True


		self.mdct_win = zeros( (4,36), dtype=double)
		self.mdct_cos_tbl = zeros( (4*36), dtype=double)
		self.mdct_cos_tbl_2 = zeros( (36,18), dtype=double)

		# block 0 
		for i in xrange(36):
			self.mdct_win[0][i] = sin( PI/36 *(i+0.5) )

		# block 1
		for i in xrange(18):
			self.mdct_win[1][i] = sin( PI/36 *(i+0.5) )

		for i in xrange(18, 24):
			self.mdct_win[1][i] = 1.0
		for i in xrange(24, 30):
			self.mdct_win[1][i] = sin( PI/12 *(i+0.5-18) )
		for i in xrange(30, 36):
			self.mdct_win[1][i] = 0.0

		# block 2 
		for i in xrange(12):
			self.mdct_win[2][i] = sin( PI/12*(i+0.5) ) 
		for i in xrange(12, 36): 
			self.mdct_win[2][i] = 0.0

		#block 3
		for i in xrange(6):
			self.mdct_win[3][i] = 0.0
		for i in xrange(6, 12):
			self.mdct_win[3][i] = sin( PI/12 *(i+0.5-6) )
		for i in xrange(12, 18):
			self.mdct_win[3][i] =1.0
		for i in xrange(18, 36):
			self.mdct_win[3][i] = sin( PI/36*(i+0.5) )            

		for i in xrange(4*36):
			self.mdct_cos_tbl[i] = cos(PI/(2*36) * i)            

		N = 36
		for p in xrange(N):
			for m in xrange(N/2):
				self.mdct_cos_tbl_2[p][m] = self.mdct_cos_tbl[((2*p+1+N/2)*(2*m+1))%(4*36)]
        
        
		#FILTER
		for i in xrange(64):
			for k in xrange(32):
				self.sbs_filter[i][k] = 1e9*cos(((PI/64*i+PI/4)*(2*k+1)))                 
				if (self.sbs_filter[i][k] >= 0):
					dummy, self.sbs_filter[i][k] = modf(self.sbs_filter[i][k]+0.5)
				else:
					dummy, self.sbs_filter[i][k] = modf(self.sbs_filter[i][k]-0.5)
				self.sbs_filter[i][k] *= 1e-9;

	def decode_CRC(self):
		if(self.cur_frame.hdr.has_CRC):
			self.bitbfr.read_bits(16)

	def decode(self, filename):
		self.frame_num = 0
		while( self.bitbfr.bits_left() > 0):        
			self.cur_frame = MP3_FRAME()
			st = time.time()
			find_next_syncword(self.bitbfr)
			self.first_frame = decode_frame_header(self.cur_frame,self.bitbfr,self.first_frame)
			self.decode_CRC()
			self.frame_num+=1
			if(self.frame_num == FRAME_CNT_LIMIT ):
		        break



mp3 = MP3("test.mp3")
mp3.decode("test.mp3")