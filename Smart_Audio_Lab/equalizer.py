import numpy as np
import scipy.signal

def apply_peaking_eq(audio, fs, freq, gain_db, q=1.0):
    A = 10 ** (gain_db / 40)
    omega = 2 * np.pi * freq / fs
    alpha = np.sin(omega) / (2 * q)

    b0 = 1 + alpha * A
    b1 = -2 * np.cos(omega)
    b2 = 1 - alpha * A
    a0 = 1 + alpha / A
    a1 = -2 * np.cos(omega)
    a2 = 1 - alpha / A

    b = np.array([b0, b1, b2]) / a0
    a = np.array([1, a1 / a0, a2 / a0])

    filtered_audio = scipy.signal.lfilter(b, a, audio)
    return filtered_audio


def apply_parametric_eq(audio, fs, eq_params):
    """
    eq_params: list of (gain, freq, q)
    """
    output = audio.copy()
    for gain, freq, q in eq_params:
        output = apply_peaking_eq(output, fs, freq, gain, q)
    return output
