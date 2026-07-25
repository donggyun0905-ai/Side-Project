import numpy as np

def stereo_effect(audio, fs, ild_db=6, itd_ms=2):
    ild_gain = 10**(ild_db / 20)
    itd_samples = int(fs * itd_ms / 1000)

    left = np.concatenate([audio, np.zeros(itd_samples)])[:len(audio)]
    right = np.concatenate([np.zeros(itd_samples), audio])[:len(audio)] / ild_gain

    stereo_audio = np.vstack([left, right]).T
    return stereo_audio
