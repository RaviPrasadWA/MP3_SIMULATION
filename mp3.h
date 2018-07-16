#include <stdlib.h>
#include <stdint.h>
#include "_defines.h"

/* Types used in the frame header */
typedef enum { /* Layer number */
  mpeg1_layer_reserved = 0,
  mpeg1_layer_3        = 1,
  mpeg1_layer_2        = 2,
  mpeg1_layer_1        = 3
}t_mpeg1_layer;

typedef enum { /* Modes */
  mpeg1_mode_stereo = 0,
  mpeg1_mode_joint_stereo,
  mpeg1_mode_dual_channel,
  mpeg1_mode_single_channel
}t_mpeg1_mode;

typedef struct { /* MPEG1 Layer 1-3 frame header */
  unsigned id;                 /* 1 bit */
  t_mpeg1_layer layer;         /* 2 bits */
  unsigned protection_bit;     /* 1 bit */
  unsigned bitrate_index;      /* 4 bits */
  unsigned sampling_frequency; /* 2 bits */
  unsigned padding_bit;        /* 1 bit */
  unsigned private_bit;        /* 1 bit */
  t_mpeg1_mode mode;           /* 2 bits */
  unsigned mode_extension;     /* 2 bits */
  unsigned copyright;          /* 1 bit */
  unsigned original_or_copy;   /* 1 bit */
  unsigned emphasis;           /* 2 bits */
}t_mpeg1_header;

typedef struct {  /* MPEG1 Layer 3 Side Information : [2][2] means [gr][ch] */
  unsigned main_data_begin;         /* 9 bits */
  unsigned private_bits;            /* 3 bits in mono,5 in stereo */
  unsigned scfsi[2][4];             /* 1 bit */
  unsigned part2_3_length[2][2];    /* 12 bits */
  unsigned big_values[2][2];        /* 9 bits */
  unsigned global_gain[2][2];       /* 8 bits */
  unsigned scalefac_compress[2][2]; /* 4 bits */
  unsigned win_switch_flag[2][2];   /* 1 bit */
  /* if(win_switch_flag[][]) */ //use a union dammit
  unsigned block_type[2][2];        /* 2 bits */
  unsigned mixed_block_flag[2][2];  /* 1 bit */
  unsigned table_select[2][2][3];   /* 5 bits */
  unsigned subblock_gain[2][2][3];  /* 3 bits */
  /* else */
  /* table_select[][][] */
  unsigned region0_count[2][2];     /* 4 bits */
  unsigned region1_count[2][2];     /* 3 bits */
  /* end */
  unsigned preflag[2][2];           /* 1 bit */
  unsigned scalefac_scale[2][2];    /* 1 bit */
  unsigned count1table_select[2][2];/* 1 bit */
  unsigned count1[2][2];            /* Not in file,calc. by huff.dec.! */
}t_mpeg1_side_info;

typedef struct { /* MPEG1 Layer 3 Main Data */
  unsigned  scalefac_l[2][2][21];    /* 0-4 bits */
  unsigned  scalefac_s[2][2][12][3]; /* 0-4 bits */
  float is[2][2][576];               /* Huffman coded freq. lines */
}t_mpeg1_main_data;

typedef struct hufftables{
  const unsigned short * hufftable;
  uint16_t treelen;
  uint8_t linbits;
}hufftables;

typedef struct { /* Scale factor band indices,for long and short windows */
  unsigned l[23];
  unsigned s[14];
}t_sf_band_indices;

typedef struct
{
  size_t processed;
  unsigned istart,iend,ostart;
  unsigned char in[INBUF_SIZE];
  unsigned out[2][576];
  t_mpeg1_header g_frame_header;
  t_mpeg1_side_info g_side_info;  /* < 100 words */
  t_mpeg1_main_data g_main_data;

  unsigned hsynth_init;
  unsigned synth_init;
  /* Bit reservoir for main data */
  unsigned g_main_data_vec[2*1024];/* Large static data */
  unsigned *g_main_data_ptr;/* Pointer into the reservoir */
  unsigned g_main_data_idx;/* Index into the current byte(0-7) */
  unsigned g_main_data_top;/* Number of bytes in reservoir(0-1024) */
  /* Bit reservoir for side info */
  unsigned side_info_vec[32+4];
  unsigned *side_info_ptr;  /* Pointer into the reservoir */
  unsigned side_info_idx;  /* Index into the current byte(0-7) */

  char new_header;
}pdmp3_handle;

#include "frame.c"
#include "requantize.c"
#include "reorder.c"
#include "stereo.c"
#include "antialias.c"

static void dmp_scf(t_mpeg1_side_info *si,t_mpeg1_main_data *md,int gr,int ch);
static void dmp_huff(t_mpeg1_main_data *md,int gr,int ch);
static void dmp_samples(t_mpeg1_main_data *md,int gr,int ch,int type);
static int Decode_L3(pdmp3_handle *id);
int pdmp3_read(pdmp3_handle *id,unsigned char *outmemory,size_t outsize,size_t *done);