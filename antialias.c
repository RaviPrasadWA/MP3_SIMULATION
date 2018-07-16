#include "antialias.h"

/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Antialias(pdmp3_handle *id,unsigned gr,unsigned ch){
  unsigned sb /* subband of 18 samples */,i,sblim,ui,li;
  float ub,lb;

  /* No antialiasing is done for short blocks */
  if((id->g_side_info.win_switch_flag[gr][ch] == 1) &&
     (id->g_side_info.block_type[gr][ch] == 2) &&
     (id->g_side_info.mixed_block_flag[gr][ch]) == 0) {
    return; /* Done */
  }
  /* Setup the limit for how many subbands to transform */
  sblim =((id->g_side_info.win_switch_flag[gr][ch] == 1) &&
    (id->g_side_info.block_type[gr][ch] == 2) &&
    (id->g_side_info.mixed_block_flag[gr][ch] == 1))?2:32;
  /* Do the actual antialiasing */
  for(sb = 1; sb < sblim; sb++) {
    for(i = 0; i < 8; i++) {
      li = 18*sb-1-i;
      ui = 18*sb+i;
      lb = id->g_main_data.is[gr][ch][li]*cs[i] - id->g_main_data.is[gr][ch][ui]*ca[i];
      ub = id->g_main_data.is[gr][ch][ui]*cs[i] + id->g_main_data.is[gr][ch][li]*ca[i];
      id->g_main_data.is[gr][ch][li] = lb;
      id->g_main_data.is[gr][ch][ui] = ub;
    }
  }
  return; /* Done */
}