# 페이즈 7 검증

## Apple Silicon Release

Standalone, AU, VST3를 빌드했다. CTest의 `reverb_impulse`, `pitch_shifter`가 통과했다. 설치한 새 AU가 `auval -v aufx PrGl Pglo`를 통과했다.

Pedalboard 0.9.17 호스트에서 두 포맷을 각각 로드했다. 48/96 kHz, 256 프레임 버퍼로 1초 길이의 스테레오 임펄스 열을 처리했다. 두 포맷 모두 출력 크기가 입력과 일치했고 유한한 출력과 잔향을 확인했다.

## AU/VST3 호스트 상태 및 오디오 비교

Pedalboard 0.9.17에서 각 포맷의 호스트 상태를 저장해 같은 포맷의 새 인스턴스에 복원했다. Dry/Wet, Shimmer Mix/Feedback/Routing, Shimmer On, 네 보이스의 활성 상태와 레벨이 일치했다. 48 kHz와 256 프레임에서 복원 전후 렌더의 최대 샘플 차이는 0이었다. 같은 조건에서 입력을 무음으로 바꾼 뒤 Freeze를 켜면 두 포맷 모두 잔향이 유지됐다.

네 보이스를 켜고 44.1/48/88.2/96 kHz와 64/256/1024 프레임의 모든 조합에서 Shimmer 라우팅 네 가지를 렌더했다. 각 조건에서 네 라우팅이 서로 다른 출력을 냈고, 같은 조건의 AU/VST3 출력은 허용오차 2e-6에서 일치했다. 모든 출력 샘플이 유한했고 절댓값이 1 미만이었다.

## Hall 엔진 성능

[페이즈 6 인수인계의 측정 조건](phase-6-handoff.md#페이즈-7-시작점)으로 C++17 `-O3 -DNDEBUG` 바이너리를 각각 만들었다. 변경 전후를 번갈아 두 세트 실행했다. 아래는 마지막 세트에서 3회 측정한 중앙값이다.

| 샘플레이트 | 변경 전 | 변경 후 |
| --- | ---: | ---: |
| 48 kHz | 26.08 ms | 25.56 ms |
| 96 kHz | 52.07 ms | 49.93 ms |

샘플레이트별 Wet 출력 절댓값 합은 변경 전후 일치했다. 첫 세트에서 48 kHz는 변경 후 31.95 ms, 변경 전 26.99 ms로 측정 편차가 있었다. 수치는 단일 DSP 엔진의 오프라인 처리 시간이다.

Shimmer 피드백의 적용 계수가 0인 샘플에서는 루프 게인 최대값 계산을 건너뛴다. 피드백이 활성화된 샘플에서는 기존 제한 계산을 적용한다.

## Logic Pro 13.0 편집기

열려 있던 AU 인스턴스에서 Basic → Advanced → Basic으로 전환했다. A/B와 Decay, Dry, Wet, Freeze 및 Shimmer On 표시 값이 전환 전후 일치했다. Shimmer 섹션에서 비활성 상태의 경로 `Input → Reverb → Wet`이 깨짐 없이 표시됐다. Multiband 섹션에서 250 Hz와 4000 Hz의 경계 및 Low·Mid·High 영역을 확인했다.

사용자가 Logic Pro에서 직접 입력을 넣었을 때 Input, Output, Tail, Stereo 미터와 스펙트럼이 반응했다. 입력을 멈추고 Input이 −60 dBFS 미만으로 내려간 뒤에도 Output과 Tail이 −39.2 dBFS에 남았고, 이후 둘 다 −56.7 dBFS로 줄었다.

## 남은 검증

- Logic Pro 13.0에서 고정 소스, 샘플레이트, I/O 버퍼와 인스턴스 수를 기록하고 바이패스, 기본 Hall, Shimmer 4보이스, Freeze, 에디터 열림/닫힘의 성능 미터를 비교한다.
- Logic AU와 VST3 DAW에서 기존 프로젝트 복원, 파라미터 자동화, AU/VST3 User 프리셋 왕복, Basic/Advanced 청취, Shimmer 활성 라우팅, 크로스오버 이동를 확인한다.
- 타 플랫폼에서 실제 호스트를 사용해 확인한다.
