#include "huffman.h"

/**Description: reads/decodes next Huffman code word from main_data reservoir.
* Parameters: Stream handle,Huffman table number and four pointers for the
              return values.
* Return value: Two(x,y) or four(x,y,v,w) decoded Huffman words.**/
static int Huffman_Decode(pdmp3_handle *id,unsigned table_num,int32_t *x,int32_t *y,int32_t *v,int32_t *w){
  unsigned point=0,error=1,bitsleft=32, //=16??
    treelen = g_huffman_main[table_num].treelen,
    linbits = g_huffman_main[table_num].linbits;

  treelen = g_huffman_main[table_num].treelen;
  if(treelen == 0) { /* Check for empty tables */
    *x = *y = *v = *w = 0;
    return(PDMP3_OK);
  }
  const unsigned short *htptr = g_huffman_main[table_num].hufftable;
  do {   /* Start reading the Huffman code word,bit by bit */
    /* Check if we've matched a code word */
    if((htptr[point] & 0xff00) == 0) {
      error = 0;
      *x =(htptr[point] >> 4) & 0xf;
      *y = htptr[point] & 0xf;
      break;
    }
    if(Get_Main_Bit(id)) { /* Go right in tree */
      while((htptr[point] & 0xff) >= 250)
        point += htptr[point] & 0xff;
      point += htptr[point] & 0xff;
    }else{ /* Go left in tree */
      while((htptr[point] >> 8) >= 250)
        point += htptr[point] >> 8;
      point += htptr[point] >> 8;
    }
  } while((--bitsleft > 0) &&(point < treelen));
  if(error) {  /* Check for error. */
    ERR("Illegal Huff code in data. bleft = %d,point = %d. tab = %d.",
      bitsleft,point,table_num);
    *x = *y = 0;
  }
  if(table_num > 31) {  /* Process sign encodings for quadruples tables. */
    *v =(*y >> 3) & 1;
    *w =(*y >> 2) & 1;
    *x =(*y >> 1) & 1;
    *y = *y & 1;
    if((*v > 0)&&(Get_Main_Bit(id) == 1)) *v = -*v;
    if((*w > 0)&&(Get_Main_Bit(id) == 1)) *w = -*w;
    if((*x > 0)&&(Get_Main_Bit(id) == 1)) *x = -*x;
    if((*y > 0)&&(Get_Main_Bit(id) == 1)) *y = -*y;
  }else{
    if((linbits > 0)&&(*x == 15))*x += Get_Main_Bits(id,linbits);/* Get linbits */
    if((*x > 0)&&(Get_Main_Bit(id) == 1)) *x = -*x; /* Get sign bit */
    if((linbits > 0)&&(*y == 15))*y += Get_Main_Bits(id,linbits);/* Get linbits */
    if((*y > 0)&&(Get_Main_Bit(id) == 1)) *y = -*y;/* Get sign bit */
  }
  return(error ? PDMP3_ERR : PDMP3_OK);  /* Done */
}

/**Description: called by Read_Main_L3 to read Huffman coded data from bitstream.
* Parameters: Stream handle,TBD
* Return value: None. The data is stored in id->g_main_data.is[ch][gr][freqline].**/
static void Read_Huffman(pdmp3_handle *id,unsigned part_2_start,unsigned gr,unsigned ch){
  int32_t x,y,v,w;
  unsigned table_num,is_pos,bit_pos_end,sfreq;
  unsigned region_1_start,region_2_start; /* region_0_start = 0 */

  /* Check that there is any data to decode. If not,zero the array. */
  if(id->g_side_info.part2_3_length[gr][ch] == 0) {
    for(is_pos = 0; is_pos < 576; is_pos++)
      id->g_main_data.is[gr][ch][is_pos] = 0.0;
    return;
  }
  /* Calculate bit_pos_end which is the index of the last bit for this part. */
  bit_pos_end = part_2_start + id->g_side_info.part2_3_length[gr][ch] - 1;
  /* Determine region boundaries */
  if((id->g_side_info.win_switch_flag[gr][ch] == 1)&&
     (id->g_side_info.block_type[gr][ch] == 2)) {
    region_1_start = 36;  /* sfb[9/3]*3=36 */
    region_2_start = 576; /* No Region2 for short block case. */
  }else{
    sfreq = id->g_frame_header.sampling_frequency;
    region_1_start =
      g_sf_band_indices[sfreq].l[id->g_side_info.region0_count[gr][ch] + 1];
    region_2_start =
      g_sf_band_indices[sfreq].l[id->g_side_info.region0_count[gr][ch] +
        id->g_side_info.region1_count[gr][ch] + 2];
  }
  /* Read big_values using tables according to region_x_start */
  for(is_pos = 0; is_pos < id->g_side_info.big_values[gr][ch] * 2; is_pos++) {
    if(is_pos < region_1_start) {
      table_num = id->g_side_info.table_select[gr][ch][0];
    } else if(is_pos < region_2_start) {
      table_num = id->g_side_info.table_select[gr][ch][1];
    }else table_num = id->g_side_info.table_select[gr][ch][2];
    /* Get next Huffman coded words */
   (void) Huffman_Decode(id,table_num,&x,&y,&v,&w);
    /* In the big_values area there are two freq lines per Huffman word */
    id->g_main_data.is[gr][ch][is_pos++] = x;
    id->g_main_data.is[gr][ch][is_pos] = y;
  }
  /* Read small values until is_pos = 576 or we run out of huffman data */
  table_num = id->g_side_info.count1table_select[gr][ch] + 32;
  for(is_pos = id->g_side_info.big_values[gr][ch] * 2;
      (is_pos <= 572) &&(Get_Main_Pos(id) <= bit_pos_end); is_pos++) {
    /* Get next Huffman coded words */
   (void) Huffman_Decode(id,table_num,&x,&y,&v,&w);
    id->g_main_data.is[gr][ch][is_pos++] = v;
    if(is_pos >= 576) break;
    id->g_main_data.is[gr][ch][is_pos++] = w;
    if(is_pos >= 576) break;
    id->g_main_data.is[gr][ch][is_pos++] = x;
    if(is_pos >= 576) break;
    id->g_main_data.is[gr][ch][is_pos] = y;
  }
  /* Check that we didn't read past the end of this section */
  if(Get_Main_Pos(id) >(bit_pos_end+1)) /* Remove last words read */
    is_pos -= 4;
  /* Setup count1 which is the index of the first sample in the rzero reg. */
  id->g_side_info.count1[gr][ch] = is_pos;
  /* Zero out the last part if necessary */
  for(/* is_pos comes from last for-loop */; is_pos < 576; is_pos++)
    id->g_main_data.is[gr][ch][is_pos] = 0.0;
  /* Set the bitpos to point to the next part to read */
 (void) Set_Main_Pos(id,bit_pos_end+1);
  return;  /* Done */
}