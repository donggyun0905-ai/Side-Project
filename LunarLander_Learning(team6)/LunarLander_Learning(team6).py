# -*- coding: utf-8 -*-
# =====================================================================
#  LunarLander_Learning(team6).py
#  인공지능 프로젝트 : 신경망 가중치를 유전자 알고리즘으로 진화시키는
#  진화 학습(Neuroevolution) 으로 LunarLander-v3 착륙선을 학습한다.
#
#  - 신경망 : 8(입력) -> 16 -> 8 -> 4(출력), 활성화 함수 tanh
#  - 선택   : 토너먼트 / 룰렛휠 / 랭크 기반 (--selection 으로 지정)
#  - 교차   : 균등 교차(Uniform crossover)
#  - 돌연변이: 가우시안 돌연변이, 유전자별 확률 20%→5% 점진 감소 (5%~40% 범위 내)
#  - 엘리트 : 2개 보존
#  - 개체수 : 128
#  - 세대수 : 2000 (필요 시 여러 번 재실행하여 더 우수한 유전자 탐색)
#
#  Reward Shaping (계획서 반영):
#   + Triangle Zone Score  : 삼각형 존 내부 스텝당 +0.10
#   + Cylinder Core Score  : 원기둥 코어 내부 스텝당 +0.30 추가
#   - Speed Penalty        : 착륙 직전(y<0.3) 과속 시 감점
#   - Angle Penalty        : 공중 비행 중 기울기 편차 감점
#   + Time Bonus           : 200스텝 이내 착륙 시 최대 +100
#   + Center Bonus         : x=0 정중앙 착륙 시 최대 +50
#   - Crash Penalty        : 추락 1회당 -120
#
#  실행 예)
#     python "LunarLander_Learning(team6).py"
#     python "LunarLander_Learning(team6).py" --resume best_gene_team6.pkl --workers 8
#     python "LunarLander_Learning(team6).py" --resume best_gene_team6.pkl --mut_start 0.10 --mut_end 0.05 --stagnation_boost 0
# =====================================================================

import os
import pickle
import argparse
import numpy as np
import gymnasium as gym

# ---------------------------------------------------------------------
# 0. 전역 하이퍼파라미터 (요구 제약 사항 반영)
# ---------------------------------------------------------------------
LAYER_SIZES   = [8, 16, 8, 4]   # 입력8 - 은닉16 - 은닉8 - 출력4
POP_SIZE      = 128             # 세대당 유전자 수 (요구사항: 128)
ELITE         = 2               # 엘리트 보존 (요구사항: 2)
GENERATIONS   = 2000            # 진화 세대수 (요구사항: 2000 제한)
TOURNAMENT_K  = 4               # 토너먼트 선택 크기
MUTATION_PROB_START = 0.40      # 돌연변이 확률 초기값 (1회차 기본: 40%)
MUTATION_PROB_END   = 0.40      # 돌연변이 확률 말기값 (1회차 기본: 40% 고정)
SIGMA_START   = 0.6             # 돌연변이 가우시안 표준편차(초기)
SIGMA_END     = 0.05            # 돌연변이 가우시안 표준편차(말기)
INIT_RANGE    = 1.0             # 초기 가중치 범위 [-1, 1]
WEIGHT_CLIP   = 5.0             # 가중치 클리핑 범위 [-5, 5]

# 학습용 적응도 평가 설정
EVAL_EPISODES_START = 8         # 초기 세대: 개체당 평가 에피소드 수
EVAL_EPISODES_MAX   = 16        # 말기 세대: 개체당 평가 에피소드 수
CRASH_PENALTY       = 120.0     # 착륙 실패 1회당 추가 벌점
MAX_STEPS           = 1000      # 한 에피소드 최대 스텝
TIME_BONUS_STEPS    = 200       # 이 스텝 이내 착륙 시 보너스 적용
TIME_BONUS_MAX      = 120.0     # 최대 타임 보너스
CENTER_BONUS_MAX    =  50.0     # 최대 정중앙 착륙 보너스
ROBUST_EVAL_INTERVAL = 100      # robust_eval 실행 주기(세대) — VAL1000 사용으로 확대
STAGNATION_THRESH   = 300       # 이 세대 이상 개선 없으면 정체기 판정
STAGNATION_BOOST    = 0.15      # 정체기 시 돌연변이 확률 추가 (+15%)

