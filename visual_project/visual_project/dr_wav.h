#pragma once
#define DR_WAV_IMPLEMENTATION
#include <stdint.h>

typedef int32_t drwav_int32;
typedef uint32_t drwav_uint32;
typedef uint64_t drwav_uint64;

typedef struct
{
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float* pSampleData; // 32-bit float PCM
} drwav_data;

#include <stdlib.h>
#include <string.h>

static float* loadWavFile(const char* filename, unsigned int* sampleRate, drwav_uint64* sampleCount);
