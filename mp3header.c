#include "mp3header.h"

/**Description: Scans bitstream for syncword until we find it or EOF. The
   syncword must be byte-aligned. It then reads and parses audio header.
* Parameters: Stream handle.
* Return value: PDMP3_OK or PDMP3_ERR if the syncword can't be found,or the header
*               contains impossible values.**/
static int Read_Header(pdmp3_handle *id) {
  unsigned b1,b2,b3,b4,header;
  /* Get the next four bytes from the bitstream */
  b1 = Get_Byte(id);
  b2 = Get_Byte(id);
  b3 = Get_Byte(id);
  b4 = Get_Byte(id);
  /* If we got an End Of File condition we're done */
  if((b1==C_EOF)||(b2==C_EOF)||(b3==C_EOF)||(b4==C_EOF))
    return(PDMP3_ERR);
  header =(b1 << 24) |(b2 << 16) |(b3 << 8) |(b4 << 0);
  /* Are the high 12 bits the syncword(0xfff)? */
  while((header & 0xfff00000) != C_SYNC) {
    /* No,so scan the bitstream one byte at a time until we find it or EOF */
    /* Shift the values one byte to the left */
    b1 = b2;
    b2 = b3;
    b3 = b4;
    /* Get one new byte from the bitstream */
    b4 = Get_Byte(id);
    /* If we got an End Of File condition we're done */
    if(b4 == C_EOF) return(PDMP3_ERR);
    /* Make up the new header */
    header =(b1 << 24) |(b2 << 16) |(b3 << 8) |(b4 << 0);
  } /* while... */
  /* If we get here we've found the sync word,and can decode the header
   * which is in the low 20 bits of the 32-bit sync+header word. */
  /* Decode the header */
  id->g_frame_header.id                 =(header & 0x00080000) >> 19;
  id->g_frame_header.layer              =(header & 0x00060000) >> 17;
  id->g_frame_header.protection_bit     =(header & 0x00010000) >> 16;
  id->g_frame_header.bitrate_index      =(header & 0x0000f000) >> 12;
  id->g_frame_header.sampling_frequency =(header & 0x00000c00) >> 10;
  id->g_frame_header.padding_bit        =(header & 0x00000200) >> 9;
  id->g_frame_header.private_bit        =(header & 0x00000100) >> 8;
  id->g_frame_header.mode               =(header & 0x000000c0) >> 6;
  id->g_frame_header.mode_extension     =(header & 0x00000030) >> 4;
  id->g_frame_header.copyright          =(header & 0x00000008) >> 3;
  id->g_frame_header.original_or_copy   =(header & 0x00000004) >> 2;
  id->g_frame_header.emphasis           =(header & 0x00000003) >> 0;
  /* Check for invalid values and impossible combinations */
  if(id->g_frame_header.id != 1) {
    ERR("ID must be 1\nHeader word is 0x%08x at file pos %d\n",header,Get_Filepos(id));
    return(PDMP3_ERR);
  }
  if(id->g_frame_header.bitrate_index == 0) {
    ERR("Free bitrate format NIY!\nHeader word is 0x%08x at file pos %d\n",header,Get_Filepos(id));
    return(PDMP3_ERR); // exit(1);
  }
  if(id->g_frame_header.bitrate_index == 15) {
    ERR("bitrate_index = 15 is invalid!\nHeader word is 0x%08x at file pos %d\n",header,Get_Filepos(id));
    return(PDMP3_ERR);
  }
  if(id->g_frame_header.sampling_frequency == 3) {
    ERR("sampling_frequency = 3 is invalid!\n");
    ERR("Header word is 0x%08x at file pos %d\n",header,Get_Filepos(id));
    return(PDMP3_ERR);
  }
  if(id->g_frame_header.layer == 0) {
    ERR("layer = 0 is invalid!\n");
    ERR("Header word is 0x%08x at file pos %d\n",header,
   Get_Filepos(id));
    return(PDMP3_ERR);
  }
  id->g_frame_header.layer = 4 - id->g_frame_header.layer;
  /* DBG("Header         =   0x%08x\n",header); */
  if(!id->new_header) id->new_header = 1;
  return(PDMP3_OK);  /* Done */
}


static int Search_Header(pdmp3_handle *id) {
  unsigned pos = id->processed;
  unsigned mark = id->istart;
  int res = PDMP3_NEED_MORE;
  int cnt = 0;
  while(Get_Inbuf_Filled(id) > 4) {
    res = Read_Header(id);
    if (id->g_frame_header.layer == 3) {
      if(res == PDMP3_OK || res == PDMP3_NEW_FORMAT) break;
    }
    if (++mark == INBUF_SIZE) {
      mark = 0;
    }
    id->istart = mark;
    id->processed = pos;
    if (++cnt > (2*576)) return(PDMP3_ERR); /* more than one frame and still no header */
  }
  return res;
}