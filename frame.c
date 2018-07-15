#include "frame.h"

static void Convert_Frame_S16(pdmp3_handle *id,unsigned char *outbuf,size_t buflen,size_t *done)
{
  short *s = (short *)outbuf;
  unsigned lo,hi,nsamps,framesz;
  int q,i,nch,gr;

  nch = (id->g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);
  framesz = sizeof(short)*nch;

  nsamps = buflen / framesz;
  if (nsamps > (2*576 - id->ostart)) {
    nsamps = 2*576 - id->ostart;
  }
  *done = nsamps * framesz;

  /* copy to outbuf */
  i = id->ostart % 576;
  gr = id->ostart/576;
  for (q = 0; q < nsamps; ++q) {
    if(nch == 1) {
      lo = id->out[gr][i] & 0xffff;
      s[q] = lo;
    } else {
      lo = id->out[gr][i] & 0xffff;
      hi =(id->out[gr][i] & 0xffff0000) >> 16;
      s[2*q] = hi;
      s[2*q+1] = lo;
    }
    if (++i == 576) {
       ++gr;
       i = 0;
    }
  }

  id->ostart += nsamps;
  if (id->ostart == (2*576)) {
    id->ostart = 0;
  }
}


static unsigned Get_Inbuf_Filled(pdmp3_handle *id) {
  return (id->istart<=id->iend)?(id->iend-id->istart):(INBUF_SIZE-id->istart+id->iend);
}

static void dmp_fr(t_mpeg1_header *hdr){
  printf("rate %d,sfreq %d,pad %d,mod %d,modext %d,emph %d\n",
          hdr->bitrate_index,hdr->sampling_frequency,hdr->padding_bit,
          hdr->mode,hdr->mode_extension,hdr->emphasis);
}

static void dmp_si(t_mpeg1_header *hdr,t_mpeg1_side_info *si){
  int nch,ch,gr;

  nch = hdr->mode == mpeg1_mode_single_channel ? 1 : 2;
  printf("main_data_begin %d,priv_bits %d\n",si->main_data_begin,si->private_bits);
  for(ch = 0; ch < nch; ch++) {
    printf("scfsi %d %d %d %d\n",si->scfsi[ch][0],si->scfsi[ch][1],si->scfsi[ch][2],si->scfsi[ch][3]);
    for(gr = 0; gr < 2; gr++) {
      printf("p23l %d,bv %d,gg %d,scfc %d,wsf %d,bt %d\n",
              si->part2_3_length[gr][ch],si->big_values[gr][ch],
              si->global_gain[gr][ch],si->scalefac_compress[gr][ch],
              si->win_switch_flag[gr][ch],si->block_type[gr][ch]);
      if(si->win_switch_flag[gr][ch]) {
        printf("mbf %d,ts1 %d,ts2 %d,sbg1 %d,sbg2 %d,sbg3 %d\n",
                si->mixed_block_flag[gr][ch],si->table_select[gr][ch][0],
                si->table_select[gr][ch][1],si->subblock_gain[gr][ch][0],
                si->subblock_gain[gr][ch][1],si->subblock_gain[gr][ch][2]);
      }else{
        printf("ts1 %d,ts2 %d,ts3 %d\n",si->table_select[gr][ch][0],
                si->table_select[gr][ch][1],si->table_select[gr][ch][2]);
      }
      printf("r0c %d,r1c %d\n",si->region0_count[gr][ch],si->region1_count[gr][ch]);
      printf("pf %d,scfs %d,c1ts %d\n",si->preflag[gr][ch],si->scalefac_scale[gr][ch],si->count1table_select[gr][ch]);
    }
  }
}

/**Description: returns next byte from bitstream, or EOF.
*  If we're not on an byte-boundary, bits remaining until next boundary are
*  discarded before getting that byte.
* Parameters: Stream handle.
* Return value: The next byte in bitstream in the lowest 8 bits,or C_EOF.**/
static unsigned Get_Byte(pdmp3_handle *id){
  unsigned val = C_EOF;
  if(id->istart != id->iend){
    val = id->in[id->istart++]; //  && 0xff;
    if(id->istart == INBUF_SIZE){
      id->istart=0;
    }
    id->processed++;
  }
  return(val);
}

