static unsigned Get_Main_Pos(pdmp3_handle *id);
static int Set_Main_Pos(pdmp3_handle *id,unsigned bit_pos);
static unsigned Get_Main_Bit(pdmp3_handle *id);
static unsigned Get_Main_Bits(pdmp3_handle *id,unsigned number_of_bits);
static int Get_Main_Data(pdmp3_handle *id,unsigned main_data_size,unsigned main_data_begin);
static int Read_Main_L3(pdmp3_handle *id);

#include "huffman.c"