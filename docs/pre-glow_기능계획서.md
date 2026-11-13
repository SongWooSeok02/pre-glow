# pre-glow 기능 기획서 v1.0

> **Professional Ambient Reverb & Spatial Sound Design Plugin**  
> 문서 상태: 제품 기능 기획 / 개발 목표 (구현 완료를 의미하지 않음)

## 1. 프로젝트 개요

**pre-glow**는 Shimmer, Freeze, Infinite Reverb를 중심으로 사용자가 잔향의 구조, 공간감, 음색, 피치, 모듈레이션을 자유롭게 설계할 수 있는 크로스 플랫폼 앰비언트 리버브 플러그인이다. 단순한 프리셋 선택을 넘어 사용자가 자신만의 공간계 사운드를 만드는 것이 핵심 목표다.

| 항목 | 내용 |
|---|---|
| 프로그램명 | pre-glow |
| 유형 | 오디오 이펙트 플러그인 |
| 목적 | 앰비언트 리버브 및 공간계 사운드 디자인 |
| 언어 | C++17 이상 |
| 프레임워크 | JUCE 8 |
| 개발 도구 | Codex, CMake, Xcode, Visual Studio |
| 운영체제 | macOS / Windows |
| 우선 플러그인 포맷 | AU (macOS), VST3 (macOS/Windows) |
| 향후 검토 | AAX / CLAP |
| 주요 사용자 | 기타리스트, 프로듀서, 믹싱 엔지니어, 사운드 디자이너 |

## 2. 핵심 기능

### 2.1 Advanced Reverb Engine
- FDN(Feedback Delay Network) 기반 고품질 리버브 엔진
- 공간 크기, 잔향 밀도, 확산도, 초기 반사음, 주파수별 감쇠 제어
- 파라미터: Decay, Size, Diffusion, Density, Pre-delay, Damping, Early/Late Balance, Attack, Shape
- 긴 잔향과 극단적인 설정에서도 안정적인 피드백 처리

### 2.2 Multi-Voice Shimmer
- 최대 4개 피치 보이스 독립 제어
- 옥타브, 완전5도, 디튠, 하강 피치 등 화성적 잔향 생성
- 보이스별 Pitch, Fine Tune, Level, Pan, Delay, Feedback Send, Tone
- 전체 Shimmer Mix, Feedback, Stereo Spread, Pitch Modulation
- 피치 시프팅 품질, 레이턴시, CPU 부하를 고려한 알고리즘 선택

### 2.3 Freeze & Infinite
- Classic Freeze: 현재 잔향 유지
- Smooth Freeze: 부드러운 전환
- Momentary Freeze: 누르는 동안 유지하는 제어 방식
- Evolving Freeze: 유지된 잔향에 변조·피치 변화 적용
- Infinite Decay: 매우 긴 지속감을 안정적으로 제공
- 전환 시간, 입력량, 릴리즈, 톤, 변조, 출력 레벨 조절
- Shimmer 동시 사용 시 피드백 폭주 방지

### 2.4 Flexible Routing
- Pre-Reverb Shimmer: 피치 처리 후 리버브
- Post-Reverb Shimmer: 리버브 후 피치 처리
- Feedback Shimmer: 제어된 리버브 피드백 경로에 피치 처리 삽입
- Parallel Shimmer: 리버브와 Shimmer 병렬 처리
- Dual Reverb: 두 리버브 경로 병렬 처리(성능 검증 후)
- Shimmer Position, Feedback Send, Parallel Blend, Reverb A/B Blend, Filter Placement 제어
- 유효한 라우팅만 허용하고 전환 시 클릭 및 불안정한 피드백 방지

### 2.5 XY Morph Pad
- X/Y축에 여러 파라미터를 동시에 할당
- 목적지별 최소·최대값, 반전, 깊이, 곡선 설정
- 호스트 오토메이션 및 지원되는 MIDI 매핑
- 선택적으로 XY 움직임 기록 및 템포 동기 재생
- 호스트 오토메이션과 모듈레이션 매트릭스 간 우선순위 정의

