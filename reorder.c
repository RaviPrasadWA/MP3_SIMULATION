#include "reorder.h"

/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Reorder(pdmp3_handle *id,unsigned gr,unsigned ch){
  unsigned sfreq,i,j,next_sfb,sfb,win_len,win;
  float re[576];

  sfreq = id->g_frame_header.sampling_frequency; /* Setup sampling freq index */
  /* Only reorder short blocks */
  if((id->g_side_info.win_switch_flag[gr][ch] == 1) &&
     (id->g_side_info.block_type[gr][ch] == 2)) { /* Short blocks */
    /* Check if the first two subbands
     *(=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
    sfb = (id->g_side_info.mixed_block_flag[gr][ch] != 0)?3:0; /* 2 longbl. sb  first */
    next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
    win_len = g_sf_band_indices[sfreq].s[sfb+1] - g_sf_band_indices[sfreq].s[sfb];
    for(i =((sfb == 0) ? 0 : 36); i < 576; /* i++ done below! */) {
      /* Check if we're into the next scalefac band */
      if(i == next_sfb) {        /* Yes */
        /* Copy reordered data back to the original vector */
        for(j = 0; j < 3*win_len; j++)
          id->g_main_data.is[gr][ch][3*g_sf_band_indices[sfreq].s[sfb] + j] = re[j];
        /* Check if this band is above the rzero region,if so we're done */
        if(i >= id->g_side_info.count1[gr][ch]) return; /* Done */
        sfb++;
        next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
        win_len = g_sf_band_indices[sfreq].s[sfb+1] - g_sf_band_indices[sfreq].s[sfb];
      } /* end if(next_sfb) */
      for(win = 0; win < 3; win++) { /* Do the actual reordering */
        for(j = 0; j < win_len; j++) {
          re[j*3 + win] = id->g_main_data.is[gr][ch][i];
          i++;
        } /* end for(j... */
      } /* end for(win... */
    } /* end for(i... */
    /* Copy reordered data of last band back to original vector */
    for(j = 0; j < 3*win_len; j++)
      id->g_main_data.is[gr][ch][3 * g_sf_band_indices[sfreq].s[12] + j] = re[j];
  } /* end else(only long blocks) */
  return; /* Done */
}