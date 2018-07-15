#include "maindata.h"

/**Description: returns pos. of next bit to be read from main data bitstream.
* Parameters: Stream handle.
* Return value: Bit position.**/
static unsigned Get_Main_Pos(pdmp3_handle *id){
  unsigned pos;
  
  pos =((size_t) id->g_main_data_ptr) -((size_t) &(id->g_main_data_vec[0]));
  pos /= 4; /* Divide by four to get number of bytes */
  pos *= 8;    /* Multiply by 8 to get number of bits */
  pos += id->g_main_data_idx;  /* Add current bit index */
  return(pos);
}

/**Description: sets position of next bit to be read from main data bitstream.
* Parameters: Stream handle,Bit position. 0 = start,8 = start of byte 1,etc.
* Return value: PDMP3_OK or PDMP3_ERR if bit_pos is past end of main data for this frame.**/
static int Set_Main_Pos(pdmp3_handle *id,unsigned bit_pos){

  id->g_main_data_ptr = &(id->g_main_data_vec[bit_pos >> 3]);
  id->g_main_data_idx = bit_pos & 0x7;

  return(PDMP3_OK);

}


/**Description: gets one bit from the local buffer which contains main_data.
* Parameters: Stream handle.
* Return value: The bit is returned in the LSB of the return value.**/
static unsigned Get_Main_Bit(pdmp3_handle *id){
  unsigned tmp;

  tmp = id->g_main_data_ptr[0] >>(7 - id->g_main_data_idx);
  tmp &= 0x01;
  id->g_main_data_ptr +=(id->g_main_data_idx + 1) >> 3;
  id->g_main_data_idx =(id->g_main_data_idx + 1) & 0x07;
  return(tmp);  /* Done */
}


/**Description: reads 'number_of_bits' from local buffer containing main_data.
* Parameters: Stream handle,number_of_bits to read(max 24)
* Return value: The bits are returned in the LSB of the return value.
*
******************************************************************************/
static unsigned Get_Main_Bits(pdmp3_handle *id,unsigned number_of_bits){
  unsigned tmp;


  if(number_of_bits == 0) return(0);

  /* Form a word of the next four bytes */
  tmp =(id->g_main_data_ptr[0] << 24) |(id->g_main_data_ptr[1] << 16) |
       (id->g_main_data_ptr[2] <<  8) |(id->g_main_data_ptr[3] <<  0);

  /* Remove bits already used */
  tmp = tmp << id->g_main_data_idx;

  /* Remove bits after the desired bits */
  tmp = tmp >>(32 - number_of_bits);

  /* Update pointers */
  id->g_main_data_ptr +=(id->g_main_data_idx + number_of_bits) >> 3;
  id->g_main_data_idx =(id->g_main_data_idx + number_of_bits) & 0x07;

  /* Done */
  return(tmp);

}


/** Description: This function assembles the main data buffer with data from
*              this frame and the previous two frames into a local buffer
*              used by the Get_Main_Bits function.
* Parameters: Stream handle,main_data_begin indicates how many bytes from
*             previous frames that should be used. main_data_size indicates the
*             number of data bytes in this frame.
* Return value: Status**/
static int Get_Main_Data(pdmp3_handle *id,unsigned main_data_size,unsigned main_data_begin){
  int i;

  if(main_data_size > 1500) 
    ERR("main_data_size = %d\n",main_data_size);
  /* Check that there's data available from previous frames if needed */
  if(main_data_begin > id->g_main_data_top) {
    /* No,there is not,so we skip decoding this frame,but we have to
     * read the main_data bits from the bitstream in case they are needed
     * for decoding the next frame. */
   (void) Get_Bytes(id,main_data_size,&(id->g_main_data_vec[id->g_main_data_top]));
    /* Set up pointers */
    id->g_main_data_ptr = &(id->g_main_data_vec[0]);
    id->g_main_data_idx = 0;
    id->g_main_data_top += main_data_size;
    return(PDMP3_NEED_MORE);    /* This frame cannot be decoded! */
  }
  for(i = 0; i < main_data_begin; i++) {  /* Copy data from previous frames */
    id->g_main_data_vec[i] = id->g_main_data_vec[id->g_main_data_top - main_data_begin + i];
  }
  /* Read the main_data from file */
 (void) Get_Bytes(id,main_data_size,&(id->g_main_data_vec[main_data_begin]));
  /* Set up pointers */
  id->g_main_data_ptr = &(id->g_main_data_vec[0]);
  id->g_main_data_idx = 0;
  id->g_main_data_top = main_data_begin + main_data_size;
  return(PDMP3_OK);  /* Done */
}