# ── Reward Shaping 파라미터 (계획서 반영) ──────────────────────────
TRIANGLE_START_HEIGHT = 1.4    # 삼각형 존 꼭짓점 높이 (착륙선 초기 고도)
FLAG_X            = 0.20       # 깃발 x 절댓값
CYLINDER_OUTER    = 0.12       # 외부 원기둥 반경 (착륙패드 폭 60%)
CYLINDER_RADIUS   = 0.05       # 내부 원기둥 반경 (중앙 코어)
TRI_STEP_BONUS    = 0.10       # 삼각형 존 내부 스텝당 보상
CYL_OUTER_BONUS   = 0.15       # 외부 원기둥 추가 보너스/step
CYL_STEP_BONUS    = 0.25       # 내부 원기둥 추가 보너스/step (외부+내부 합산 +0.40)
SPEED_Y_THRESH    = 0.30       # 속도 패널티 적용 고도 기준
SPEED_VX_LIMIT    = 0.50       # 수평 속도 허용 한계
SPEED_VY_LIMIT    = 1.00       # 수직 속도 허용 한계
SPEED_PEN_SCALE   = 30.0       # 속도 초과 패널티 배율
ANGLE_PEN_SCALE   = 2.0        # 각도 편차 패널티 배율 (높이 비례)
ANG_VEL_PEN_SCALE = 1.0        # 각속도 패널티 배율 (항상 적용)
MAIN_ENG_PEN      = 0.3        # 고고도 메인 엔진 추가 패널티 배율 (높이 비례, 자유낙하 유도)
LOW_ALT_HVX_PEN   = 5.0        # 저고도(py<0.5) 수평 속도 패널티 (비스듬 진입 억제)
# ─────────────────────────────────────────────────────────────────────

# 산출물
BEST_PICKLE        = "best_gene_team6.pkl"
ROBUST_PICKLE      = "best_gene_team6_robust.pkl"
FUNC_FILE          = "decideOutput_function(team6).py"
FAILURE_SEEDS_FILE = "failure_seeds.pkl"

# 가중치(유전자) 길이 계산
def gene_length(sizes):
    n = 0
    for i in range(len(sizes) - 1):
        n += sizes[i] * sizes[i + 1]
        n += sizes[i + 1]
    return n

GENE_LEN = gene_length(LAYER_SIZES)


# ---------------------------------------------------------------------
# 1. 입력 정규화
# ---------------------------------------------------------------------
CordNorm        = 2.5
LinearSpeedNorm = 10.0
AngleNorm       = 6.2831855
AngularSpeedNorm = 10.0

def normalizeInputs(observe):
    out = []
    out.append(observe[0] / CordNorm)
    out.append(observe[1] / CordNorm)
    out.append(observe[2] / LinearSpeedNorm)
    out.append(observe[3] / LinearSpeedNorm)
    out.append(observe[4] / AngleNorm)
    out.append(observe[5] / AngularSpeedNorm)
    out.append(observe[6])
    out.append(observe[7])
    return out


# ---------------------------------------------------------------------
# 2. 신경망 순전파
# ---------------------------------------------------------------------
def decode_gene(gene, sizes=LAYER_SIZES):
    layers = []
    idx = 0
    g = np.asarray(gene, dtype=np.float64)
    for i in range(len(sizes) - 1):
        n_in, n_out = sizes[i], sizes[i + 1]
        W = g[idx: idx + n_in * n_out].reshape(n_in, n_out); idx += n_in * n_out
        b = g[idx: idx + n_out];                              idx += n_out
        layers.append((W, b))
    return layers


def forward(x, layers):
    for (W, b) in layers:
        x = np.tanh(x @ W + b)
    return x

def _action(obs, layers):
    return int(np.argmax(forward(np.asarray(normalizeInputs(obs), dtype=np.float64), layers)))


def decideOutput(observation, Gene):
    x = np.asarray(normalizeInputs(observation), dtype=np.float64)
    layers = decode_gene(Gene, LAYER_SIZES)
    out = forward(x, layers)
    return int(np.argmax(out))


