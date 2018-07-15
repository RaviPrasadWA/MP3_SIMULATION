#include "_defines.h"
#include "tables.h"


static void Convert_Frame_S16(pdmp3_handle *id,unsigned char *outbuf,size_t buflen,size_t *done);
static unsigned Get_Inbuf_Filled(pdmp3_handle *id);
static void dmp_fr(t_mpeg1_header *hdr);
static int Read_Frame(pdmp3_handle *id);
static void dmp_si(t_mpeg1_header *hdr,t_mpeg1_side_info *si);
static unsigned Get_Byte(pdmp3_handle *id);
static int Get_Bytes(pdmp3_handle *id,unsigned no_of_bytes,unsigned data_vec[]);
static int Read_CRC(pdmp3_handle *id);
static unsigned Get_Filepos(pdmp3_handle *id);
static int Read_Audio_L3(pdmp3_handle *id);

#include "sideband.c"
#include "mp3header.c"
#include "maindata.c"