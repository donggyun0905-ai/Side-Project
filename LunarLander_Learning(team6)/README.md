# 🚀 LunarLander Neuroevolution (team6)

> **유전 알고리즘으로 신경망을 진화시켜 LunarLander를 착륙시키는 뉴로에볼루션 프로젝트**
> 강화학습 라이브러리 없이 선택·교차·돌연변이 연산자를 직접 구현해 학습

---

## 💡 프로젝트 개요

"인공지능" 수업 팀 프로젝트(team6, 팀장 담당)입니다. Gymnasium의 `LunarLander-v3` 환경에서, PPO 같은 강화학습 라이브러리 대신 **신경망 가중치 자체를 유전자로 취급해 진화시키는 뉴로에볼루션(Neuroevolution)** 방식으로 착륙선을 학습시켰습니다. 리워드 셰이핑 설계, 유전 연산자 구현, 커리큘럼 학습 로직을 직접 맡아 진행했습니다.

최종 결과: **평균 보상 271.72, 착륙 성공률 100%** (VAL1000 검증 기준)

---

## ✨ 주요 기능

- **신경망 구조**: 8(입력) → 16 → 8 → 4(출력), 활성화 함수 tanh, 128개체 × 2000세대
- **선택 연산자 3종**: 토너먼트 / 룰렛휠 / 랭크 기반 선택 (`--selection` 옵션으로 전환)
- **균등 교차 + 가우시안 돌연변이**: 세대가 진행될수록 돌연변이 강도(σ)를 점진적으로 감소시키는 어닐링 스케줄 적용
- **정교한 리워드 셰이핑**: 착륙 목표 지점을 삼각형→외부 원기둥→내부 원기둥으로 단계화해 중앙 착륙을 유도하고, 속도·각도·드리프트·저고도 수평이동에 각각 패널티 부여
- **커리큘럼 학습**: 실패한 시드를 자동 수집해 다음 세대 평가 세트에 20% 섞어 넣어 약점을 집중 보완
- **이중 평가 체계**: 학습용 shaped reward와 채점 기준과 동일한 raw reward(`evaluate_raw`)를 분리해 리워드 해킹 방지
- **재현성/검증**: 1000개 시드로 구성된 별도 VAL 세트로 주기적 검증, 개선 시에만 best gene 갱신 및 자동 백업(`_prev.pkl`)
- **멀티프로세싱 지원**: `--workers`로 개체 적합도 평가를 병렬화
- **학습/제출 분리**: 학습 스크립트(`LunarLander_Learning(team6).py`)와 학습된 유전자를 그대로 담은 추론 전용 제출 스크립트(`decideOutput_function(team6).py`)를 분리 생성

---

## 🧱 기술 스택

| 영역 | 기술 |
|---|---|
| 환경 | Gymnasium (`LunarLander-v3`) |
| 알고리즘 | 유전 알고리즘 (Neuroevolution), NumPy 기반 순전파 |
| 병렬화 | Python `multiprocessing` |

---

## 🚀 실행 방법

```bash
pip install gymnasium[box2d] numpy

# 처음부터 학습 (기본 2000세대, roulette 선택)
python "LunarLander_Learning(team6).py"

# 이전 결과 이어서 학습 (워밍스타트) + 병렬 평가
python "LunarLander_Learning(team6).py" --resume best_gene_team6.pkl --workers 8

# 저장된 최우수 유전자로 채점 테스트
python "LunarLander_Learning(team6).py" --test --testcount 1000

# 실패해도 N회 전부 돌려서 전체 평균/성공률 산출 (실패 seed 수집)
python "LunarLander_Learning(team6).py" --test_all --testcount 1000
```

학습이 끝나면 `best_gene_team6.pkl`(최우수 유전자)과 `decideOutput_function(team6).py`(제출용 추론 스크립트)가 갱신됩니다.