# ---------------------------------------------------------------------
# 3. 적응도(fitness) 평가  ── Reward Shaping 포함
# ---------------------------------------------------------------------
def evaluate_gene(gene, seeds, env=None):
    layers = decode_gene(gene, LAYER_SIZES)
    own_env = False
    if env is None:
        env = gym.make("LunarLander-v3", render_mode=None)
        own_env = True

    returns = []
    lands = 0
    for sd in seeds:
        obs, _ = env.reset(seed=int(sd))
        sm = 0.0
        steps = 0
        last_r = 0.0
        while True:
            prev_obs = obs  # 행동 결정 전 상태 저장 (착지 후 엔진 패널티 판정용)
            action = _action(obs, layers)
            obs, r, term, trunc, _ = env.step(action)
            sm += r
            last_r = r
            steps += 1

            # ── Reward Shaping ──────────────────────────────────────
            px      = float(obs[0])
            py      = float(obs[1])
            vx      = float(obs[2])
            vy      = float(obs[3])
            angle   = float(obs[4])
            ang_vel = float(obs[5])

            # Triangle Zone Score: 200스텝 이내에만 적용 (이후 맴돌기 방지)
            # 중앙에 가까울수록 계단식 보너스: 삼각형→외부원기둥→내부원기둥
            if steps <= TIME_BONUS_STEPS and 0.0 <= py <= TRIANGLE_START_HEIGHT:
                half_w = FLAG_X * (1.0 - py / TRIANGLE_START_HEIGHT)
                if abs(px) <= half_w:
                    sm += TRI_STEP_BONUS          # 삼각형 내부: +0.10
                    if abs(px) <= CYLINDER_OUTER:
                        sm += CYL_OUTER_BONUS     # 외부 원기둥: +0.15 (합계 +0.25)
                    if abs(px) <= CYLINDER_RADIUS:
                        sm += CYL_STEP_BONUS      # 내부 원기둥: +0.25 (합계 +0.50)

            # Speed Penalty: 착륙 직전 고도에서 과속 시 감점
            if py < SPEED_Y_THRESH:
                excess = (max(0.0, abs(vx) - SPEED_VX_LIMIT)
                          + max(0.0, abs(vy) - SPEED_VY_LIMIT))
                sm -= excess * SPEED_PEN_SCALE

            # 착지 직전 추가 속도 패널티: py<0.15 구간에서 아래로 빠르게 내려올 때만 감점
            if py < 0.15 and vy < -0.5:
                sm -= (abs(vy) - 0.5) * 20.0

            # 저고도 수평 이동 패널티: 비스듬 진입 억제, 수직 접근 유도
            if py < 0.5:
                sm -= abs(vx) * LOW_ALT_HVX_PEN

            # Angle Penalty: 높이 비례 각도 패널티 + 항상 각속도 패널티 (최소 20% 유지)
            height_factor = max(0.2, min(1.0, py / TRIANGLE_START_HEIGHT))
            sm -= abs(angle)   * ANGLE_PEN_SCALE   * height_factor
            sm -= abs(ang_vel) * ANG_VEL_PEN_SCALE

            # Main Engine Penalty: 고고도에서 중력 이용 유도 (단, 급낙하 중 비상 브레이킹은 면제)
            if action == 2 and vy > -(SPEED_VY_LIMIT * 1.5):
                sm -= py * MAIN_ENG_PEN
                if py > TRIANGLE_START_HEIGHT:  # 삼각형 위(py>1.4): 추가 억제
                    sm -= (py - TRIANGLE_START_HEIGHT) * 1.5
            elif py > TRIANGLE_START_HEIGHT:  # 삼각형 위에서 메인 안 쓰면 소량 보너스
                sm += (py - TRIANGLE_START_HEIGHT) * 0.2

            # Descent Bonus: 고도에서 아래로 내려오는 속도에 소량 보너스 (하강 유도)
            if py > SPEED_Y_THRESH and vy < 0:
                sm += min(0.1, -vy * 0.05)

            # Drift Penalty: 패드 반대 방향으로 멀어지면 감점 (속도가 중심을 향해야 함)
            if py > SPEED_Y_THRESH:
                sm -= min(0.5, max(0.0, px * vx)) * 2.0

            # 착지 후 엔진 재점화 패널티: 안정 착지 후만 적용, 충격 흡수(|vy|≥0.5)는 면제
            if float(prev_obs[6]) > 0.5 and float(prev_obs[7]) > 0.5 and action != 0:
                if abs(vy) < 0.5:
                    sm -= 10.0
            # ────────────────────────────────────────────────────────

            if term or trunc or steps >= MAX_STEPS:
                break

        if last_r >= 99.9:
            lands += 1
            time_bonus   = max(0.0, (TIME_BONUS_STEPS - steps) / TIME_BONUS_STEPS) * TIME_BONUS_MAX
            center_bonus = max(0.0, 1.0 - abs(float(obs[0])) / 0.2) * CENTER_BONUS_MAX
            edge_penalty = min(20.0, max(0.0, abs(float(obs[0])) - 0.15) * 200.0)
            angle_bonus  = max(0.0, 1.0 - abs(float(obs[4])) / 0.2) * 15.0
            sm += time_bonus + center_bonus - edge_penalty + angle_bonus
        returns.append(sm)

    if own_env:
        env.close()

    returns = np.array(returns, dtype=np.float64)
    crashes = len(seeds) - lands
    fitness = float(returns.mean() - CRASH_PENALTY * (crashes / len(seeds)))
    return fitness, float(returns.mean()), lands / len(seeds)