/** Description: reads 'no_of_bytes' from input stream into 'data_vec[]'.
*   Parameters: Stream handle,number of bytes to read,vector pointer where to
                store them.
*   Return value: PDMP3_OK or PDMP3_ERR if the operation couldn't be performed.**/
static int Get_Bytes(pdmp3_handle *id,unsigned no_of_bytes,unsigned data_vec[]){
  int i;
  unsigned val;

  for(i = 0; i < no_of_bytes; i++) {
    val = Get_Byte(id);
    if(val == C_EOF) return(C_EOF);
    else data_vec[i] = val;
  }
  return(PDMP3_OK);
}


/**Description: Reads 16 CRC bits
* Parameters: Stream handle.
* Return value: PDMP3_OK or PDMP3_ERR if CRC could not be read.**/
static int Read_CRC(pdmp3_handle *id){
  /* Get next two bytes from bitstream, If we got an End Of File we're done */
  if((Get_Byte(id)==C_EOF)||(Get_Byte(id)==C_EOF)) return(FALSE);
  return(PDMP3_OK);  /* Done */
}


/**Description: returns current file position in bytes.
* Parameters: Stream handle.
* Return value: File pos in bytes,or 0 if no file open.**/
static unsigned Get_Filepos(pdmp3_handle *id){
  return(id->processed);
}

