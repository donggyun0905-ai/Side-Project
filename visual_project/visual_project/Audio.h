#pragma once
#include <vector>
#include <string>
#include <fftw3.h>
#include "AudioPlayer.h"

struct AudioReactiveData
{
    float lowBand;
    float midBand;
    float highBand;
    float beatLevel;
};

class AudioAnalyzer
{
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

    bool loadAudio(const std::string& path, int sampleRate);
    void update(float dt);

    const AudioReactiveData& getReactiveData() const { return reactiveData; }
    bool isReady() const { return ready; }

private:
    void computeFFTWindow();
    void computeBands();

private:
    AudioPlayer player;    // ★ 정상: 딱 1개만 선언

    std::vector<float> samples;
    std::vector<short> pcm16;

    int sampleRate = 44100;
    int fftSize = 1024;
    int hopSize = 512;

    size_t currentSample = 0;

    float* fftInput = nullptr;
    fftwf_complex* fftOutput = nullptr;
    fftwf_plan fftPlan = nullptr;

    AudioReactiveData reactiveData{};
    bool ready = false;

    float smoothLow = 0.0f;
    float smoothMid = 0.0f;
    float smoothHigh = 0.0f;
};
