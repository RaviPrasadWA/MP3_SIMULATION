#include "mp3.h"

static void dmp_scf(t_mpeg1_side_info *si,t_mpeg1_main_data *md,int gr,int ch){
  int sfb,win;

  if((si->win_switch_flag[gr][ch] != 0) &&(si->block_type[gr][ch] == 2)) {
    if(si->mixed_block_flag[gr][ch] != 0) { /* First the long block scalefacs */
      for(sfb = 0; sfb < 8; sfb++)
        printf("scfl%d %d%s",sfb,md->scalefac_l[gr][ch][sfb],(sfb==7)?"\n":",");
      for(sfb = 3; sfb < 12; sfb++) /* And next the short block scalefacs */
        for(win = 0; win < 3; win++)
          printf("scfs%d,%d %d%s",sfb,win,md->scalefac_s[gr][ch][sfb][win],(win==2)?"\n":",");
    }else{                /* Just short blocks */
      for(sfb = 0; sfb < 12; sfb++)
        for(win = 0; win < 3; win++)
          printf("scfs%d,%d %d%s",sfb,win,md->scalefac_s[gr][ch][sfb][win],(win==2)?"\n":",");
    }
  }else for(sfb = 0; sfb < 21; sfb++) /* Just long blocks; scalefacs first */
    printf("scfl%d %d%s",sfb,md->scalefac_l[gr][ch][sfb],(sfb == 20)?"\n":",");
}


static void dmp_huff(t_mpeg1_main_data *md,int gr,int ch){
  int i;
  printf("HUFFMAN\n");
  for(i = 0; i < 576; i++) printf("%d: %d\n",i,(int) md->is[gr][ch][i]);
}

static void dmp_samples(t_mpeg1_main_data *md,int gr,int ch,int type){
  int i,val;
  extern double rint(double);

  printf("SAMPLES%d\n",type);
  for(i = 0; i < 576; i++) {
    val =(int) rint(md->is[gr][ch][i] * 32768.0);
    if(val >= 32768) val = 32767;
    if(val < -32768) val = -32768;
    printf("%d: %d\n",i,val);
  }
}


/**Description: decodes a layer 3 bitstream into audio samples.
* Parameters: Stream handle,outdata vector.
* Return value: PDMP3_OK or PDMP3_ERR if the frame contains errors. **/
static int Decode_L3(pdmp3_handle *id){
  unsigned gr,ch,nch;

  /* Number of channels(1 for mono and 2 for stereo) */
  nch =(id->g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);
  for(gr = 0; gr < 2; gr++) {
    for(ch = 0; ch < nch; ch++) {
      dmp_scf(&id->g_side_info,&id->g_main_data,gr,ch); //noop unless debug
      dmp_huff(&id->g_main_data,gr,ch); //noop unless debug
      L3_Requantize(id,gr,ch); /* Requantize samples */
      dmp_samples(&id->g_main_data,gr,ch,0); //noop unless debug
      L3_Reorder(id,gr,ch); /* Reorder short blocks */
    } /* end for(ch... */
    L3_Stereo(id,gr); /* Stereo processing */
    dmp_samples(&id->g_main_data,gr,0,1); //noop unless debug
    dmp_samples(&id->g_main_data,gr,1,1); //noop unless debug
    for(ch = 0; ch < nch; ch++) {
      L3_Antialias(id,gr,ch); /* Antialias */
      dmp_samples(&id->g_main_data,gr,ch,2); //noop unless debug
      L3_Hybrid_Synthesis(id,gr,ch); /*(IMDCT,windowing,overlapp add) */
      L3_Frequency_Inversion(id,gr,ch); /* Frequency inversion */
      dmp_samples(&id->g_main_data,gr,ch,3); //noop unless debug
      L3_Subband_Synthesis(id,gr,ch,id->out[gr]); /* Polyphase subband synthesis */
    } /* end for(ch... */
#ifdef DEBUG
    {
      int i,ctr = 0;
      printf("PCM:\n");
      for(i = 0; i < 576; i++) {
        printf("%d: %d\n",ctr++,(out[i] >> 16) & 0xffff);
        if(nch == 2) printf("%d: %d\n",ctr++,out[i] & 0xffff);
      }
    }
#endif /* DEBUG */
  } /* end for(gr... */
  return(PDMP3_OK);   /* Done */
}


