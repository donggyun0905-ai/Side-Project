# 🎛️ SmartAudio Lab

> **PyQt5 기반 실시간 오디오 이펙트 프로세서**
> WAV 로드부터 파형 시각화, 파라메트릭 EQ, 오토튠까지 한 화면에서 처리하는 미니 오디오 워크스테이션

---

## 💡 프로젝트 개요

SmartAudio Lab은 WAV 오디오 파일을 불러와 여러 DSP(디지털 신호처리) 이펙트를 실시간으로 적용하고, 파형을 시각화하며, 결과를 다시 WAV로 저장할 수 있는 데스크톱 오디오 툴입니다. 이펙트 계산 로직(EQ, 필터, 피치 시프트 등)을 직접 구현하고, 재생/시각화는 별도 스레드로 분리해 UI가 멈추지 않도록 설계했습니다.

---

## ✨ 주요 기능

- **WAV 로드/재생/저장** — 슬라이더로 재생 위치 탐색(seek) 가능
- **실시간 파형 시각화** — `QThread` 기반 별도 스레드에서 파형을 계속 갱신 (`pyqtgraph`)
- **3밴드 파라메트릭 이퀄라이저** — Gain/Q/Freq를 각각 조절 가능한 peaking biquad 필터를 직접 구현
- **콤필터** — 딜레이 기반 피드백 에코 효과
- **입체음향(스테레오 이미징)** — ILD(레벨 차)·ITD(시간차) 기반으로 좌/우 채널을 분리 생성
- **오토튠** — `librosa`의 피치 시프트를 이용한 음정 보정
- **내장 FM 신디사이저** — 건반 클릭 시 FM 합성음을 재생하고, 연주 결과를 현재 오디오에 이어붙여 저장 가능

이펙트들은 하나의 `effects_state`로 관리되며, 켜고 끌 때마다 원본 오디오에 파이프라인 순서대로 재적용됩니다.

---

## 🧱 기술 스택

| 영역 | 기술 |
|---|---|
| GUI | PyQt5, pyqtgraph |
| 오디오 I/O | sounddevice, librosa |
| DSP | NumPy, SciPy (biquad 필터 계수 직접 유도) |

---

## 📁 폴더 구조

```
Smart_Audio_Lab/
├── Audio_Lab.py     # 메인 GUI, 이펙트 체인/재생/시각화 관리
├── equalizer.py     # 3밴드 파라메트릭 EQ (peaking biquad filter)
├── effects.py       # 콤필터
├── stereo.py        # 입체음향 (ILD/ITD)
├── autotune.py      # 피치 시프트 기반 오토튠
├── synthesizer.py   # FM 신디사이저 가상 건반 위젯
└── utils.py         # WAV 저장 유틸
```

---

## 🚀 실행 방법

```bash
pip install PyQt5 pyqtgraph numpy scipy sounddevice librosa
python Audio_Lab.py
```

실행 후 **WAV 불러오기**로 파일을 연 다음, EQ/콤필터/입체음향/오토튠 패널에서 값을 조절하고 **적용** 버튼을 누르면 파형과 재생에 즉시 반영됩니다.