/**Description: reads main data for layer 3 from main_data bit reservoir.
* Parameters: Stream handle.
* Return value: PDMP3_OK or PDMP3_ERR if the data contains errors.**/
static int Read_Main_L3(pdmp3_handle *id){
  unsigned framesize,sideinfo_size,main_data_size,gr,ch,nch,sfb,win,slen1,slen2,nbits,part_2_start;
  int res;

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
  /* Assemble main data buffer with data from this frame and the previous
   * two frames. main_data_begin indicates how many bytes from previous
   * frames that should be used. This buffer is later accessed by the
   * Get_Main_Bits function in the same way as the side info is.
   */
  res = Get_Main_Data(id,main_data_size,id->g_side_info.main_data_begin);
  if(res != PDMP3_OK) return(res); /* This could be due to not enough data in reservoir */
  for(gr = 0; gr < 2; gr++) {
    for(ch = 0; ch < nch; ch++) {
      part_2_start = Get_Main_Pos(id);
      /* Number of bits in the bitstream for the bands */
      slen1 = mpeg1_scalefac_sizes[id->g_side_info.scalefac_compress[gr][ch]][0];
      slen2 = mpeg1_scalefac_sizes[id->g_side_info.scalefac_compress[gr][ch]][1];
      if((id->g_side_info.win_switch_flag[gr][ch] != 0)&&(id->g_side_info.block_type[gr][ch] == 2)) {
        if(id->g_side_info.mixed_block_flag[gr][ch] != 0) {
          for(sfb = 0; sfb < 8; sfb++)
            id->g_main_data.scalefac_l[gr][ch][sfb] = Get_Main_Bits(id,slen1);
          for(sfb = 3; sfb < 12; sfb++) {
            nbits = (sfb < 6)?slen1:slen2;/*slen1 for band 3-5,slen2 for 6-11*/
            for(win = 0; win < 3; win++)
              id->g_main_data.scalefac_s[gr][ch][sfb][win]=Get_Main_Bits(id,nbits);
          }
        }else{
          for(sfb = 0; sfb < 12; sfb++){
            nbits = (sfb < 6)?slen1:slen2;/*slen1 for band 3-5,slen2 for 6-11*/
            for(win = 0; win < 3; win++)
              id->g_main_data.scalefac_s[gr][ch][sfb][win]=Get_Main_Bits(id,nbits);
          }
        }
      }else{ /* block_type == 0 if winswitch == 0 */
        /* Scale factor bands 0-5 */
        if((id->g_side_info.scfsi[ch][0] == 0) ||(gr == 0)) {
          for(sfb = 0; sfb < 6; sfb++)
            id->g_main_data.scalefac_l[gr][ch][sfb] = Get_Main_Bits(id,slen1);
        }else if((id->g_side_info.scfsi[ch][0] == 1) &&(gr == 1)) {
          /* Copy scalefactors from granule 0 to granule 1 */
          for(sfb = 0; sfb < 6; sfb++)
            id->g_main_data.scalefac_l[1][ch][sfb]=id->g_main_data.scalefac_l[0][ch][sfb];
        }
        /* Scale factor bands 6-10 */
        if((id->g_side_info.scfsi[ch][1] == 0) ||(gr == 0)) {
          for(sfb = 6; sfb < 11; sfb++)
            id->g_main_data.scalefac_l[gr][ch][sfb] = Get_Main_Bits(id,slen1);
        }else if((id->g_side_info.scfsi[ch][1] == 1) &&(gr == 1)) {
          /* Copy scalefactors from granule 0 to granule 1 */
          for(sfb = 6; sfb < 11; sfb++)
            id->g_main_data.scalefac_l[1][ch][sfb]=id->g_main_data.scalefac_l[0][ch][sfb];
        }
        /* Scale factor bands 11-15 */
        if((id->g_side_info.scfsi[ch][2] == 0) ||(gr == 0)) {
          for(sfb = 11; sfb < 16; sfb++)
            id->g_main_data.scalefac_l[gr][ch][sfb] = Get_Main_Bits(id,slen2);
        } else if((id->g_side_info.scfsi[ch][2] == 1) &&(gr == 1)) {
          /* Copy scalefactors from granule 0 to granule 1 */
          for(sfb = 11; sfb < 16; sfb++)
            id->g_main_data.scalefac_l[1][ch][sfb]=id->g_main_data.scalefac_l[0][ch][sfb];
        }
        /* Scale factor bands 16-20 */
        if((id->g_side_info.scfsi[ch][3] == 0) ||(gr == 0)) {
          for(sfb = 16; sfb < 21; sfb++)
            id->g_main_data.scalefac_l[gr][ch][sfb] = Get_Main_Bits(id,slen2);
        }else if((id->g_side_info.scfsi[ch][3] == 1) &&(gr == 1)) {
          /* Copy scalefactors from granule 0 to granule 1 */
          for(sfb = 16; sfb < 21; sfb++)
            id->g_main_data.scalefac_l[1][ch][sfb]=id->g_main_data.scalefac_l[0][ch][sfb];
        }
      }
      /* Read Huffman coded data. Skip stuffing bits. */
      Read_Huffman(id,part_2_start,gr,ch);
    } /* end for(gr... */
  } /* end for(ch... */
  /* The ancillary data is stored here,but we ignore it. */
  return(PDMP3_OK);  /* Done */
}