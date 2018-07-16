#include "requantize.h"

/**Description: calculates y=x^(4/3) when requantizing samples.
* Parameters: TBD
* Return value: TBD **/
static inline float Requantize_Pow_43(unsigned is_pos){
  #ifdef POW34_TABLE
    static float powtab34[8207];
    static int init = 0;
    int i;

    if(init == 0) {   /* First time initialization */
      for(i = 0; i < 8207; i++)
        powtab34[i] = pow((float) i,4.0 / 3.0);
      init = 1;
    }
  #ifdef DEBUG
    if(is_pos > 8206) {
      ERR("is_pos = %d larger than 8206!",is_pos);
      is_pos = 8206;
    }
  #endif /* DEBUG */
    return(powtab34[is_pos]);  /* Done */
  #elif defined POW34_ITERATE
    float a4,a2,x,x2,x3,x_next,is_f1,is_f2,is_f3;
    unsigned i;
  //static unsigned init = 0;
  //static float powtab34[32];
    static float coeff[3] = {-1.030797119e+02,6.319399834e+00,2.395095071e-03};
  //if(init == 0) { /* First time initialization */
  //  for(i = 0; i < 32; i++) powtab34[i] = pow((float) i,4.0 / 3.0);
  //  init = 1;
  //}
    /* We use a table for 0<is_pos<32 since they are so common */
    if(is_pos < 32) return(powtab34[is_pos]);
    a2 = is_pos * is_pos;
    a4 = a2 * a2;
    is_f1 =(float) is_pos;
    is_f2 = is_f1 * is_f1;
    is_f3 = is_f1 * is_f2;
    /*  x = coeff[0] + coeff[1]*is_f1 + coeff[2]*is_f2 + coeff[3]*is_f3; */
    x = coeff[0] + coeff[1]*is_f1 + coeff[2]*is_f2;
    for(i = 0; i < 3; i++) {
      x2 = x*x;
      x3 = x*x2;
      x_next =(2*x3 + a4) /(3*x2);
      x = x_next;
    }
    return(x);
  #else /* no optimization */
  return powf((float)is_pos,4.0f / 3.0f);
  #endif /* POW34_TABLE || POW34_ITERATE */
}

/** Description: requantize sample in subband that uses long blocks.
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void Requantize_Process_Long(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned is_pos,unsigned sfb){
  float tmp1,tmp2,tmp3,sf_mult,pf_x_pt;
  static float pretab[21] = { 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2 };

  sf_mult = id->g_side_info.scalefac_scale[gr][ch] ? 1.0 : 0.5;
  pf_x_pt = id->g_side_info.preflag[gr][ch] * pretab[sfb];
  tmp1 = pow(2.0,-(sf_mult *(id->g_main_data.scalefac_l[gr][ch][sfb] + pf_x_pt)));
  tmp2 = pow(2.0,0.25 *((int32_t) id->g_side_info.global_gain[gr][ch] - 210));
  if(id->g_main_data.is[gr][ch][is_pos] < 0.0)
    tmp3 = -Requantize_Pow_43(-id->g_main_data.is[gr][ch][is_pos]);
  else tmp3 = Requantize_Pow_43(id->g_main_data.is[gr][ch][is_pos]);
  id->g_main_data.is[gr][ch][is_pos] = tmp1 * tmp2 * tmp3;
  return; /* Done */
}

/**Description: requantize sample in subband that uses short blocks.
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void Requantize_Process_Short(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned is_pos,unsigned sfb,unsigned win){
  float tmp1,tmp2,tmp3,sf_mult;

  sf_mult = id->g_side_info.scalefac_scale[gr][ch] ? 1.0f : 0.5f;
  tmp1 = pow(2.0f,-(sf_mult * id->g_main_data.scalefac_s[gr][ch][sfb][win]));
  tmp2 = pow(2.0f,0.25f *((float) id->g_side_info.global_gain[gr][ch] - 210.0f -
              8.0f *(float) id->g_side_info.subblock_gain[gr][ch][win]));
  tmp3 =(id->g_main_data.is[gr][ch][is_pos] < 0.0)
    ? -Requantize_Pow_43(-id->g_main_data.is[gr][ch][is_pos])
    : Requantize_Pow_43(id->g_main_data.is[gr][ch][is_pos]);
  id->g_main_data.is[gr][ch][is_pos] = tmp1 * tmp2 * tmp3;
  return; /* Done */
}


/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Requantize(pdmp3_handle *id,unsigned gr,unsigned ch){
  unsigned sfb /* scalefac band index */,next_sfb /* frequency of next sfb */,
    sfreq,i,j,win,win_len;

  /* Setup sampling frequency index */
  sfreq = id->g_frame_header.sampling_frequency;
  /* Determine type of block to process */
  if((id->g_side_info.win_switch_flag[gr][ch] == 1) && (id->g_side_info.block_type[gr][ch] == 2)) { /* Short blocks */
    /* Check if the first two subbands
     *(=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
    if(id->g_side_info.mixed_block_flag[gr][ch] != 0) { /* 2 longbl. sb  first */
      /* First process the 2 long block subbands at the start */
      sfb = 0;
      next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
      for(i = 0; i < 36; i++) {
        if(i == next_sfb) {
          sfb++;
          next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
        } /* end if */
        Requantize_Process_Long(id,gr,ch,i,sfb);
      }
      /* And next the remaining,non-zero,bands which uses short blocks */
      sfb = 3;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] -
        g_sf_band_indices[sfreq].s[sfb];

      for(i = 36; i < id->g_side_info.count1[gr][ch]; /* i++ done below! */) {
        /* Check if we're into the next scalefac band */
        if(i == next_sfb) {        /* Yes */
          sfb++;
          next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
          win_len = g_sf_band_indices[sfreq].s[sfb+1] -
            g_sf_band_indices[sfreq].s[sfb];
        } /* end if(next_sfb) */
        for(win = 0; win < 3; win++) {
          for(j = 0; j < win_len; j++) {
            Requantize_Process_Short(id,gr,ch,i,sfb,win);
            i++;
          } /* end for(j... */
        } /* end for(win... */

      } /* end for(i... */
    }else{ /* Only short blocks */
      sfb = 0;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] -
        g_sf_band_indices[sfreq].s[sfb];
      for(i = 0; i < id->g_side_info.count1[gr][ch]; /* i++ done below! */) {
        /* Check if we're into the next scalefac band */
        if(i == next_sfb) {        /* Yes */
          sfb++;
          next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
          win_len = g_sf_band_indices[sfreq].s[sfb+1] -
            g_sf_band_indices[sfreq].s[sfb];
        } /* end if(next_sfb) */
        for(win = 0; win < 3; win++) {
          for(j = 0; j < win_len; j++) {
            Requantize_Process_Short(id,gr,ch,i,sfb,win);
            i++;
          } /* end for(j... */
        } /* end for(win... */
      } /* end for(i... */
    } /* end else(only short blocks) */
  }else{ /* Only long blocks */
    sfb = 0;
    next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
    for(i = 0; i < id->g_side_info.count1[gr][ch]; i++) {
      if(i == next_sfb) {
        sfb++;
        next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
      } /* end if */
      Requantize_Process_Long(id,gr,ch,i,sfb);
    }
  } /* end else(only long blocks) */
  return; /* Done */
}