/**Description: Reads audio and main data from bitstream into a buffer. main
*  data is taken from this frame and up to 2 previous frames.
* Parameters: Stream handle.
* Return value: PDMP3_OK or PDMP3_ERR if data could not be read,or contains errors.**/
static int Read_Audio_L3(pdmp3_handle *id){
  unsigned framesize,sideinfo_size,main_data_size,nch,ch,gr,scfsi_band,region,window;

  /* Number of channels(1 for mono and 2 for stereo) */
  nch =(id->g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);
  /* Calculate header audio data size */
  framesize = (144 *
    g_mpeg1_bitrates[id->g_frame_header.layer-1][id->g_frame_header.bitrate_index]) /
    g_sampling_frequency[id->g_frame_header.sampling_frequency] +
    id->g_frame_header.padding_bit;
  if(framesize > 2000) {
    ERR("framesize = %d\n",framesize);
    return(PDMP3_ERR);
  }
  /* Sideinfo is 17 bytes for one channel and 32 bytes for two */
  sideinfo_size =(nch == 1 ? 17 : 32);
  /* Main data size is the rest of the frame,including ancillary data */
  main_data_size = framesize - sideinfo_size - 4 /* sync+header */;
  /* CRC is 2 bytes */
  if(id->g_frame_header.protection_bit == 0) main_data_size -= 2;
  /* DBG("framesize      =   %d\n",framesize); */
  /* DBG("sideinfo_size  =   %d\n",sideinfo_size); */
  /* DBG("main_data_size =   %d\n",main_data_size); */
  /* Read sideinfo from bitstream into buffer used by Get_Side_Bits() */
  Get_Sideinfo(id,sideinfo_size);
  if(Get_Filepos(id) == C_EOF) return(PDMP3_ERR);
  /* Parse audio data */
  /* Pointer to where we should start reading main data */
  id->g_side_info.main_data_begin = Get_Side_Bits(id,9);
  /* Get private bits. Not used for anything. */
  if(id->g_frame_header.mode == mpeg1_mode_single_channel)
    id->g_side_info.private_bits = Get_Side_Bits(id,5);
  else id->g_side_info.private_bits = Get_Side_Bits(id,3);
  /* Get scale factor selection information */
  for(ch = 0; ch < nch; ch++)
    for(scfsi_band = 0; scfsi_band < 4; scfsi_band++)
      id->g_side_info.scfsi[ch][scfsi_band] = Get_Side_Bits(id,1);
  /* Get the rest of the side information */
  for(gr = 0; gr < 2; gr++) {
    for(ch = 0; ch < nch; ch++) {
      id->g_side_info.part2_3_length[gr][ch]    = Get_Side_Bits(id,12);
      id->g_side_info.big_values[gr][ch]        = Get_Side_Bits(id,9);
      id->g_side_info.global_gain[gr][ch]       = Get_Side_Bits(id,8);
      id->g_side_info.scalefac_compress[gr][ch] = Get_Side_Bits(id,4);
      id->g_side_info.win_switch_flag[gr][ch]   = Get_Side_Bits(id,1);
      if(id->g_side_info.win_switch_flag[gr][ch] == 1) {
        id->g_side_info.block_type[gr][ch]       = Get_Side_Bits(id,2);
        id->g_side_info.mixed_block_flag[gr][ch] = Get_Side_Bits(id,1);
        for(region = 0; region < 2; region++)
          id->g_side_info.table_select[gr][ch][region] = Get_Side_Bits(id,5);
        for(window = 0; window < 3; window++)
          id->g_side_info.subblock_gain[gr][ch][window] = Get_Side_Bits(id,3);
        if((id->g_side_info.block_type[gr][ch]==2)&&(id->g_side_info.mixed_block_flag[gr][ch]==0))
          id->g_side_info.region0_count[gr][ch] = 8; /* Implicit */
        else id->g_side_info.region0_count[gr][ch] = 7; /* Implicit */
        /* The standard is wrong on this!!! */   /* Implicit */
        id->g_side_info.region1_count[gr][ch] = 20 - id->g_side_info.region0_count[gr][ch];
     }else{
       for(region = 0; region < 3; region++)
         id->g_side_info.table_select[gr][ch][region] = Get_Side_Bits(id,5);
       id->g_side_info.region0_count[gr][ch] = Get_Side_Bits(id,4);
       id->g_side_info.region1_count[gr][ch] = Get_Side_Bits(id,3);
       id->g_side_info.block_type[gr][ch] = 0;  /* Implicit */
      }  /* end if ... */
      id->g_side_info.preflag[gr][ch]            = Get_Side_Bits(id,1);
      id->g_side_info.scalefac_scale[gr][ch]     = Get_Side_Bits(id,1);
      id->g_side_info.count1table_select[gr][ch] = Get_Side_Bits(id,1);
    } /* end for(channel... */
  } /* end for(granule... */
  return(PDMP3_OK);/* Done */

}

static int Read_Frame(pdmp3_handle *id){
  /* Try to find the next frame in the bitstream and decode it */
  if(Search_Header(id) != PDMP3_OK) return(PDMP3_ERR);
  #ifdef DEBUG
    { 
      static int framenum = 0;
      printf("\nFrame %d\n",framenum++);
      dmp_fr(&id->g_frame_header);
    }
    DBG("Starting decode,Layer: %d,Rate: %6d,Sfreq: %05d",
         id->g_frame_header.layer,
         g_mpeg1_bitrates[id->g_frame_header.layer - 1][id->g_frame_header.bitrate_index],
         g_sampling_frequency[id->g_frame_header.sampling_frequency]);
  #endif
  /* Get CRC word if present */
  if((id->g_frame_header.protection_bit==0)&&(Read_CRC(id)!=PDMP3_OK)) 
    return(PDMP3_ERR);
  if(id->g_frame_header.layer == 3) {  /* Get audio data */
    Read_Audio_L3(id);  /* Get side info */
    dmp_si(&id->g_frame_header,&id->g_side_info); /* DEBUG */
    /* If there's not enough main data in the bit reservoir,
     * signal to calling function so that decoding isn't done! */
    /* Get main data(scalefactors and Huffman coded frequency data) */
    return(Read_Main_L3(id));
  }else{
    ERR("Only layer 3(!= %d) is supported!\n",id->g_frame_header.layer);
    return(PDMP3_ERR);
  }
  return(PDMP3_OK);
}