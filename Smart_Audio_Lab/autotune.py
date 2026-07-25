import numpy as np
import librosa

def auto_tune(audio, fs, tune_strength=1.0):
    pitch_shift = tune_strength * 2 
    tuned_audio = librosa.effects.pitch_shift(audio, sr=fs, n_steps=pitch_shift)

    if len(tuned_audio) < len(audio):
        tuned_audio = np.pad(tuned_audio, (0, len(audio) - len(tuned_audio)))
    elif len(tuned_audio) > len(audio):
        tuned_audio = tuned_audio[:len(audio)]
    
    return tuned_audio

