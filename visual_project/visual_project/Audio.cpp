#define NOMINMAX
#include "Audio.h"
#include "AudioPlayer.h"
#include <fftw3.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>


static float clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

AudioAnalyzer::AudioAnalyzer() {}
AudioAnalyzer::~AudioAnalyzer()
{
    if (fftPlan) fftwf_destroy_plan(fftPlan);
    if (fftInput) fftwf_free(fftInput);
    if (fftOutput) fftwf_free(fftOutput);
}

/* -------------------------------------------------
   🔥 간단 WAV 로더 (16bit PCM 전용)
   ------------------------------------------------- */
bool loadWavSimple(const std::string& path,
    std::vector<float>& outSamples,
    unsigned int& outSampleRate)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[WAV] File open failed: " << path << "\n";
        return false;
    }

    // --- WAV 헤더 구조 읽기 ---
    char riff[4];
    f.read(riff, 4);                // "RIFF"
    f.ignore(4);                    // 파일 전체 크기
    char wave[4];
    f.read(wave, 4);                // "WAVE"

    char fmt[4];
    f.read(fmt, 4);                 // "fmt "
    uint32_t fmtSize;
    f.read((char*)&fmtSize, 4);     // 보통 16
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    f.read((char*)&audioFormat, 2);
    f.read((char*)&numChannels, 2);
    f.read((char*)&sampleRate, 4);
    f.read((char*)&byteRate, 4);
    f.read((char*)&blockAlign, 2);
    f.read((char*)&bitsPerSample, 2);

    // fmt chunk size 초과분 skip
    if (fmtSize > 16)
        f.ignore(fmtSize - 16);

    // --- data chunk 찾기 ---
    char dataId[4];
    uint32_t dataSize = 0;

    while (true)
    {
        f.read(dataId, 4);
        f.read((char*)&dataSize, 4);
        if (std::string(dataId, 4) == "data")
            break;
        f.ignore(dataSize);  // 다른 chunk skip
    }

    if (bitsPerSample != 16) {
        std::cerr << "[WAV] Only PCM16 supported\n";
        return false;
    }

    // --- 데이터 읽기 ---
    size_t totalSamples = dataSize / (bitsPerSample / 8) / numChannels;
    outSamples.resize(totalSamples);

    for (size_t i = 0; i < totalSamples; i++)
    {
        int16_t sL;
        f.read((char*)&sL, sizeof(int16_t));
        if (numChannels == 2) {
            int16_t sR;
            f.read((char*)&sR, sizeof(int16_t));
            outSamples[i] = ((sL + sR) * 0.5f) / 32768.0f; // 스테레오 → 모노
        }
        else {
            outSamples[i] = sL / 32768.0f;
        }
    }

    outSampleRate = sampleRate;
    std::cout << "[WAV] Loaded " << path << " (" << totalSamples << " samples)\n";
    return true;
}


/* -------------------------------------------------
   AudioAnalyzer::loadAudio — WAV 읽기 + FFT 준비
   ------------------------------------------------- */
bool AudioAnalyzer::loadAudio(const std::string& path, int sr)
{
    unsigned int wavRate = 0;
    if (!loadWavSimple(path, samples, wavRate))
        return false;

    sampleRate = wavRate;

    // ----- FFT 준비 -----
    fftSize = 1024;
    hopSize = 512;
    fftInput = (float*)fftwf_malloc(sizeof(float) * fftSize);
    fftOutput = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * (fftSize / 2 + 1));
    fftPlan = fftwf_plan_dft_r2c_1d(fftSize, fftInput, fftOutput, FFTW_MEASURE);

    currentSample = 0;
    smoothLow = smoothMid = smoothHigh = 0.0f;
    reactiveData = {};
    ready = true;


    //----------------------------------------------------
    // float samples → short PCM 변환 후 재생
    //----------------------------------------------------
    pcm16.resize(samples.size());
    for (size_t i = 0; i < samples.size(); i++)
    {
        float v = samples[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        pcm16[i] = (short)(v * 32767.0f);
    }

    player.play(pcm16.data(), pcm16.size() * sizeof(short), sampleRate);

    return true;
}



/* -------------------------------------------------
   update / FFT 수행 / 대역 계산
   ------------------------------------------------- */
void AudioAnalyzer::update(float dt)
{
    if (!ready || samples.empty()) return;

    int bytesPerSample = sizeof(short);

    currentSample += (size_t)(sampleRate * dt);

    if (currentSample + fftSize >= samples.size())
        currentSample = 0;

    computeFFTWindow();
    computeBands();
}

void AudioAnalyzer::computeFFTWindow()
{
    for (int i = 0; i < fftSize; i++)
    {
        size_t idx = currentSample + i;
        float s = (idx < samples.size()) ? samples[idx] : 0.0f;

        float w = 0.5f * (1.0f - cosf(2.0f * 3.1415926f * i / (fftSize - 1)));
        fftInput[i] = s * w;
    }

    fftwf_execute(fftPlan);
}

void AudioAnalyzer::computeBands()
{
    auto* out = (fftwf_complex*)fftOutput;
    int nBins = fftSize / 2 + 1;
    float binHz = (float)sampleRate / fftSize;

    float low = 0, mid = 0, high = 0;
    float lc = 0, mc = 0, hc = 0;

    for (int b = 1; b < nBins; b++)
    {
        float freq = b * binHz;
        float mag = sqrt(out[b][0] * out[b][0] + out[b][1] * out[b][1]);

        if (freq <= 200) { low += mag; lc++; }
        else if (freq <= 2000) { mid += mag; mc++; }
        else if (freq <= 8000) { high += mag; hc++; }
    }

    if (lc > 0) low /= lc;
    if (mc > 0) mid /= mc;
    if (hc > 0) high /= hc;

    low = clamp01(low * 5.0f);
    mid = clamp01(mid * 5.0f);
    high = clamp01(high * 5.0f);

    const float s = 0.8f;
    smoothLow = s * smoothLow + (1 - s) * low;
    smoothMid = s * smoothMid + (1 - s) * mid;
    smoothHigh = s * smoothHigh + (1 - s) * high;

    reactiveData.lowBand = smoothLow;
    reactiveData.midBand = smoothMid;
    reactiveData.highBand = smoothHigh;

    static float prevLow = 0;
    float diff = smoothLow - prevLow;
    prevLow = smoothLow;

    reactiveData.beatLevel = clamp01(diff * 5.0f);
}