# ---------------------------------------------------------------------
# 3b. 순수 gymnasium 점수 평가 (reward shaping 없음) — 교수님 채점 기준과 동일
# ---------------------------------------------------------------------
def evaluate_raw(gene, seeds, env=None, return_failed=False):
    layers = decode_gene(gene, LAYER_SIZES)
    own_env = False
    if env is None:
        env = gym.make("LunarLander-v3", render_mode=None)
        own_env = True

    returns = []
    lands = 0
    failed_seeds = []
    for sd in seeds:
        obs, _ = env.reset(seed=int(sd))
        sm = 0.0
        last_r = 0.0
        while True:
            action = _action(obs, layers)
            obs, r, term, trunc, _ = env.step(action)
            sm += r
            last_r = r
            if term or trunc:
                break
        if last_r >= 99.9:
            lands += 1
        else:
            failed_seeds.append(int(sd))
        returns.append(sm)

    if own_env:
        env.close()

    returns = np.array(returns, dtype=np.float64)
    if return_failed:
        return float(returns.mean()), lands / len(returns), failed_seeds
    return float(returns.mean()), lands / len(returns)


# 멀티프로세싱용 워커
_WORKER_ENV = None
def _worker_init():
    global _WORKER_ENV
    _WORKER_ENV = gym.make("LunarLander-v3", render_mode=None)

def _worker_eval(args):
    gene, seeds = args
    return evaluate_gene(gene, seeds, env=_WORKER_ENV)


# ---------------------------------------------------------------------
# 4. 유전 연산자 : 선택 / 교차 / 돌연변이
# ---------------------------------------------------------------------
def tournament_select(pop, fits, k=TOURNAMENT_K, rng=None):
    """토너먼트 선택: 무작위 k명 중 적응도 최고를 부모로."""
    idx = rng.integers(0, len(pop), size=k)
    best = idx[0]
    for i in idx[1:]:
        if fits[i] > fits[best]:
            best = i
    return pop[best]

def roulette_select(pop, fits, rng):
    """룰렛 휠 선택: 적합도에 비례한 확률로 선택."""
    shifted = fits - fits.min() + 1e-8
    probs   = shifted / shifted.sum()
    idx     = rng.choice(len(pop), p=probs)
    return pop[idx]

def rank_select(pop, fits, rng):
    """랭크 기반 선택: 순위에 비례한 확률로 선택."""
    order = np.argsort(fits)          # 오름차순 (낮은 적합도 = rank 1)
    n     = len(pop)
    ranks = np.empty(n, dtype=np.float64)
    for pos, i in enumerate(order):
        ranks[i] = pos + 1            # rank 1(최하) ~ n(최상)
    probs = ranks / ranks.sum()
    idx   = rng.choice(n, p=probs)
    return pop[idx]

def select_parent(pop, fits, method, rng):
    if method == "roulette":
        return roulette_select(pop, fits, rng)
    elif method == "rank":
        return rank_select(pop, fits, rng)
    else:
        return tournament_select(pop, fits, TOURNAMENT_K, rng)

def uniform_crossover(p1, p2, rng):
    """균등 교차: 각 유전자를 50% 확률로 부모1/부모2에서 선택."""
    mask = rng.random(len(p1)) < 0.5
    return np.where(mask, p1, p2)

def gaussian_mutate(child, sigma, prob, rng):
    """가우시안 돌연변이: 유전자별 prob 확률로 N(0,sigma) 잡음 추가."""
    mask  = rng.random(len(child)) < prob
    noise = rng.normal(0.0, sigma, size=len(child)) * mask
    return np.clip(child + noise, -WEIGHT_CLIP, WEIGHT_CLIP)


# ---------------------------------------------------------------------
# 5. 메인 진화 루프
# ---------------------------------------------------------------------
def make_seed_master(train_seed=None, size=5000):
    """훈련용 시드 풀 생성.
    train_seed=None  → 완전 랜덤 (매 실행마다 다름, 일반화에 유리)
    train_seed=정수   → 고정 시드 (실행 간 동일 환경, 안정적 비교 가능)
    """
    rng = np.random.default_rng(train_seed)
    return rng.integers(0, 2**31 - 1, size=size).tolist()


