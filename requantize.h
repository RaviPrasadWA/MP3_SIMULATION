#include <math.h>

static inline float Requantize_Pow_43(unsigned is_pos);
static void Requantize_Process_Long(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned is_pos,unsigned sfb);
static void Requantize_Process_Short(pdmp3_handle *id,unsigned gr,unsigned ch,unsigned is_pos,unsigned sfb,unsigned win);
static void L3_Requantize(pdmp3_handle *id,unsigned gr,unsigned ch);