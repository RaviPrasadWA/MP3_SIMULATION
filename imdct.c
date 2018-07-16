#include "imdct.h"

/**Description: Does inverse modified DCT and windowing.
* Parameters: TBD
* Return value: TBD **/
static void IMDCT_Win(float in[18],float out[36],unsigned block_type){
  unsigned i,m,N,p;
  float sum,tin[18];
#ifndef IMDCT_TABLES
  static float g_imdct_win[4][36];
  static unsigned init = 1;
//TODO : move to separate init function
  if(init) { /* Setup the four(one for each block type) window vectors */
    for(i = 0; i < 36; i++)  g_imdct_win[0][i] = sin(C_PI/36 *(i + 0.5)); //0
    for(i = 0; i < 18; i++)  g_imdct_win[1][i] = sin(C_PI/36 *(i + 0.5)); //1
    for(i = 18; i < 24; i++) g_imdct_win[1][i] = 1.0;
    for(i = 24; i < 30; i++) g_imdct_win[1][i] = sin(C_PI/12 *(i + 0.5 - 18.0));
    for(i = 30; i < 36; i++) g_imdct_win[1][i] = 0.0;
    for(i = 0; i < 12; i++)  g_imdct_win[2][i] = sin(C_PI/12 *(i + 0.5)); //2
    for(i = 12; i < 36; i++) g_imdct_win[2][i] = 0.0;
    for(i = 0; i < 6; i++)   g_imdct_win[3][i] = 0.0; //3
    for(i = 6; i < 12; i++)  g_imdct_win[3][i] = sin(C_PI/12 *(i + 0.5 - 6.0));
    for(i = 12; i < 18; i++) g_imdct_win[3][i] = 1.0;
    for(i = 18; i < 36; i++) g_imdct_win[3][i] = sin(C_PI/36 *(i + 0.5));
    init = 0;
  } /* end of init */
#endif
  for(i = 0; i < 36; i++) out[i] = 0.0;
  for(i = 0; i < 18; i++) tin[i] = in[i];
  if(block_type == 2) { /* 3 short blocks */
    N = 12;
    for(i = 0; i < 3; i++) {
      for(p = 0; p < N; p++) {
        sum = 0.0;
        for(m = 0;m < N/2; m++)
#ifdef IMDCT_NTABLES
          sum += tin[i+3*m] * cos_N12[m][p];
#else
          sum += tin[i+3*m] * cos(C_PI/(2*N)*(2*p+1+N/2)*(2*m+1));
#endif
        out[6*i+p+6] += sum * g_imdct_win[block_type][p]; //TODO FIXME +=?
      }
    } /* end for(i... */
  }else{ /* block_type != 2 */
    N = 36;
    for(p = 0; p < N; p++){
      sum = 0.0;
      for(m = 0; m < N/2; m++)
#ifdef IMDCT_NTABLES
        sum += in[m] * cos_N36[m][p];
#else
        sum += in[m] * cos(C_PI/(2*N)*(2*p+1+N/2)*(2*m+1));
#endif
      out[p] = sum * g_imdct_win[block_type][p];
    }
  }
}