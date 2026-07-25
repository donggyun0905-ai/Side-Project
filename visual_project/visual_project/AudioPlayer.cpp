#include "AudioPlayer.h"

// -----------------------------
// ★ WAVE OUT CALLBACK (REQUIRED)
// -----------------------------
void CALLBACK WaveOutProc(
    HWAVEOUT hwo,
    UINT msg,
    DWORD_PTR instance,
    DWORD_PTR param1,
    DWORD_PTR param2
)
{
    if (msg == WOM_DONE)
    {
        AudioPlayer* player = reinterpret_cast<AudioPlayer*>(instance);

        // 재생된 바이트 누적
        player->streamedBytes += player->header.dwBufferLength;
    }
}


// -----------------------------
// AudioPlayer 구현
// -----------------------------
bool AudioPlayer::play(short* data, DWORD size, int sampleRate)
{
    pcmData = data;
    dataSize = size;
    streamedBytes = 0;  // ★ 초기화

    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx,
        (DWORD_PTR)WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        return false;

    header.lpData = (LPSTR)pcmData;
    header.dwBufferLength = dataSize;

    waveOutPrepareHeader(hWaveOut, &header, sizeof(header));
    waveOutWrite(hWaveOut, &header, sizeof(header));

    return true;
}


void AudioPlayer::stop()
{
    if (hWaveOut)
    {
        waveOutReset(hWaveOut);
        waveOutUnprepareHeader(hWaveOut, &header, sizeof(header));
        waveOutClose(hWaveOut);
        hWaveOut = nullptr;
    }
}
