#include "stereo.h"

/**Description: intensity stereo processing for entire subband with long blocks.
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void Stereo_Process_Intensity_Long(pdmp3_handle *id,unsigned gr,unsigned sfb){
  unsigned i,sfreq,sfb_start,sfb_stop,is_pos;
  float is_ratio_l,is_ratio_r,left,right;

  /* Check that((is_pos[sfb]=scalefac) != 7) => no intensity stereo */
  if((is_pos = id->g_main_data.scalefac_l[gr][0][sfb]) != 7) {
    sfreq = id->g_frame_header.sampling_frequency; /* Setup sampling freq index */
    sfb_start = g_sf_band_indices[sfreq].l[sfb];
    sfb_stop = g_sf_band_indices[sfreq].l[sfb+1];
    if(is_pos == 6) { /* tan((6*PI)/12 = PI/2) needs special treatment! */
      is_ratio_l = 1.0f;
      is_ratio_r = 0.0f;
    }else{
      is_ratio_l = is_ratios[is_pos] /(1.0f + is_ratios[is_pos]);
      is_ratio_r = 1.0f /(1.0f + is_ratios[is_pos]);
    }
    /* Now decode all samples in this scale factor band */
    for(i = sfb_start; i < sfb_stop; i++) {
      left = is_ratio_l * id->g_main_data.is[gr][0][i];
      right = is_ratio_r * id->g_main_data.is[gr][0][i];
      id->g_main_data.is[gr][0][i] = left;
      id->g_main_data.is[gr][1][i] = right;
    }
  }
  return; /* Done */
} /* end Stereo_Process_Intensity_Long() */

/**Description: This function is used to perform intensity stereo processing
*              for an entire subband that uses short blocks.
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void Stereo_Process_Intensity_Short(pdmp3_handle *id,unsigned gr,unsigned sfb){
  unsigned sfb_start,sfb_stop,is_pos,is_ratio_l,is_ratio_r,i,sfreq,win,win_len;
  float left,right;

  sfreq = id->g_frame_header.sampling_frequency;   /* Setup sampling freq index */
  /* The window length */
  win_len = g_sf_band_indices[sfreq].s[sfb+1] - g_sf_band_indices[sfreq].s[sfb];
  /* The three windows within the band has different scalefactors */
  for(win = 0; win < 3; win++) {
    /* Check that((is_pos[sfb]=scalefac) != 7) => no intensity stereo */
    if((is_pos = id->g_main_data.scalefac_s[gr][0][sfb][win]) != 7) {
      sfb_start = g_sf_band_indices[sfreq].s[sfb]*3 + win_len*win;
      sfb_stop = sfb_start + win_len;
      if(is_pos == 6) { /* tan((6*PI)/12 = PI/2) needs special treatment! */
        is_ratio_l = 1.0;
        is_ratio_r = 0.0;
      }else{
        is_ratio_l = is_ratios[is_pos] /(1.0 + is_ratios[is_pos]);
        is_ratio_r = 1.0 /(1.0 + is_ratios[is_pos]);
      }
      /* Now decode all samples in this scale factor band */
      for(i = sfb_start; i < sfb_stop; i++) {
        left = is_ratio_l = id->g_main_data.is[gr][0][i];
        right = is_ratio_r = id->g_main_data.is[gr][0][i];
        id->g_main_data.is[gr][0][i] = left;
        id->g_main_data.is[gr][1][i] = right;
      }
    } /* end if(not illegal is_pos) */
  } /* end for(win... */
  return; /* Done */
} /* end Stereo_Process_Intensity_Short() */


/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Stereo(pdmp3_handle *id,unsigned gr){
  unsigned max_pos,i,sfreq,sfb /* scalefac band index */;
  float left,right;

  /* Do nothing if joint stereo is not enabled */
  if((id->g_frame_header.mode != 1)||(id->g_frame_header.mode_extension == 0)) return;
  /* Do Middle/Side("normal") stereo processing */
  if(id->g_frame_header.mode_extension & 0x2) {
    /* Determine how many frequency lines to transform */
    max_pos = id->g_side_info.count1[gr][!!(id->g_side_info.count1[gr][0] > id->g_side_info.count1[gr][1])];
    /* Do the actual processing */
    for(i = 0; i < max_pos; i++) {
      left =(id->g_main_data.is[gr][0][i] + id->g_main_data.is[gr][1][i])
        *(C_INV_SQRT_2);
      right =(id->g_main_data.is[gr][0][i] - id->g_main_data.is[gr][1][i])
        *(C_INV_SQRT_2);
      id->g_main_data.is[gr][0][i] = left;
      id->g_main_data.is[gr][1][i] = right;
    } /* end for(i... */
  } /* end if(ms_stereo... */
  /* Do intensity stereo processing */
  if(id->g_frame_header.mode_extension & 0x1) {
    /* Setup sampling frequency index */
    sfreq = id->g_frame_header.sampling_frequency;
    /* First band that is intensity stereo encoded is first band scale factor
     * band on or above count1 frequency line. N.B.: Intensity stereo coding is
     * only done for higher subbands, but logic is here for lower subbands. */
    /* Determine type of block to process */
    if((id->g_side_info.win_switch_flag[gr][0] == 1) &&
       (id->g_side_info.block_type[gr][0] == 2)) { /* Short blocks */
      /* Check if the first two subbands
       *(=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
      if(id->g_side_info.mixed_block_flag[gr][0] != 0) { /* 2 longbl. sb  first */
        for(sfb = 0; sfb < 8; sfb++) {/* First process 8 sfb's at start */
          /* Is this scale factor band above count1 for the right channel? */
          if(g_sf_band_indices[sfreq].l[sfb] >= id->g_side_info.count1[gr][1])
            Stereo_Process_Intensity_Long(id,gr,sfb);
        } /* end if(sfb... */
        /* And next the remaining bands which uses short blocks */
        for(sfb = 3; sfb < 12; sfb++) {
          /* Is this scale factor band above count1 for the right channel? */
          if(g_sf_band_indices[sfreq].s[sfb]*3 >= id->g_side_info.count1[gr][1])
            Stereo_Process_Intensity_Short(id,gr,sfb); /* intensity stereo processing */
        }
      }else{ /* Only short blocks */
        for(sfb = 0; sfb < 12; sfb++) {
          /* Is this scale factor band above count1 for the right channel? */
          if(g_sf_band_indices[sfreq].s[sfb]*3 >= id->g_side_info.count1[gr][1])
            Stereo_Process_Intensity_Short(id,gr,sfb); /* intensity stereo processing */
        }
      } /* end else(only short blocks) */
    }else{                        /* Only long blocks */
      for(sfb = 0; sfb < 21; sfb++) {
        /* Is this scale factor band above count1 for the right channel? */
        if(g_sf_band_indices[sfreq].l[sfb] >= id->g_side_info.count1[gr][1]) {
          /* Perform the intensity stereo processing */
          Stereo_Process_Intensity_Long(id,gr,sfb);
        }
      }
    } /* end else(only long blocks) */
  } /* end if(intensity_stereo processing) */
}