### 2.6 Multiband Reverb Control
- Low / Mid / High 3밴드 제어
- 크로스오버 주파수 조절
- 밴드별 Decay Multiplier, Feedback Damping, Level, Shimmer Send, Modulation Depth
- 우선 공유 FDN 기반 주파수별 감쇠 구현
- 독립 멀티밴드 리버브 경로는 음질·CPU 검증 후 확장

## 3. 리버브 알고리즘

| 알고리즘 | 목표 사운드 |
|---|---|
| Room | 짧고 자연스러운 공간 반사음 |
| Hall | 넓고 부드러운 홀 잔향 |
| Plate | 밀도 높고 선명한 잔향 |
| Cloud | 부드럽게 퍼지는 앰비언트 질감 |
| Cathedral | 웅장하고 긴 대형 공간 잔향 |
| Void | 비현실적이고 실험적인 공간감 |

각 알고리즘은 단순한 프리셋 차이가 아니라 의미 있는 DSP·음향 특성의 차이를 갖도록 설계한다.

## 4. 세부 사운드 디자인 기능

| 모듈 | 기능 및 설정 |
|---|---|
| Modulation Engine | Sine/Triangle LFO, Smooth Random, Sample & Hold, Slow Drift, Envelope Follower |
| Modulation Matrix | 최소 8개 할당 목표, Source/Destination/Depth/Polarity/Curve/Smoothing |
| Tone Shaping | HPF, LPF, Low/High Shelf, Parametric Mid EQ, Feedback 및 Shimmer 필터 |
| Stereo Engine | Width, Balance, Early/Late Width, Mid/Side, Decorrelation, Mono Compatibility |
| Bloom | 입력 후 잔향이 서서히 확산되는 효과; Amount, Attack, Release, Shape |
| Swell | Wet 신호의 부드러운 등장; Time, Sensitivity, Curve |
| Ducking | 연주 중 Wet 신호 자동 감소; Threshold, Amount, Attack, Release |
| Reverse-like Reverb | 역재생처럼 부풀어 오르는 잔향; 필요한 버퍼링·레이턴시 반영 |
| Tempo Sync | 호스트 BPM 기반 LFO·모션 동기화 |

## 5. 사용자 인터페이스

### 5.1 Basic Mode
- Algorithm, Decay, Size, Diffusion, Shimmer, Freeze, Modulation, Tone, Mix를 한 화면에서 조절
- 빠른 작업을 위한 직관적인 인터페이스

### 5.2 Advanced Mode
1. SPACE
2. DECAY
3. SHIMMER
4. FREEZE
5. ROUTING
6. MULTIBAND
7. MODULATION
8. XY MORPH
9. TONE
10. STEREO
11. DYNAMICS

### 5.3 시각 디자인 및 계측
- 다크 네이비·차콜 배경, 보라색·푸른색 포인트, 은은한 발광 효과
- 크기 조절 가능한 공통 GUI
- 입력/출력 레벨, 리버브 테일, Decay Envelope, Stereo Correlation, Shimmer/Modulation Activity
- Routing Diagram, Multiband Response Display
- 시각화 작업은 오디오 스레드를 차단하지 않도록 구현

## 6. 프리셋 및 사용 편의성

- Factory/User Preset 저장·불러오기·검색·분류·이름 변경·삭제
- 카테고리: Ambient Guitar, Shoegaze, Dream Pop, Post-Rock, Cinematic, Shimmer, Freeze, Vocal, Experimental
- A/B Comparison, Undo/Redo, Default Preset
- DAW Automation 및 전체 프로젝트 상태 저장·복원
- 지원되는 호스트 환경에서 MIDI 제어
- AU/VST3 간 공유 가능한 버전 관리형 프리셋 포맷
- Eco / Normal / High Quality 모드(성능 검증 후)

## 7. 지원 운영체제 및 DAW 목표

| DAW | macOS | Windows |
|---|---|---|
| Logic Pro | AU | 해당 없음 |
| Ableton Live | AU / VST3 | VST3 |
| REAPER | AU / VST3 | VST3 |
| FL Studio | VST3 | VST3 |
| Cubase | VST3 | VST3 |
| Studio One | VST3 | VST3 |
| Pro Tools | AAX 추후 검토 | AAX 추후 검토 |

위 표는 **지원 목표**이며, 실제 지원 여부는 DAW별 빌드·로딩·오토메이션·상태 복원 검증 이후 확정한다.

