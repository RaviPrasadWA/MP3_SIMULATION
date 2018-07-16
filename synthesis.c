#include "synthesis.h"

/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Frequency_Inversion(pdmp3_handle *id,unsigned gr,unsigned ch){
  unsigned sb,i;

  for(sb = 1; sb < 32; sb += 2) { //OPT? : for(sb = 18; sb < 576; sb += 36)
    for(i = 1; i < 18; i += 2)
      id->g_main_data.is[gr][ch][sb*18 + i] = -id->g_main_data.is[gr][ch][sb*18 + i];
  }
  return; /* Done */
}

/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Hybrid_Synthesis(pdmp3_handle *id,unsigned gr,unsigned ch){
  unsigned sb,i,j,bt;
  float rawout[36];
  static float store[2][32][18];

  if(id->hsynth_init) { /* Clear stored samples vector. OPT? use memset */
    for(j = 0; j < 2; j++) {
      for(sb = 0; sb < 32; sb++) {
        for(i = 0; i < 18; i++) {
          store[j][sb][i] = 0.0;
        }
      }
    }
    id->hsynth_init = 0;
  } /* end if(hsynth_init) */
  for(sb = 0; sb < 32; sb++) { /* Loop through all 32 subbands */
    /* Determine blocktype for this subband */
    bt =((id->g_side_info.win_switch_flag[gr][ch] == 1) &&
     (id->g_side_info.mixed_block_flag[gr][ch] == 1) &&(sb < 2))
      ? 0 : id->g_side_info.block_type[gr][ch];
    /* Do the inverse modified DCT and windowing */
    IMDCT_Win(&(id->g_main_data.is[gr][ch][sb*18]),rawout,bt);
    for(i = 0; i < 18; i++) { /* Overlapp add with stored vector into main_data vector */
      id->g_main_data.is[gr][ch][sb*18 + i] = rawout[i] + store[ch][sb][i];
      store[ch][sb][i] = rawout[i + 18];
    } /* end for(i... */
  } /* end for(sb... */
  return; /* Done */
}

/**Description: TBD
* Parameters: Stream handle,TBD
* Return value: TBD **/
static void L3_Subband_Synthesis(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned outdata[576]){
  float u_vec[512],s_vec[32],sum; /* u_vec can be used insted of s_vec */
  int32_t samp;
  static unsigned init = 1;
  unsigned i,j,ss,nch;
  static float g_synth_n_win[64][32],v_vec[2 /* ch */][1024];

  /* Number of channels(1 for mono and 2 for stereo) */
  nch =(id->g_frame_header.mode == mpeg1_mode_single_channel) ? 1 : 2 ;
  /* Setup the n_win windowing vector and the v_vec intermediate vector */

  if(init) {
    for(i = 0; i < 64; i++) {
      for(j = 0; j < 32; j++) /*TODO: put in lookup table*/
        g_synth_n_win[i][j] = cos(((float)(16+i)*(2*j+1)) *(C_PI/64.0));
    }
    for(i = 0; i < 2; i++) /* Setup the v_vec intermediate vector */
      for(j = 0; j < 1024; j++) v_vec[i][j] = 0.0; /*TODO: memset */
    init = 0;
  } /* end if(init) */

  if(id->synth_init) {
    for(i = 0; i < 2; i++) /* Setup the v_vec intermediate vector */
      for(j = 0; j < 1024; j++) v_vec[i][j] = 0.0; /*TODO: memset*/
    id->synth_init = 0;
  } /* end if(synth_init) */

  for(ss = 0; ss < 18; ss++){ /* Loop through 18 samples in 32 subbands */
    for(i = 1023; i > 63; i--)  /* Shift up the V vector */
      v_vec[ch][i] = v_vec[ch][i-64];
    for(i = 0; i < 32; i++) /* Copy next 32 time samples to a temp vector */
      s_vec[i] =((float) id->g_main_data.is[gr][ch][i*18 + ss]);
    for(i = 0; i < 64; i++){ /* Matrix multiply input with n_win[][] matrix */
      sum = 0.0;
      for(j = 0; j < 32; j++) sum += g_synth_n_win[i][j] * s_vec[j];
      v_vec[ch][i] = sum;
    } /* end for(i... */
    for(i = 0; i < 8; i++) { /* Build the U vector */
      for(j = 0; j < 32; j++) { /* <<7 == *128 */
        u_vec[(i << 6) + j]      = v_vec[ch][(i << 7) + j];
        u_vec[(i << 6) + j + 32] = v_vec[ch][(i << 7) + j + 96];
      }
    } /* end for(i... */
    for(i = 0; i < 512; i++) /* Window by u_vec[i] with g_synth_dtbl[i] */
      u_vec[i] = u_vec[i] * g_synth_dtbl[i];
    for(i = 0; i < 32; i++) { /* Calc 32 samples,store in outdata vector */
      sum = 0.0;
      for(j = 0; j < 16; j++) /* sum += u_vec[j*32 + i]; */
        sum += u_vec[(j << 5) + i];
      /* sum now contains time sample 32*ss+i. Convert to 16-bit signed int */
      samp =(int32_t)(sum * 32767.0);
      if(samp > 32767) samp = 32767;
      else if(samp < -32767) samp = -32767;
      samp &= 0xffff;
      if(ch == 0) {  /* This function must be called for channel 0 first */
        /* We always run in stereo mode,& duplicate channels here for mono */
        if(nch == 1) {
          outdata[32*ss + i] =(samp << 16) |(samp);
        }else{
          outdata[32*ss + i] = samp << 16;
        }
      }else{
        outdata[32*ss + i] |= samp;
      }
    } /* end for(i... */
  } /* end for(ss... */
  return; /* Done */
}