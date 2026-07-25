import sys
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QPushButton, QFileDialog, QLabel, QHBoxLayout, QGroupBox, QSlider
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal

import numpy as np
import librosa
import pyqtgraph as pg
import sounddevice as sd
from equalizer import apply_parametric_eq
from equalizer import apply_peaking_eq
from PyQt5.QtWidgets import QFrame
from effects import comb_filter
from stereo import stereo_effect
from autotune import auto_tune
from synthesizer import SynthWidget
from utils import save_wav


class VisualizerThread(QThread):
    waveform_ready = pyqtSignal(np.ndarray)

    def __init__(self, audio=None, fs=44100, start_pos=0):
        super().__init__()
        self.audio = audio
        self.fs = fs
        self.running = True
        self.current_frame = start_pos

    def run(self):
        while self.running:
            if self.audio is not None:
                start = self.current_frame
                end = min(start + 22050, len(self.audio))
                segment = self.audio[start:end]
                self.current_frame += 2205
                if len(segment) < 1024:
                    continue
                self.waveform_ready.emit(segment[:5000])
            self.msleep(50)

    def update_audio(self, audio, fs):
        self.audio = audio
        self.fs = fs
        self.current_frame = 0


class SmartAudioLab(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SmartAudio Lab")
        self.setGeometry(100, 100, 1400, 900)

        self.audio = None
        self.original_audio = None
        self.processed_audio = None
        self.fs = 44100
        self.stream = None
        self.play_cursor = 0
        self.visual_thread = None

        self.effects_state = {
            "eq": None,
            "comb": False,
            "stereo": None,
            "autotune": None
        }

        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()

        top_layout = QHBoxLayout()
        load_btn = QPushButton("WAV 불러오기")
        load_btn.clicked.connect(self.load_file)
        top_layout.addWidget(load_btn)

        self.play_btn = QPushButton("▶ 재생")
        self.play_btn.setCheckable(True)
        self.play_btn.clicked.connect(self.toggle_play)
        top_layout.addWidget(self.play_btn)

        save_btn = QPushButton("WAV 저장")
        save_btn.clicked.connect(self.save_file)
        top_layout.addWidget(save_btn)

        layout.addLayout(top_layout)

        self.status_label = QLabel("상태: 준비됨")
        layout.addWidget(self.status_label)

        self.wave_plot = pg.PlotWidget(title="Waveform")
        self.wave_plot.setYRange(-1, 1)
        layout.addWidget(self.wave_plot)
        
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setEnabled(False)
        self.slider.sliderReleased.connect(self.slider_released)
        layout.addWidget(self.slider)
        self.slider.sliderPressed.connect(self.slider_pressed)


        # 파라메트릭 EQ
        eq_group = QGroupBox("파라메트릭 이퀄라이저 (3밴드)")
        eq_layout = QHBoxLayout()
        
        self.eq_gain_sliders = []
        self.eq_q_sliders = []
        self.eq_freq_sliders = []
        
        # Gain
        for band in ['Low', 'Mid', 'High']:
            vbox = QVBoxLayout()
            label = QLabel(f"{band} GAIN")
            label.setFixedWidth(80)
            label.setAlignment(Qt.AlignCenter)
            slider = QSlider(Qt.Vertical)
            slider.setMinimum(-10)
            slider.setMaximum(10)
            slider.setValue(0)
            vbox.addWidget(label)
            vbox.addWidget(slider)
            self.eq_gain_sliders.append(slider)
        
            frame = QFrame()
            frame.setFrameShape(QFrame.Box)
            frame.setLineWidth(1)
            frame.setLayout(vbox)
            eq_layout.addWidget(frame)
        
        #  Q
        for band in ['Low', 'Mid', 'High']:
            vbox = QVBoxLayout()
            label = QLabel(f"{band} Q")
            label.setFixedWidth(80)
            label.setAlignment(Qt.AlignCenter)
            slider = QSlider(Qt.Vertical)
            slider.setMinimum(1)
            slider.setMaximum(10)
            slider.setValue(1)
            vbox.addWidget(label)
            vbox.addWidget(slider)
            self.eq_q_sliders.append(slider)
        
            frame = QFrame()
            frame.setFrameShape(QFrame.Box)
            frame.setLineWidth(1)
            frame.setLayout(vbox)
            eq_layout.addWidget(frame)
        
        # FREQ
        freq_defaults = [100, 1000, 10000]
        for idx, band in enumerate(['Low', 'Mid', 'High']):
            vbox = QVBoxLayout()
            label = QLabel(f"{band} FREQ")
            label.setFixedWidth(80)
            label.setAlignment(Qt.AlignCenter)
            slider = QSlider(Qt.Vertical)
            slider.setMinimum(20)
            slider.setMaximum(20000)
            slider.setValue(freq_defaults[idx])
            vbox.addWidget(label)
            vbox.addWidget(slider)
            self.eq_freq_sliders.append(slider)
        
            frame = QFrame()
            frame.setFrameShape(QFrame.Box)
            frame.setLineWidth(1)
            frame.setLayout(vbox)
            eq_layout.addWidget(frame)
        
        # EQ 적용/해제 버튼
        button_layout = QVBoxLayout()
        eq_btn = QPushButton("EQ 적용")
        eq_btn.clicked.connect(self.apply_eq)
        button_layout.addWidget(eq_btn)
        
        eq_reset = QPushButton("EQ 해제")
        eq_reset.clicked.connect(self.reset_eq)
        button_layout.addWidget(eq_reset)
        
        eq_layout.addLayout(button_layout)
        eq_group.setLayout(eq_layout)
        layout.addWidget(eq_group)


        # 콤필터
        comb_btn = QPushButton("콤필터 적용 (로딩이 있습니다. 기다려주세요.)")
        comb_btn.clicked.connect(self.apply_comb_filter)
        layout.addWidget(comb_btn)

          # 입체음향
        stereo_group = QGroupBox("입체음향")
        stereo_layout = QHBoxLayout()
        
        stereo_slider_layout = QVBoxLayout()
        self.stereo_slider = QSlider(Qt.Vertical)
        self.stereo_slider.setMinimum(0)
        self.stereo_slider.setMaximum(20)
        self.stereo_slider.setValue(6)
        stereo_slider_layout.addWidget(QLabel("ILD (dB)"))
        stereo_slider_layout.addWidget(self.stereo_slider)
        
        stereo_button_layout = QVBoxLayout()
        stereo_apply = QPushButton("입체음향 적용")
        stereo_apply.clicked.connect(self.apply_stereo)
        stereo_button_layout.addWidget(stereo_apply)
        
        stereo_reset = QPushButton("입체음향 해제")
        stereo_reset.clicked.connect(self.reset_stereo)
        stereo_button_layout.addWidget(stereo_reset)
        
        stereo_layout.addLayout(stereo_slider_layout)
        stereo_layout.addLayout(stereo_button_layout)
        
        stereo_group.setLayout(stereo_layout)
        layout.addWidget(stereo_group)
        # 오토튠
        autotune_group = QGroupBox("오토튠")
        autotune_layout = QHBoxLayout()
        
        autotune_slider_layout = QVBoxLayout()
        self.tune_slider = QSlider(Qt.Vertical)
        self.tune_slider.setMinimum(0)
        self.tune_slider.setMaximum(100)
        self.tune_slider.setValue(50)
        autotune_slider_layout.addWidget(QLabel("Tune 강도 (%)"))
        autotune_slider_layout.addWidget(self.tune_slider)
        
        autotune_button_layout = QVBoxLayout()
        autotune_apply = QPushButton("오토튠 적용")
        autotune_apply.clicked.connect(self.apply_autotune)
        autotune_button_layout.addWidget(autotune_apply)
        
        autotune_reset = QPushButton("오토튠 해제")
        autotune_reset.clicked.connect(self.reset_autotune)
        autotune_button_layout.addWidget(autotune_reset)
        
        autotune_layout.addLayout(autotune_slider_layout)
        autotune_layout.addLayout(autotune_button_layout)
        
        autotune_group.setLayout(autotune_layout)
        layout.addWidget(autotune_group)
        # 가상악기
        synth_group = QGroupBox("가상 악기 (FM 기반 피아노(테스트용이며 저장에는 포함되지 않습니다.))")
        synth_layout = QVBoxLayout()
        
        synth_widget = SynthWidget(self)
        synth_layout.addWidget(synth_widget)
        
        synth_group.setLayout(synth_layout)
        layout.addWidget(synth_group)

        self.setLayout(layout)
        
        bottom_layout = QHBoxLayout()
        
        bottom_layout.addWidget(eq_group)
        bottom_layout.addWidget(stereo_group)
        bottom_layout.addWidget(autotune_group)
        bottom_layout.addWidget(synth_group)
        
        layout.addLayout(bottom_layout)
        
    def apply_all_effects(self):
        if self.original_audio is None:
            return
        audio = self.original_audio.copy()
    
        # EQ
        if self.effects_state["eq"]:
            eq_params = self.effects_state["eq"]
            audio = apply_parametric_eq(audio, self.fs, eq_params)
    
        # Comb filter
        if self.effects_state["comb"]:
            audio = comb_filter(audio)
    
        # Stereo
        if self.effects_state["stereo"] is not None:
            ild = self.effects_state["stereo"]
            stereo_audio = stereo_effect(audio, self.fs, ild_db=ild)
            audio = np.mean(stereo_audio, axis=1)
    
        # Autotune
        if self.effects_state["autotune"] is not None:
            strength = self.effects_state["autotune"]
            audio = auto_tune(audio, self.fs, tune_strength=strength)
    
        self.processed_audio = audio
        self.start_visualization()

    def add_synth_audio(self, synth_audio):
        if self.processed_audio is None:
            self.processed_audio = synth_audio
        else:
            self.processed_audio = np.concatenate((self.processed_audio, synth_audio))
    
        self.original_audio = self.processed_audio.copy()
        self.slider.setMaximum(len(self.processed_audio))
        self.slider.setEnabled(True)
        self.start_visualization()

    def slider_pressed(self):
        self.stop_playback()
        self.stop_visualization()
        self.play_btn.setChecked(False)
        self.play_btn.setText("▶ 재생")

    def load_file(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "WAV 파일 열기", "", "WAV Files (*.wav)")
        if file_path:
            self.audio, self.fs = librosa.load(file_path, sr=None, mono=True)
            self.original_audio = self.audio.copy()
            self.processed_audio = self.audio.copy()
            self.status_label.setText(f"파일 로드됨: {file_path}")
            self.slider.setMaximum(len(self.processed_audio))
            self.slider.setEnabled(True)
            
    def toggle_play(self):
        if self.processed_audio is None:
            return
    
        if self.play_btn.isChecked():
            self.play_btn.setText("⏸ 일시정지")
    
            self.start_visualization()
            self.start_playback()
        else:
            self.play_btn.setText("▶ 재생")
            self.stop_visualization()
            self.stop_playback()

    def start_visualization(self):
        self.stop_visualization()
        self.visual_thread = VisualizerThread(self.processed_audio, self.fs)
        self.visual_thread.waveform_ready.connect(self.draw_waveform)
        self.visual_thread.start()

    def stop_visualization(self):
        if self.visual_thread:
            self.visual_thread.running = False
            self.visual_thread.quit()
            self.visual_thread.wait()
            self.visual_thread = None

    def start_playback(self):
        def callback(outdata, frames, time, status):
            if self.play_cursor + frames > len(self.processed_audio):
                outdata[:] = np.zeros((frames, 1), dtype=np.float32)
                raise sd.CallbackStop()
            chunk = self.processed_audio[self.play_cursor:self.play_cursor + frames]
            outdata[:] = chunk.reshape(-1, 1)
            self.play_cursor += frames
            self.slider.setValue(self.play_cursor)

        self.stream = sd.OutputStream(samplerate=self.fs, channels=1, dtype='float32', callback=callback)
        self.stream.start()

    def stop_playback(self):
        if self.stream:
            self.stream.stop()
            self.stream.close()
            self.stream = None

    def slider_released(self):
        self.stop_playback()
        self.stop_visualization()
    
        self.play_cursor = self.slider.value()
    
        self.visual_thread = VisualizerThread(self.processed_audio, self.fs, start_pos=self.play_cursor)
        self.visual_thread.waveform_ready.connect(self.draw_waveform)
        self.visual_thread.start()
    
        self.start_playback()
    
        self.play_btn.setChecked(True)
        self.play_btn.setText("⏸ 일시정지")
    def apply_eq(self):
        eq_params = []
        for i in range(3):
            gain = self.eq_gain_sliders[i].value()
            q = self.eq_q_sliders[i].value()
            freq = self.eq_freq_sliders[i].value()
            eq_params.append((gain, freq, q))
        self.effects_state["eq"] = eq_params
        self.apply_all_effects()

    def reset_eq(self):
        self.effects_state["eq"] = None
        self.apply_all_effects()

    def apply_comb_filter(self):
        self.effects_state["comb"] = True
        self.apply_all_effects()

    def apply_stereo(self):
        ild = self.stereo_slider.value()
        self.effects_state["stereo"] = ild
        self.apply_all_effects()

    def reset_stereo(self):
        self.effects_state["stereo"] = None
        self.apply_all_effects()

    def apply_autotune(self):
        strength = self.tune_slider.value() / 100
        self.effects_state["autotune"] = strength
        self.apply_all_effects()

    def reset_autotune(self):
        self.effects_state["autotune"] = None
        self.apply_all_effects()

    def save_file(self):
        file_path, _ = QFileDialog.getSaveFileName(self, "WAV 저장", "", "WAV Files (*.wav)")
        if file_path:
            self.apply_all_effects()
            save_wav(file_path, self.processed_audio, self.fs)

    def draw_waveform(self, segment):
        self.wave_plot.clear()
        self.wave_plot.plot(segment, pen='c')

    def closeEvent(self, event):
        self.stop_visualization()
        self.stop_playback()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = SmartAudioLab()
    win.show()
    sys.exit(app.exec_())
