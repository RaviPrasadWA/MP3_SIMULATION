#include "imdct.c"

static void L3_Frequency_Inversion(pdmp3_handle *id,unsigned gr,unsigned ch);
static void L3_Hybrid_Synthesis(pdmp3_handle *id,unsigned gr,unsigned ch);
static void L3_Subband_Synthesis(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned outdata[576]);
