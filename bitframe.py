class BitFrame:

	def __init__(self, frame=[]):
		self.frame 			= [ ord(_) for _ in frame ]
		self.position 		= 0
		self.frame_length 	= len(frame)*8

	def init_with_int_array(self, frame):
		self.frame 			= frame
		self.position 		= 0
		self.frame_length 	= len(frame)*8

	def seek_abs(self, position):
		self.position = position

	def seek_rel(self, offset):
		self.position += offset

	def read_bits(self, n_bits, debug=False):
		val = 0
		bits_read = 0
		while(bits_read < n_bits):
			val 		= val << 1
			src_byte 	= self.frame[ self.position//8 ]
			src_bit 	= src_byte & (0x80>>(self.position%8))
			if(src_bit != 0):
				val |= 1
			self.position  	+= 1
			bits_read 		+= 1
		return val 

	def bits_left(self):
		if(self.frame_length <= self.position):
			return 0
		else:
			return (self.frame_length - self.position)

	def get_pos(self):
		return self.position 