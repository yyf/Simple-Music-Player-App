//
//  Simple-Music-Player.c
//  portaudio with sndfile
//
//  Created by Yuan-Yi Fan on 07/31/15
//

#include <stdlib.h>
#include <stdio.h>

#include <sndfile.h>
#include <portaudio.h>

#define AUDIO_FILE_NAME "test.wav" // play a wav file in my directory


/*
  1. open file using sndfile
  2. read the file into myData 
  3. pass myData into pa callback
  4. fill pa output buffer using sf_readf_int function (to read myData)
*/

typedef struct
{
    SNDFILE * s; // the file
    SF_INFO i;   // file info including frames, samplerate, channels, format, sections, seekable
    int pos;     // specify where to start playing
}   myData;      // custom data structure for using sndfile

// interface
// The "callback" is a function that is called by the PortAudio engine whenever it has captured audio data, or when it needs more audio data for output.
int Simple_Music_Player_Callback(const void *input,
                                       void *output,
                                       unsigned long frameCount,
                                       const PaStreamCallbackTimeInfo* paTimeInfo,
                                       PaStreamCallbackFlags statusFlags,
                                       void *userData
                                );

int main()
{  
    // PA variables
    PaStream * stream;
    PaError error;
    PaStreamParameters outputParameters;

    // read file using sndfile
    myData * data = (myData *)malloc(sizeof(myData));               // data of myData type, to pass in the Pa_OpenStream later
    data->pos = 0;                                                  // set play from beginning 
    data->i.format = 0; 
    data->s = sf_open(AUDIO_FILE_NAME, SFM_READ, &data->i);         // (const char *path, int mode, SF_INFO *sfinfo)
                                                                    // SFM_READ: read only mode; 
    if (!data->s)                                                   // check if we can open the audio file
    {
        printf("error opening audio file \n");
        return 1;
    }  



    // start PA
    Pa_Initialize(); 
    // output setup
    outputParameters.device = Pa_GetDefaultOutputDevice(); 
    outputParameters.channelCount = data->i.channels;               // use same number of channels
    outputParameters.sampleFormat = paInt32; 
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = 0; 
    // open stream
    error = Pa_OpenStream(&stream,  
                        0,                                          // no input 
                        &outputParameters,
                        data->i.samplerate,                         // use same sample rate
                        paFramesPerBufferUnspecified,               // let portaudio choose buffer size
                        paNoFlag,                                   // no special mode
                        Simple_Music_Player_Callback,               // our callback function
                        data );                                     // pass myData to our callback function
    // check if we can open PA
    if (error) 
    {
        printf("error opening audio output, error code = %i \n", error);
        Pa_Terminate();
        return 1;
    }


    // do the job
    Pa_StartStream(stream); // start calling the Simple_Music_Player_Callback
      Pa_Sleep(2000);     // pause for 2 seconds (as my audio file length is 2 seconds)
    Pa_StopStream(stream);  

    // exit PA
    Pa_Terminate(); 

    return 0;
}

// implementation
int Simple_Music_Player_Callback(const void *input,
                                       void *output,
                                       unsigned long frameCount,
                                       const PaStreamCallbackTimeInfo* paTimeInfo,
                                       PaStreamCallbackFlags statusFlags,
                                       void *userData
                                )
{
    myData * data = (myData *) userData;                  // get the data from the audio file

    int * cur_ptr;                                        // current pointer into the output  
    int * out = (int *) output;                           // output buffer

    int curSize = frameCount;
    int curRead;                                          // use cur_ptr and curHead for sf_readf_int function

    cur_ptr = out;                                        // set the output ptr to the beginning 


    while ( curSize > 0 )                                 // while frameCount > 0, call sf_seek, sf_readf_int, and update read indexes
    {    
        // pass in data, seek position, starting from 0
        sf_seek(data->s, data->pos, SEEK_SET);            // sf_count_t  sf_seek (SNDFILE *sndfile, sf_count_t frames, int whence)
                                                          // The file seek functions work much like lseek in unistd.h with the exception that the non-audio data is ignored and the seek only moves within the audio data section of the file. In addition, seeks are defined in number of (multichannel) frames. Therefore, a seek in a stereo file from the current position forward with an offset of 1 would skip forward by one sample of both channels.
                                                          // "SEEK_SET" The offset is set to the start of the audio data plus offset (multichannel) frames.
        
        if ( curSize > ( data->i.frames - data->pos ) )   // past EOF? 
        {      
              curRead = data->i.frames - data->pos;       // only read to the EOF
              data->pos = 0;                              // reset to read from the beginning
        } 
        else                                              // while frameCount < this diff
        {      
              curRead = curSize;                          // fill up the rest of the output buffer 
              data->pos += curRead;                       // increment the pos
        }

        // read data straight into output buffer using the computed cur_ptr and curRead indexes 
        sf_readf_int(data->s, cur_ptr, curRead);         // sf_count_t  sf_readf_int (SNDFILE *sndfile, int *ptr, sf_count_t frames)
                                                         // The file read frames functions fill the array pointed to by ptr with the requested number of frames of data. The array must be large enough to hold the product of frames and the number of channels. Care must be taken to ensure that there is enough space in the array pointed to by ptr, to take (frames * channels) number of items (shorts, ints, floats or doubles). The sf_readf_XXXX functions return the number of frames read. Unless the end of the file was reached during the read, the return value should equal the number of frames requested. Attempts to read beyond the end of the file will not result in an error but will cause the sf_readf_XXXX functions to return less than the number of frames requested or 0 if already at the end of the file.
                                                         // 

        // update these two indexes    
        cur_ptr += curRead;                              // increment the output ptr
        curSize -= curRead;                              // decrement the number of samples left to process 
    }

    return paContinue;
}