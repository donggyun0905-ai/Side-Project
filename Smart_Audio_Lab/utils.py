import numpy as np
from scipy.io.wavfile import write

def save_wav(file_path, audio, fs):
    audio_scaled = np.int16(audio / np.max(np.abs(audio)) * 32767)
    write(file_path, fs, audio_scaled)