def evolve(generations=GENERATIONS, workers=1, seed=12345, resume=None,
           selection="tournament",
           mut_start=MUTATION_PROB_START, mut_end=MUTATION_PROB_END,
           failure_seeds_file=None, stagnation_boost=STAGNATION_BOOST,
           train_seed=None):
    rng = np.random.default_rng(seed)

    pop = [rng.uniform(-INIT_RANGE, INIT_RANGE, size=GENE_LEN) for _ in range(POP_SIZE)]
    if resume and os.path.exists(resume):
        with open(resume, "rb") as f:
            saved = pickle.load(f)
        seed_gene = np.asarray(saved["gene"], dtype=np.float64)
        if len(seed_gene) == GENE_LEN:
            n_warm = POP_SIZE // 3  # 1/3만 warm start → 나머지 2/3는 랜덤 (다양성 확보)
            for i in range(n_warm):
                pop[i] = np.clip(seed_gene + rng.normal(0, 0.3, GENE_LEN), -WEIGHT_CLIP, WEIGHT_CLIP)
            pop[0] = seed_gene.copy()
            print(f"[resume] {resume} 로부터 워밍스타트 (avg_reward={saved.get('avg_reward')}, warm={n_warm}개 σ=0.3)")

    pool = None
    if workers > 1:
        import multiprocessing as mp
        pool = mp.Pool(workers, initializer=_worker_init)

    eval_env = gym.make("LunarLander-v3", render_mode=None)

    best_overall = {"gene": None, "fitness": -1e9, "avg_reward": -1e9, "land_rate": 0.0}
    best_robust  = {"gene": None, "fitness": -1e9, "avg_reward": -1e9, "land_rate": 0.0}

    VAL_SEEDS = np.random.default_rng(999).integers(0, 2**31 - 1, size=1000).tolist()

    seed_master = make_seed_master(train_seed)

    # 커리큘럼 학습: 실패 seed 로드
    failure_seeds = []
    if failure_seeds_file and os.path.exists(failure_seeds_file):
        with open(failure_seeds_file, "rb") as f:
            failure_seeds = pickle.load(f)
        print(f"[커리큘럼] 실패 seed {len(failure_seeds)}개 로드 → 매 세대 평가의 20%에 혼합")

    # 커리큘럼 모드면 검증 seed에도 실패 seed 포함 → #1 pkl이 실패 환경도 잘 푸는 유전자로 갱신됨
    val_seeds_for_eval = list(VAL_SEEDS) + list(failure_seeds)

    def robust_eval(gene):
        # raw 점수로 평가 + 실패 seed 자동 수집
        raw_avg, raw_land, new_failed = evaluate_raw(gene, val_seeds_for_eval, env=eval_env, return_failed=True)
        if new_failed:
            truly_new = [s for s in new_failed if s not in val_seeds_for_eval]
            if truly_new:
                failure_seeds.extend(truly_new)
                val_seeds_for_eval.extend(truly_new)
                existing = []
                if os.path.exists(FAILURE_SEEDS_FILE):
                    with open(FAILURE_SEEDS_FILE, "rb") as _f:
                        existing = pickle.load(_f)
                with open(FAILURE_SEEDS_FILE, "wb") as _f:
                    pickle.dump(list(set(existing + truly_new)), _f)
                print(f"  [커리큘럼 자동] 새 실패 seed {len(truly_new)}개 발견 → 누적 {len(failure_seeds)}개 저장")
        return raw_land, raw_avg

    if resume and os.path.exists(resume):
        lr0, ar0 = robust_eval(pop[0])
        best_overall = {"gene": pop[0].copy(), "fitness": ar0,
                        "avg_reward": ar0, "land_rate": lr0, "gen": -1}

    gens_since_improvement = 0  # 정체기 감지용 카운터

    print(f"[설정] 선택기법={selection}  개체수={POP_SIZE}  엘리트={ELITE}  세대={generations}")
    print(f"[설정] 돌연변이 확률: {mut_start*100:.0f}% → {mut_end*100:.0f}%")
    print(f"[설정] Reward Shaping: Triangle+{TRI_STEP_BONUS}/step, Cylinder+{CYL_STEP_BONUS}/step, "
          f"SpeedPen×{SPEED_PEN_SCALE}, AnglePen×{ANGLE_PEN_SCALE}/step")

    for gen in range(generations):
        frac   = gen / max(1, generations - 1)
        n_eval = int(round(EVAL_EPISODES_START + (EVAL_EPISODES_MAX - EVAL_EPISODES_START) * frac))
        if failure_seeds:
            n_fail = max(1, int(n_eval * 0.20))
            n_rand = n_eval - n_fail
            f_idx  = rng.choice(len(failure_seeds), size=n_fail, replace=True)
            seeds  = [failure_seeds[int(i)] for i in f_idx]
            seeds += [seed_master[i] for i in rng.choice(len(seed_master), size=n_rand, replace=False)]
        else:
            seeds  = [seed_master[i] for i in rng.choice(len(seed_master), size=n_eval, replace=False)]

        if pool is not None:
            results = pool.map(_worker_eval, [(g, seeds) for g in pop])
        else:
            results = [evaluate_gene(g, seeds, env=eval_env) for g in pop]

        fits      = np.array([r[0] for r in results])
        avg_rew   = np.array([r[1] for r in results])
        land_rate = np.array([r[2] for r in results])

        order  = np.argsort(fits)[::-1]
        best_i = order[0]

        val_info = ""
        gens_since_improvement += 1
        if gen % ROBUST_EVAL_INTERVAL == 0 or gen == generations - 1:
            v_land, v_avg = robust_eval(pop[best_i])
            val_info = f"  VAL{len(val_seeds_for_eval)}={v_avg:7.2f}({v_land*100:.1f}%)"
            if v_land >= best_overall["land_rate"] and v_avg > best_overall["avg_reward"]:
                best_overall = {
                    "gene": pop[best_i].copy(),
                    "fitness": float(fits[best_i]),
                    "avg_reward": float(v_avg),
                    "land_rate": float(v_land),
                    "gen": gen,
                }
                save_best(best_overall)
                val_info += " ★NEW"
                gens_since_improvement = 0  # 개선됐으면 리셋
            # 커리큘럼 모드: VAL 기준 100% 착륙 + 최고점 유전자를 robust pkl에 별도 저장
            if failure_seeds and v_land >= 1.0 and v_avg > best_robust["avg_reward"]:
                best_robust = {
                    "gene": pop[best_i].copy(),
                    "fitness": float(fits[best_i]),
                    "avg_reward": float(v_avg),
                    "land_rate": float(v_land),
                    "gen": gen,
                }
                with open(ROBUST_PICKLE, "wb") as _f:
                    pickle.dump(best_robust, _f)
                val_info += " ★ROBUST"

        if gen % 10 == 0 or gen == generations - 1:
            print(f"[Gen {gen:4d}] eval_ep={n_eval:2d}  "
                  f"best_fit={fits[best_i]:8.2f}  best_avgR={avg_rew[best_i]:8.2f}  "
                  f"land={land_rate[best_i]*100:5.1f}%  "
                  f"|  ALL-best avgR={best_overall['avg_reward']:7.2f} "
                  f"land={best_overall['land_rate']*100:5.1f}%"
                  f"{val_info}")

        # 다음 세대 생성
        new_pop  = [pop[i].copy() for i in order[:ELITE]]
        sigma    = SIGMA_START + (SIGMA_END - SIGMA_START) * frac
        mut_prob_base = mut_start + (mut_end - mut_start) * frac
        stagnation_boost_val = stagnation_boost if gens_since_improvement >= STAGNATION_THRESH else 0.0
        mut_prob = min(0.40, mut_prob_base + stagnation_boost_val)
        if stagnation_boost_val > 0 and gen % 50 == 0:
            print(f"  [정체기] {gens_since_improvement}세대 개선 없음 → 돌연변이 {mut_prob_base*100:.0f}%+{stagnation_boost_val*100:.0f}%={mut_prob*100:.0f}%")
        while len(new_pop) < POP_SIZE:
            p1    = select_parent(pop, fits, selection, rng)
            p2    = select_parent(pop, fits, selection, rng)
            child = uniform_crossover(p1, p2, rng)
            child = gaussian_mutate(child, sigma, mut_prob, rng)
            new_pop.append(child)
        pop = new_pop

    if pool is not None:
        pool.close(); pool.join()
    eval_env.close()

    print(f"\n=== 학습 종료 ===  최우수 평균보상={best_overall['avg_reward']:.2f}  "
          f"착륙률={best_overall['land_rate']*100:.1f}%  (gen {best_overall.get('gen')})")
    if failure_seeds and best_robust["gene"] is not None:
        print(f"=== robust 저장 ===  혼합평가 최고={best_robust['avg_reward']:.2f}  "
              f"착륙률={best_robust['land_rate']*100:.1f}%  (gen {best_robust.get('gen')})  → {ROBUST_PICKLE}")
    return best_overall


