import numpy as np
import sounddevice as sd
from PyQt5.QtWidgets import QWidget, QPushButton

class SynthWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__()
        self.parent = parent

        self.fs = 44100
        self.duration = 1.0
        self.mod_freq = 100
        self.mod_index = 2.0
        self.amplitude = 0.8

        self.note_freqs = {
            'C': 261.63, 'C#': 277.18, 'D': 293.66, 'D#': 311.13,
            'E': 329.63, 'F': 349.23, 'F#': 369.99, 'G': 392.00,
            'G#': 415.30, 'A': 440.00, 'A#': 466.16, 'B': 493.88
        }

        self.init_ui()

    def init_ui(self):
        self.setFixedSize(400, 200)
        white_notes = ['C', 'D', 'E', 'F', 'G', 'A', 'B']
        black_notes = {'C#': 1, 'D#': 2, 'F#': 4, 'G#': 5, 'A#': 6}
        key_width = 50

        for i, note in enumerate(white_notes):
            btn = QPushButton(note, self)
            btn.setGeometry(i * key_width, 50, key_width, 150)
            btn.setStyleSheet("background-color: white; border: 1px solid black;")
            btn.clicked.connect(lambda _, n=note: self.play_note(n))

        for note, pos in black_notes.items():
            btn = QPushButton("", self)
            btn.setGeometry(pos * key_width - 15, 50, 30, 90)
            btn.setStyleSheet("background-color: black;")
            btn.clicked.connect(lambda _, n=note: self.play_note(n))

    def fm_synthesis(self, carrier_freq):
        t = np.linspace(0, self.duration, int(self.fs * self.duration), endpoint=False)
        mod_signal = np.sin(2 * np.pi * self.mod_freq * t)
        signal = self.amplitude * np.sin(2 * np.pi * carrier_freq * t + self.mod_index * mod_signal)
        return signal

    def play_note(self, note):
        carrier_freq = self.note_freqs[note]
        audio = self.fm_synthesis(carrier_freq)
        sd.play(audio, self.fs)

        if self.parent:
            self.parent.add_synth_audio(audio)