## 8. 기술 아키텍처 및 품질 원칙

- 공통 C++ DSP 엔진과 JUCE 플러그인 래퍼·GUI 분리
- macOS AU/VST3, Windows VST3, 양 플랫폼 Standalone 빌드
- 안정적인 플러그인/파라미터 ID 및 상태 직렬화
- 오디오 콜백 내 동적 할당, 블로킹 락, 파일 I/O 금지
- 클릭 없는 파라미터 변경, denormal 및 NaN/Inf 방지
- 레이턴시 정확한 보고, 바이패스·잔향 테일 처리
- 44.1/48/88.2/96kHz 및 다양한 버퍼 크기 검증
- CPU·메모리 프로파일링, 극단적인 설정과 장시간 Freeze 테스트
- JUCE 및 외부 DSP 라이브러리의 배포 라이선스 검토

## 9. 개발 로드맵

| 단계 | 개발 내용 |
|---|---|
| Phase 1 | JUCE/CMake 기반 크로스 플랫폼 프로젝트, 기본 AU/VST3, 파라미터·GUI |
| Phase 2 | FDN, Diffusion, Early Reflections, Decay, Damping, Stereo 리버브 엔진 |
| Phase 3 | Long Decay, Freeze, Infinite, Bloom, Swell |
| Phase 4 | 피치 시프터, 최대 4보이스 Shimmer, Feedback Shimmer |
| Phase 5 | Flexible Routing, Multiband, XY Morph, Modulation Matrix, Ducking, Reverse-like |
| Phase 6 | Basic/Advanced GUI, 프리셋 브라우저, 시각화 |
| Phase 7 | DSP 테스트, 성능 최적화, AU/VST3 검증, DAW별 QA |
| Phase 8 | macOS/Windows 설치·배포 패키징, 문서화 |

## 10. 제품 핵심 가치

1. **Sound Quality:** 자연스럽고 풍부한 잔향과 안정적인 실시간 처리
2. **Freedom:** 세밀한 파라미터·모듈레이션·내부 라우팅 제어
3. **Creativity:** Shimmer, Freeze, Morph 기반의 창의적 사운드 디자인
4. **Compatibility:** 다양한 DAW 및 운영체제에서 일관된 사용 경험

## 11. 개발 원칙 및 현재 상태

이 문서는 최종 제품의 목표 기능과 단계별 진행 상황을 기록한다. 아래 구현 상태와 플랫폼별 검증 범위는 2026-11-14(KST) 기준이다.

| 단계 | 구현 상태 | 검증 범위 |
|---|---|---|
| Phase 1–2 | JUCE/CMake 프로젝트, 기본 플러그인 UI와 파라미터, FDN 리버브, 입력 확산 및 초기 반사 구현 | [Phase 2 인수인계](phase-2-handoff.md) |
| Phase 3 | 기존 Long Decay에 Freeze, Infinite Decay, Bloom, Swell을 더해 단계 범위 구현 완료 | Apple Silicon Release AU·VST3·Standalone 빌드, 4개 샘플레이트 DSP 테스트, AU 검증 통과. Logic 청취 및 프로젝트 저장·복원 검증은 남아 있다. 세부 구현 범위는 [Phase 3 인수인계](phase-3-handoff.md) 참조. |
| Phase 4 | ±24반음 피치 시프터와 독립적으로 조절할 수 있는 4보이스 Shimmer DSP 구현. 리버브 피드백 경로와 플러그인 제어 연결 예정 | Apple Silicon에서 44.1·48·88.2·96kHz의 목표 음정, 출력 범위, 리셋, 보이스 합산, 팬, 딜레이, 피드백 센드 분리 테스트와 기존 리버브 테스트 통과. |

Phase 4의 다음 작업은 Shimmer DSP를 플러그인에 연결하고 리버브 피드백 경로를 추가하는 것이다. Freeze와 함께 사용할 때는 기존 Freeze 우선순위를 유지하고, 피드백이 누적되는 동안과 Freeze를 해제할 때 출력이 안정적인지 검증한다.

> **제품 방향:** 간편하게 사용할 수도 있지만, 원한다면 잔향의 내부 구조까지 직접 설계할 수 있는 앰비언트 사운드 디자인 플러그인.
