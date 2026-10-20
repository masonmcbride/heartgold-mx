#include "ds_audio_output.h"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CAPACITY 8192
#define BUFFER_FRAMES 512
struct DSAudioOutput {
    AudioQueueRef queue;pthread_mutex_t mutex;
    int16_t samples[CAPACITY*2];unsigned read,count;uint64_t played;
};
static void fill(void *context,AudioQueueRef queue,AudioQueueBufferRef buffer) {
    DSAudioOutput *out=context;int16_t *dst=buffer->mAudioData;
    pthread_mutex_lock(&out->mutex);
    unsigned frames=out->count<BUFFER_FRAMES?out->count:BUFFER_FRAMES;
    for(unsigned i=0;i<frames;i++){
        dst[2*i]=out->samples[2*out->read];dst[2*i+1]=out->samples[2*out->read+1];out->read=(out->read+1)%CAPACITY;
    }
    out->count-=frames;out->played+=frames;pthread_mutex_unlock(&out->mutex);
    memset(dst+frames*2,0,(BUFFER_FRAMES-frames)*4);buffer->mAudioDataByteSize=BUFFER_FRAMES*4;
    AudioQueueEnqueueBuffer(queue,buffer,0,NULL);
}
DSAudioOutput *ds_audio_output_new(char *error,size_t size) {
    DSAudioOutput *out=calloc(1,sizeof(*out));if(!out)return NULL;pthread_mutex_init(&out->mutex,NULL);
    AudioStreamBasicDescription format={.mSampleRate=DS_AUDIO_RATE,.mFormatID=kAudioFormatLinearPCM,
        .mFormatFlags=kLinearPCMFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked,
        .mBytesPerPacket=4,.mFramesPerPacket=1,.mBytesPerFrame=4,.mChannelsPerFrame=2,.mBitsPerChannel=16};
    OSStatus status=AudioQueueNewOutput(&format,fill,out,NULL,NULL,0,&out->queue);
    for(unsigned i=0;!status&&i<3;i++) {AudioQueueBufferRef buffer;status=AudioQueueAllocateBuffer(out->queue,BUFFER_FRAMES*4,&buffer);if(!status)fill(out,out->queue,buffer);}
    if(!status)status=AudioQueueStart(out->queue,NULL);
    if(status){if(error&&size)snprintf(error,size,"Native audio output failed (%d)",(int)status);ds_audio_output_free(out);return NULL;}
    return out;
}
void ds_audio_output_free(DSAudioOutput *out) {
    if(!out)return;if(out->queue){AudioQueueStop(out->queue,true);AudioQueueDispose(out->queue,true);}pthread_mutex_destroy(&out->mutex);free(out);
}
void ds_audio_output_push(DSAudioOutput *out,const int16_t *pcm,unsigned frames) {
    if(!out||!pcm)return;pthread_mutex_lock(&out->mutex);
    if(frames>CAPACITY){pcm+=(frames-CAPACITY)*2;frames=CAPACITY;}
    if(out->count+frames>CAPACITY){unsigned dropped=out->count+frames-CAPACITY;out->read=(out->read+dropped)%CAPACITY;out->count-=dropped;}
    for(unsigned i=0;i<frames;i++){unsigned at=(out->read+out->count+i)%CAPACITY;out->samples[at*2]=pcm[i*2];out->samples[at*2+1]=pcm[i*2+1];}
    out->count+=frames;pthread_mutex_unlock(&out->mutex);
}
uint64_t ds_audio_output_frames(DSAudioOutput *out){if(!out)return 0;pthread_mutex_lock(&out->mutex);uint64_t frames=out->played;pthread_mutex_unlock(&out->mutex);return frames;}
