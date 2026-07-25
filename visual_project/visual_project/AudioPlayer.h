#pragma once
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

class AudioPlayer
{
public:
    HWAVEOUT hWaveOut = nullptr;
    WAVEHDR header{};
    short* pcmData = nullptr;
    DWORD dataSize = 0;

    DWORD streamedBytes = 0;   // ★ 재생된 바이트 수 추적용

    bool play(short* data, DWORD size, int sampleRate);
    void stop();

    // ★ 현재 재생 sample index 반환
    DWORD getPlaybackSample(int bytesPerSample) const {
        return streamedBytes / bytesPerSample;
    }
};
