import numpy as np

def comb_filter(audio, delay_samples=500, feedback=0.7):
    output = np.zeros_like(audio)
    for n in range(len(audio)):
        output[n] = audio[n]
        if n >= delay_samples:
            output[n] += feedback * output[n - delay_samples]
    return output
