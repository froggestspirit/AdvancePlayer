/*
 * $Id$
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define min(a,b) fmin(a,b)
#define max(a,b) fmax(a,b)
#include <portaudio.h>
#define SAMPLE_RATE   (48000)
extern unsigned char music[];
int song;
extern int totalSongs;
extern bool running;
float audio[2];
float *lastAudio;
char *filename;
FILE* musicFile = NULL;
//#include "GBA.h"

typedef struct
{
    float left_phase;
    float right_phase;
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData)
{
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    static unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */

    for(i = 0; i < framesPerBuffer; i++){
        mloop(&audio);

        *out++ = *(audio);
        *out++ = *(audio+1);
    }
    return 0;
}

/*******************************************************************/
static paTestData data;
static float *out;
int main(int argc, char *argv[]){
    song = 0;
    bool inf = false;
    if(argc > 1){
        filename = argv[1];
    }else{
        filename = "SA2.bin";
    }
    if(argc > 2) song = atoi(argv[2]);
    if(argc > 3) inf = true;
    PaStream *stream;
    PaError err;
    musicFile = fopen(filename, "rb");
    if (0 != fseek(musicFile, 0, SEEK_END)) return false;
    int sizef = ftell(musicFile);
    if (0 != fseek(musicFile, 0, SEEK_SET)) return false;
    if (sizef != fread(music, 1, sizef, musicFile)) return false;
    printf("music Size: %X\n",sizef);
    fclose(musicFile);
    minit(song,SAMPLE_RATE,inf);//music Song Number, Frequency, infinite loop
    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream(&stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paFloat32,  /* 32 bit output */
                                SAMPLE_RATE,
                                0x1000,        /* frames per buffer */
                                patestCallback,
                                &data);
    if(err != paNoError) goto error;

    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;
    while(1){
        sleep(2);
    if(!running){
            err = Pa_StopStream(stream);
            stop();
            return 0;
        }
    }
    err = Pa_StopStream(stream);
    if(err != paNoError) goto error;
    err = Pa_CloseStream(stream);
    if(err != paNoError) goto error;
    Pa_Terminate();
    printf("Done!\n");
    return err;
error:
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
