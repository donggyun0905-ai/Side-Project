# 🚁 Drone Show Simulator

> **OpenGL 기반 실시간 3D 드론 라이트쇼 시뮬레이터**
> 음악을 실시간 분석해 수백 대의 가상 드론이 대형을 이루며 움직이는 쇼를 렌더링합니다

---

## 💡 프로젝트 개요

실제 드론 라이트쇼 제작 파이프라인을 모방한 3D 시뮬레이터입니다. 음악 파일을 FFT로 실시간 분석해 저음/중음/고음 밴드와 비트를 추출하고, 이를 씬 연출(카메라 이동, 반짝임 등)에 반영합니다. 드론의 대형은 코드로 직접 정의하거나(원형, 격자, 텍스트) CSV 좌표 파일로부터 불러올 수 있으며, 겨울왕국 캐릭터(산타/엘사/안나/올라프) 같은 커스텀 포즈 대형도 포함되어 있습니다. 대학교 "비주얼 컴퓨터" 과목 기말 프로젝트로 제작했습니다.

---

## ✨ 주요 기능

- **오디오 반응형 연출** — FFTW로 오디오를 실시간 FFT 분석, 저/중/고음 밴드와 비트 레벨을 추출해 씬에 반영
- **CSV 기반 드론 포메이션** — `snow.csv`, `santa_drones.csv`, `elsa_drones.csv` 등 좌표 데이터를 읽어 대형을 구성 (스케일/오프셋/축 스왑/색상 고정 옵션 지원)
- **코드 기반 대형 생성** — 원형(`setCircle`), 격자(`setGrid`), 2줄 텍스트(`setTextTwoLines`) 대형을 파라미터만으로 생성
- **쇼 타임라인 연출** — `ShowControl`이 시간 흐름에 따라 INTRO → VERSE → CHORUS → OUTRO 섹션을 전환하며 대형 전환 타이밍을 제어
- **자유 카메라 & 셰이더 라이팅** — 키보드로 카메라 조작, Phong 셰이더로 드론 라이팅 처리
- **파트별 드론 애니메이션** — 몸통/팔/손 등 파트를 구분해 캐릭터 포즈의 세부 움직임(팔 흔들기 등) 표현

---

## 🏗 구조

```
visual_project/
├── main.cpp             # GLUT 진입점, 렌더 루프
├── Scene.h/.cpp          # 드론 목록, 카메라, 쇼 타임라인(섹션) 관리
├── Drone.h/.cpp          # 개별 드론 상태(위치/색상/파트)와 업데이트
├── Formation.h/.cpp      # 대형 생성 로직 (원형/격자/텍스트/CSV/캐릭터 포즈)
├── ShowControl.h/.cpp    # 시간 기반 연출 시퀀스 제어
├── Camera.h/.cpp         # 카메라 이동/회전
├── Controller.h/.cpp     # 키보드 입력 → 카메라 제어
├── Audio.h/.cpp          # FFTW 기반 실시간 오디오 분석 (저/중/고음, 비트)
├── AudioPlayer.h/.cpp    # WAV 재생 (dr_wav)
├── shader_loader.h/.cpp  # GLSL 셰이더 로드
└── shaders/              # drone_phong.vert / .frag
```

---

## 🧱 기술 스택

| 영역 | 기술 |
|---|---|
| 그래픽스 | OpenGL, GLUT/freeglut, GLEW, GLM |
| 오디오 분석 | FFTW3 |
| 오디오 로딩 | dr_wav (헤더 온리) |
| 빌드 | Visual Studio 2022 (MSVC v143, x64), NuGet (nupengl.core, glm) |

---

## 🚀 빌드 및 실행 방법

Visual Studio 2022에서 `visual_project.sln`을 열고 `x64 | Debug`(또는 Release) 구성으로 빌드하면 NuGet 패키지(nupengl.core, glm)가 자동 복원됩니다.

```powershell
# 또는 MSBuild로 직접 빌드
MSBuild visual_project.sln /p:Configuration=Debug /p:Platform=x64 /t:Restore,Build
```

빌드 결과물은 `x64/Debug/visual_project.exe`에 생성되며, 같은 폴더의 음악(WAV)·CSV 대형 데이터·셰이더·DLL이 함께 있어야 정상 실행됩니다.

**조작법**: 방향키/특수키로 카메라 조작, `ESC`로 종료