/**Description: Convert MP3 data to PCM data
* Parameters: Stream handle,a pointer to a buffer for the PCM data,the size of
              the PCM buffer in bytes,a pointer to return the number of
              converted bytes.
* Return value: PDMP3_OK or an error.**/
int mp3_read(pdmp3_handle *id,unsigned char *outmemory,size_t outsize,size_t *done){
  if(id && outmemory && outsize && done) {
    *done = 0;
    if(outsize) {
      int res = PDMP3_ERR;

      if (id->ostart) {
        Convert_Frame_S16(id,outmemory,outsize,done);
        outmemory += *done;
        outsize -= *done;
        res = PDMP3_OK;
      }

      while(outsize) {
        if (Get_Inbuf_Filled(id) >= (2*576)) {
          size_t pos = id->processed;
          unsigned mark = id->istart;

          res = Read_Frame(id);
          if(res == PDMP3_OK || res == PDMP3_NEW_FORMAT) {
            size_t batch;

            Decode_L3(id);
            Convert_Frame_S16(id,outmemory,outsize,&batch);
            outmemory += batch;
            outsize -= batch;
            *done += batch;
          }
          else {
            id->processed = pos;
            id->istart = mark;
            break;
          }
        }
        else {
          res = PDMP3_NEED_MORE;
          break;
        }
      } /* outsize */
      if(id->new_header == 1 && res == PDMP3_OK) {
        res = PDMP3_NEW_FORMAT;
      }
      return(res);
    }
    else if(outsize < (2*576)) {
      return(PDMP3_NO_SPACE);
    }
    return(PDMP3_NEED_MORE);
  }
  return(PDMP3_ERR);
}

/**Description: Create a new streaming handle
* Parameters: None
* Return value: Stream handle **/

pdmp3_handle* mp3_new(const char *decoder,int *error){
  return malloc(sizeof(pdmp3_handle));
}

/**Description: Resets the stream handle.
* Parameters: Stream handle
* Return value: PDMP3_OK or PDMP3_ERR **/
int mp3_open_feed(pdmp3_handle *id){
  if(id) {
    id->ostart = 0;
    id->istart = 0;
    id->iend = 0;
    id->processed = 0;
    id->new_header = 0;

    id->hsynth_init = 1;
    id->synth_init = 1;
    id->g_main_data_top = 0;

    return(PDMP3_OK);
  }
  return(PDMP3_ERR);
}

int main(int ac, char **av){
  static const char *filename,*audio_name = "/dev/dsp";
  static FILE *fp =(FILE *) NULL;
  unsigned char out[INBUF_SIZE];
  pdmp3_handle *id;
  size_t done;
  int res;
  id = mp3_new(NULL,NULL);
  if(id == 0)
    Error("Cannot open stream API (out of memory)",0);
  while(*av){
    filename = *av++;
    if(!strcmp(filename,"-")) fp=stdin;
    else fp = fopen(filename,"r");
    if(fp == (FILE *) NULL)
      Error("Cannot open file\n",0);

    mp3_open_feed(id);
    while((res = mp3_read(id,out,INBUF_SIZE,&done)) != PDMP3_ERR){
      audio_write(id,audio_name,filename,out,done);
      if(res == PDMP3_OK || res == PDMP3_NEW_FORMAT) {
      }
      else if(res == PDMP3_NEED_MORE){
        unsigned char in[4096];

        res = fread(in,1,4096,fp);
        if(!res) break;

        res = pdmp3_feed(id,in,res);
      }
    }
    fclose(fp);
  }
  pdmp3_delete(id);
}