# ---------------------------------------------------------------------
# 6. 산출물 저장
# ---------------------------------------------------------------------
def save_best(best):
    # 덮어쓰기 전 자동 백업
    if os.path.exists(BEST_PICKLE):
        import shutil
        shutil.copy2(BEST_PICKLE, BEST_PICKLE.replace(".pkl", "_prev.pkl"))
    with open(BEST_PICKLE, "wb") as f:
        pickle.dump(best, f)
    write_func_file(best["gene"], best["avg_reward"], best["land_rate"])

def write_func_file(gene, avg_reward, land_rate):
    gene_list = [round(float(w), 8) for w in gene]
    body = f'''# -*- coding: utf-8 -*-
# =====================================================================
#  decideOutput_function(team6).py
#  학습된 최우수 Gene + decideOutput() 함수 (제출용)
#  (LunarLander_Learning(team6).py 의 decideOutput 과 완전히 동일)
#
#  VAL 검증 평균 raw reward = {avg_reward:.2f},  착륙 성공률 = {land_rate*100:.1f}%
#  신경망 구조 : {LAYER_SIZES} , 활성화 함수 : tanh
# =====================================================================
import numpy as np

LAYER_SIZES = {LAYER_SIZES}

CordNorm = 2.5
LinearSpeedNorm = 10.0
AngleNorm = 6.2831855
AngularSpeedNorm = 10.0

def normalizeInputs(observe):
    out = []
    out.append(observe[0] / CordNorm)
    out.append(observe[1] / CordNorm)
    out.append(observe[2] / LinearSpeedNorm)
    out.append(observe[3] / LinearSpeedNorm)
    out.append(observe[4] / AngleNorm)
    out.append(observe[5] / AngularSpeedNorm)
    out.append(observe[6])
    out.append(observe[7])
    return out

def _decode(gene, sizes=LAYER_SIZES):
    layers = []
    idx = 0
    g = np.asarray(gene, dtype=np.float64)
    for i in range(len(sizes) - 1):
        n_in, n_out = sizes[i], sizes[i + 1]
        W = g[idx: idx + n_in * n_out].reshape(n_in, n_out); idx += n_in * n_out
        b = g[idx: idx + n_out];                              idx += n_out
        layers.append((W, b))
    return layers

def decideOutput(observation, Gene):
    x = np.asarray(normalizeInputs(observation), dtype=np.float64)
    for (W, b) in _decode(Gene, LAYER_SIZES):
        x = np.tanh(x @ W + b)
    return int(np.argmax(x))

# ----- 학습된 최우수 유전자 -----
Gene = {gene_list}
'''
    with open(FUNC_FILE, "w", encoding="utf-8") as f:
        f.write(body)


