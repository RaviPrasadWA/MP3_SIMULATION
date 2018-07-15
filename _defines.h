#include <stdio.h>

#define PDMP3_OK           0
#define PDMP3_ERR         -1
#define PDMP3_NEED_MORE  -10
#define PDMP3_NEW_FORMAT -11
#define PDMP3_NO_SPACE     7
#define PDMP3_ENC_SIGNED_16 (0x080|0x040|0x10)
#define INBUF_SIZE      (4*4096)
#define SIM_UNIX
#define TRUE       1
#define FALSE      0
#define C_SYNC            0xfff00000
#define C_EOF             0xffffffff
#define C_PI              3.14159265358979323846
#define C_INV_SQRT_2      0.70710678118654752440
#define Hz                1
#define kHz               1000*Hz
#define bit_s                        1
#define kbit_s            1000*bit_s
#define FRAG_SIZE_LN2     0x0011 /* 2^17=128kb */
#define FRAG_NUMS         0x0004

#define DBG(str,args...) { printf(str,## args); printf("\n"); }
#define ERR(str,args...) { fprintf(stderr,str,## args) ; fprintf(stderr,"\n"); }