# ---------------------------------------------------------------------
# 7. 테스트 : 교수님 runGame 방식 (실패 시 즉시 중단, 성공만 평균)
# ---------------------------------------------------------------------
def run_competition_test(gene, testcount=1000, render=False):
    layers = decode_gene(gene, LAYER_SIZES)
    env = gym.make("LunarLander-v3", render_mode="human" if render else None)
    total = 0.0
    num = 0
    while True:
        sm = 0.0
        obs, _ = env.reset()
        last_r = 0.0
        while True:
            action = _action(obs, layers)
            obs, r, term, trunc, _ = env.step(action)
            sm += r
            last_r = r
            if term or trunc:
                if last_r >= 99.9:
                    total += sm
                    num += 1
                    print(num, " Final Fitness:", round(sm, 2))
                else:
                    print("Landing failed.....",  "(성공 횟수:", num, ")")
                    env.close()
                    return (total / num) if num > 0 else 0.0, num
                break
        if num >= testcount:
            break
    env.close()
    avg = total / num if num > 0 else 0.0
    print("Average Fitness :", avg, " / 성공 횟수:", num)
    return avg, num


# ---------------------------------------------------------------------
# 8. 테스트 : 실패해도 N회 전부 돌려서 전체 평균 산출
# ---------------------------------------------------------------------
def run_all_test(gene, testcount=1000, render=False, quiet=False):
    layers = decode_gene(gene, LAYER_SIZES)
    env = gym.make("LunarLander-v3", render_mode=None)
    rng = np.random.default_rng()
    scores = []
    success_scores = []
    failure_seeds = []

    for i in range(testcount):
        seed = int(rng.integers(0, 2**31 - 1))
        obs, _ = env.reset(seed=seed)
        sm = 0.0
        last_r = 0.0
        while True:
            action = _action(obs, layers)
            obs, r, term, trunc, _ = env.step(action)
            sm += r
            last_r = r
            if term or trunc:
                break
        scores.append(sm)
        if last_r >= 99.9:
            success_scores.append(sm)
            if not quiet:
                print(f"{i+1:4d}  Final Fitness: {round(sm, 2)}  O")
        else:
            failure_seeds.append(seed)
            print(f"{i+1:4d}  Final Fitness: {round(sm, 2)}  X FAIL  (seed={seed})")
            if render:
                r_env = gym.make("LunarLander-v3", render_mode="human")
                obs_r, _ = r_env.reset(seed=seed)
                while True:
                    obs_r, r, term, trunc, _ = r_env.step(_action(obs_r, layers))
                    if term or trunc:
                        break
                r_env.close()

    env.close()

    if failure_seeds:
        existing = []
        if os.path.exists(FAILURE_SEEDS_FILE):
            with open(FAILURE_SEEDS_FILE, "rb") as f:
                existing = pickle.load(f)
        merged = list(set(existing + failure_seeds))
        with open(FAILURE_SEEDS_FILE, "wb") as f:
            pickle.dump(merged, f)
        print(f"  실패 seed {len(failure_seeds)}개 저장 → {FAILURE_SEEDS_FILE} (누적 {len(merged)}개)")

    total_avg   = sum(scores) / len(scores)
    success_avg = sum(success_scores) / len(success_scores) if success_scores else 0.0
    print(f"\n===== 전체 {testcount}회 결과 =====")
    print(f"  성공 횟수  : {len(success_scores)} / {testcount}  ({len(success_scores)/testcount*100:.1f}%)")
    print(f"  전체 평균  : {total_avg:.4f}")
    print(f"  성공만 평균: {success_avg:.4f}")
    return total_avg, len(success_scores)


# ---------------------------------------------------------------------
# 9. 엔트리 포인트
# ---------------------------------------------------------------------
if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--gen",       type=int,  default=GENERATIONS, help="진화 세대 수")
    ap.add_argument("--workers",   type=int,  default=1,           help="병렬 평가 프로세스 수")
    ap.add_argument("--seed",      type=int,  default=12345,       help="RNG seed")
    ap.add_argument("--resume",    type=str,  default=None,        help="이전 pkl 이어서 학습")
    ap.add_argument("--selection",  type=str,  default="roulette",
                    choices=["tournament", "roulette", "rank"],
                    help="선택 기법: tournament / roulette / rank")
    ap.add_argument("--mut_start",  type=float, default=MUTATION_PROB_START,
                    help="돌연변이 확률 시작값 (0.05~0.40)")
    ap.add_argument("--mut_end",    type=float, default=MUTATION_PROB_END,
                    help="돌연변이 확률 끝값 (0.05~0.40)")
    ap.add_argument("--failure_seeds",  type=str, default=None,
                    help="커리큘럼 학습: 실패 seed 파일 경로 (예: failure_seeds.pkl)")
    ap.add_argument("--stagnation_boost", type=float, default=STAGNATION_BOOST,
                    help="정체기 돌연변이 추가 확률 (0으로 설정 시 비활성화, 기본: 0.15)")
    ap.add_argument("--train_seed", type=int, default=None,
                    help="훈련 시드 풀 고정값 (미입력 시 완전 랜덤)")
    ap.add_argument("--test",      action="store_true",
                    help="저장된 유전자로 채점 테스트 (실패 시 즉시 중단, 교수님 방식)")
    ap.add_argument("--test_all",  action="store_true",
                    help="실패해도 N회 전부 돌려서 전체 평균 산출")
    ap.add_argument("--testcount", type=int,  default=1000,        help="테스트 횟수")
    ap.add_argument("--render",    action="store_true",            help="테스트 시 화면 표시")
    ap.add_argument("--quiet",     action="store_true",            help="test_all 시 성공 출력 생략")
    ap.add_argument("--gene_file", type=str,  default=None,
                    help="테스트할 유전자 pkl 파일 (기본: best_gene_team6.pkl)")
    args = ap.parse_args()

    print(f"유전자 길이(GENE_LEN) = {GENE_LEN},  신경망 = {LAYER_SIZES}")

    pkl_path = args.gene_file if args.gene_file else BEST_PICKLE
    if args.test:
        with open(pkl_path, "rb") as f:
            best = pickle.load(f)
        print(f"[테스트] {pkl_path}  (학습당시 avgR={best.get('avg_reward', 0):.2f})")
        run_competition_test(best["gene"], testcount=args.testcount, render=args.render)
    elif args.test_all:
        with open(pkl_path, "rb") as f:
            best = pickle.load(f)
        print(f"[테스트] {pkl_path}  (학습당시 avgR={best.get('avg_reward', 0):.2f})")
        run_all_test(best["gene"], testcount=args.testcount, render=args.render, quiet=args.quiet)
    else:
        evolve(generations=args.gen, workers=args.workers,
               seed=args.seed, resume=args.resume, selection=args.selection,
               mut_start=args.mut_start, mut_end=args.mut_end,
               failure_seeds_file=args.failure_seeds,
               stagnation_boost=args.stagnation_boost,
               train_seed=args.train_seed)
