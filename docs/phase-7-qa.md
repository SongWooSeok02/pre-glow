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

- Logic의 에디터 표시 여부는 같은 루프에서 프로세스 CPU를 반복 측정했다. Processing Threads 미터에서는 조건별 순간 화면 비교까지 수행했다.
- Logic AU에서 Basic/Advanced를 청취로 비교하고 재생 중 Shimmer 라우팅 전환의 오디오 결과를 확인한다. 기존 프로젝트 호환 범위는 저장 시점의 플러그인 버전이 확인된 프로젝트로 추가 검증한다.
- VST3에서 Basic/Advanced와 Shimmer 전환을 청취로 확인한다. Basic/Advanced로 각각 저장한 REAPER 프로젝트의 48 kHz 렌더는 샘플 단위로 일치했다. REAPER에서 새 검증 프로젝트 및 스키마 7 프로젝트 복원, 실시간 Decay 자동화와 렌더, User 프리셋 UI 왕복 및 크로스오버 이동을 확인했다. 스키마 7 프로젝트에서 확인한 Wet 표시와 저장값 차이는 아래에 기록했다.
- 실제 Intel Mac과 Windows에서 호스트 검증을 진행한다.

## 2026-11-16 재검증

- Apple Silicon Release 빌드 출력이 최신 상태였다. CTest 2개와 설치된 AU의 `auval -v aufx PrGl Pglo`가 통과했다.
- 설치된 AU와 로컬 VST3를 Pedalboard에서 재검증했다. 상태 복원 전후 최대 샘플 차이는 두 포맷 모두 0이었다. Freeze 잔향이 유지됐고, 샘플레이트 4개와 버퍼 크기 3개의 조합에서 Shimmer 경로 4개의 출력이 포맷 간 허용오차 2e-6 이내로 일치했다.
- 기존 로컬 0.1.3 베타 ZIP 세 개를 검사했다. AU, VST3, Standalone 모두 압축 무결성, 압축 해제 후 번들 서명 검증, arm64 아키텍처 확인을 통과했다. 공개 릴리스, Intel macOS 및 Windows 패키지는 남아 있다. 배포 전 적용할 JUCE 및 VST3 SDK 라이선스 조건도 확정해야 한다.
- 최신 소스 기준 [GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/43548722406)은 계정 결제 또는 지출 한도 문제로 Apple Silicon, Intel macOS, Windows 작업이 모두 시작되지 않았다.

## 2026-11-17 Logic Pro 프로젝트 복원

48 kHz의 새 검증 프로젝트에서 모노 오디오 트랙의 Audio Units → pre-glow → PRE-GLOW → Mono→Stereo 메뉴를 선택했다. AU 편집기가 열렸고 Freeze를 켠 뒤 Advanced로 전환해도 켜진 상태가 유지됐다. Shimmer 편집기에서 Feedback 경로 표시와 보이스 컨트롤을 확인했다. 프로젝트를 저장하고 닫은 뒤 다시 열었을 때 PRE-GLOW가 로드됐고 Freeze가 켜진 상태로 복원됐다. 검증 프로젝트 이름은 `PRE-GLOW Phase 7 QA 2026-11-17`이다.

이번 기록은 새 프로젝트의 AU 복원과 메뉴 동작을 확인한 결과다. 이전 버전 프로젝트 호환, 자동화 재생, User 프리셋 왕복, 오디오 청취 및 성능 미터 비교는 추가 검증이 필요하다.

### 이전 플러그인 상태 형식 검증

Pedalboard에서 설치된 Apple Silicon AU와 로컬 VST3 각각에 스키마 1·2·4·7의 테스트 상태를 호스트 상태 컨테이너로 전달했다. 두 포맷 모두 스키마 8로 다시 저장됐고 다음 값을 확인했다.

| 이전 형식 | 복원 결과 |
| --- | --- |
| 1 | Mix 80%가 Dry 20%, Wet 80%로 이전됐다. 기존 Decay와 Damping을 유지했고 Freeze는 꺼졌다. |
| 2 | Bloom Amount 40을 유지하면서 Bloom이 켜졌다. |
| 4 | 레벨 0인 Shimmer 보이스 1은 꺼지고 레벨 0.6인 보이스 2는 켜졌다. |
| 7 | Dry 15%, Wet 85%가 유지됐고 Y축은 85% Wet에 해당하는 0.7로 복원됐다. |

테스트용으로 구성한 플러그인 상태를 사용했다. 이전 버전에서 저장한 실제 DAW 프로젝트의 재개봉 검증은 남아 있다.

## 2026-11-24 Logic AU 프로젝트 및 User 프리셋

- 11월 14일에 저장된 `pre-glow Diffusion Test` 프로젝트의 임시 사본을 Logic Pro에서 열었다. 기존 `ac30 crunch` 트랙의 PRE-GLOW AU에서 Decay 10.00 s, Dry 30%, Wet 70%, Shimmer On을 확인했다. Advanced에서 Shimmer Feedback 경로와 보이스 1이 표시됐다. 프로젝트를 저장할 당시 플러그인의 정확한 버전은 확인하지 못했다.
- Advanced에서 크로스오버 250/4000 Hz를 350/5000 Hz로 변경해 User 프리셋으로 저장했다. 450/6000 Hz로 변경한 뒤 저장한 프리셋을 로드하자 350/5000 Hz로 복원됐다.
- Logic에서 저장한 프리셋의 페이로드를 Pedalboard 0.9.17의 VST3 호스트 상태에 넣었다. VST3가 다시 저장한 상태의 파라미터 85개가 일치했다. 48 kHz, 256 프레임에서 저장 전과 새 VST3 인스턴스에 복원한 뒤의 임펄스 렌더 최대 샘플 차이는 0이었다. VST3 DAW의 프리셋 UI는 추가 검증이 필요하다.
- Apple Silicon의 기존 Release DSP 테스트 바이너리 두 개와 설치된 AU의 `auval -v aufx PrGl Pglo`가 통과했다. [직전 GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/44035639280)의 세 플랫폼 작업은 계정 결제 또는 지출 한도 문제로 시작되지 않았다.

### Logic 재생 자동화 및 Rosetta 재검증

- Logic Pro에서 `pre-glow Diffusion Test` 프로젝트 사본의 `Audio 1` 트랙에 있는 `pre-glow-test` 클립을 루프 재생했다. PRE-GLOW Decay의 트랙 자동화 레인을 Read로 두었을 때 1마디에서 10.41 s, 2마디에서 9.35 s, 3마디에서 6.88 s를 확인했다.
- 재생 중 AU Controls에서 Shimmer Mix 35% 및 보이스 4개 활성 상태로 Feedback을 Parallel로 전환했다. Logic에서 Parallel이 표시됐고 재생이 계속됐다. 전환 시 오디오 결과의 청취는 남아 있다.
- Custom LCD에서 CPU/HD 미터를 표시했다. 미터를 더블클릭할 때 접근성 오류가 반복돼 상세 창의 조건별 비교는 남아 있다.
- Apple Silicon과 x86_64 Release에서 각각 AU, VST3, Standalone을 새로 빌드하고 CTest 2개를 통과했다. Apple Silicon의 Rosetta에서 x86_64 Pedalboard 0.9.17로 x86_64 VST3를 로드했다. 크로스오버 350/5000 Hz, Decay 5 s, Shimmer Mix 35%, Parallel 및 네 보이스 활성 상태를 새 인스턴스에 복원했다. 48 kHz 및 256 프레임에서 복원 전후 임펄스 렌더의 최대 샘플 차이는 0이었다.
- [GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/45334837463)에서는 Apple Silicon, Intel macOS, Windows 작업 모두 계정 결제 또는 지출 한도 문제로 시작되지 않았다.

## 2026-11-25 성능 비교 준비 및 오프라인 측정

Logic Pro에서 `pre-glow Diffusion Test` 사본의 프로젝트 샘플레이트 48 kHz, 오디오 장치 I/O 버퍼 128 프레임, Processing Threads Automatic을 확인했다. Key Command Assignments에 `Show/Hide Performance Meter`가 있었고 키는 지정되어 있지 않았다. Logic 미터의 조건별 측정은 남아 있다.

Apple Silicon에서 Pedalboard 0.9.17로 8초 스테레오 합성 신호를 48 kHz, 128 프레임, 플러그인 한 인스턴스에서 처리했다. 첫 실행을 제외하고 조건 순서를 번갈아 다섯 번 측정한 중앙값이다. Freeze는 처음 2초간 Shimmer 4보이스로 신호를 공급하고, 이후 6초간 Freeze를 켠 채 무음을 입력했다. 두 포맷의 Freeze 출력은 마지막 1초에도 잔향이 있었다.

| 조건 | AU | VST3 |
| --- | ---: | ---: |
| Hall | 119.00 ms | 119.38 ms |
| Hall + Shimmer 4보이스 | 332.07 ms | 334.61 ms |
| Hall + Shimmer 4보이스, Freeze 전환 | 584.20 ms | 590.39 ms |

호스트에서 VST3 Bypass를 설정한 처리 시간은 2.65 ms였다. AU의 Bypass 플래그는 이 측정에서 처리 시간의 감소가 관측되지 않아 Logic AU 바이패스 비교가 필요하다. 측정은 오프라인 호스트 처리 시간이며 에디터 열림/닫힘 조건은 Logic의 실시간 미터에서 확인할 예정이다.

[직전 GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/45340115050)의 세 플랫폼 작업은 계정 결제 또는 지출 한도 문제로 시작되지 않았다.

### Logic 실시간 성능 미터 비교

`pre-glow Diffusion Test` 프로젝트 사본에서 `Audio 1`의 `pre-glow-test`를 1마디부터 5마디까지 반복 재생했다. 48 kHz, I/O 버퍼 128 프레임, Processing Threads Automatic, MacBook Pro 스피커를 사용했다. 오디오 클립을 처리하는 PRE-GLOW AU는 한 개였고, 별도의 트랙에도 PRE-GLOW AU 한 개가 들어 있었다. Processing Threads 및 Drive I/O는 프로젝트 전체의 부하다.

| 조건 | 가장 높은 Processing Threads 막대 |
| --- | ---: |
| PRE-GLOW 바이패스 | 전체 막대 높이의 약 2% |
| Hall, Shimmer Off, Freeze Off, 표준 Controls 창 표시 | 약 2% |
| Hall, Shimmer 4보이스, Freeze Off, PRE-GLOW 편집기 표시 | 약 7% |
| Hall, Shimmer 4보이스, Freeze On, PRE-GLOW 편집기 표시 | 약 12% |
| Hall, Shimmer 4보이스, Freeze Off, 모든 플러그인 창 숨김 | 약 12% |

Shimmer Mix 35%, Feedback 20%를 적용했다. 표의 값은 조건당 한 화면에서 막대 영역의 픽셀 높이를 비교한 근사치다. 모든 화면에서 Processing Threads는 25% 눈금 아래였고 Drive I/O 막대는 보이지 않았다. 편집기 표시 여부에 따른 성능 차이는 반복 측정이 필요하다. 검증 뒤 재생을 멈추고 승인받아 지정했던 임시 키를 제거했다.

### REAPER VST3 및 Shimmer 전환 렌더

- Apple Silicon의 REAPER 7.86에서 PRE-GLOW VST3를 로드하고 새 QA 프로젝트를 저장해 다시 열었다. 크로스오버 209/5426 Hz, Parallel, Shimmer Mix 35% 및 네 보이스 활성 상태가 복원됐다. 크로스오버 값은 호스트가 표시하는 Hz로 비교했다.
- VST3 Advanced에서 크로스오버를 320/4800 Hz로 바꾸고 User 프리셋에 저장했다. 460/6200 Hz로 바꾼 뒤 프리셋을 로드하자 320/4800 Hz로 복원됐다.
- Decay 자동화 지점 두 개가 재개봉 후 유지됐다. 동일 프로젝트를 자동화 활성/비활성으로 각각 48 kHz, 스테레오, 4초 렌더했다. 출력 최대 샘플 차이는 0.005035, RMS 차이는 0.000409였다.
- Rosetta의 x86_64 REAPER 7.86에서 x86_64 VST3로 같은 자동화 프로젝트를 다시 렌더했다. Apple Silicon 렌더와의 최대 샘플 차이는 1.19e-7이었다. 실제 Intel Mac과 Windows에서의 호스트 실행은 남아 있다.
- 임시 검색 경로에서 PRE-GLOW를 찾지 못했다. 원본 Release VST3 번들의 서명 검사에서 리소스 오류도 확인했다. 검증용 복사본에 로컬 서명을 적용하고 macOS 표준 VST3 폴더에서 호스트 스캔을 통과한 뒤, 검증을 마치고 해당 복사본을 QA 폴더로 옮겼다.
- Pedalboard에서 Logic 검증 클립을 48 kHz, 128 프레임으로 처리하며 1초 지점에서 Feedback을 Parallel로 바꿨다. Hall, Shimmer 4보이스, Mix 35%, Feedback 20% 조건에서 전환 후 최대 샘플 크기는 0.128941이었다. Feedback 유지 대조군과의 최대 샘플 차이는 0.007848이었고, 전환한 AU와 VST3의 렌더 최대 샘플 차이는 0이었다. 두 렌더를 이어 붙인 A/B 파일을 로컬에 생성했다.

### REAPER 실시간 자동화, 이전 프로젝트 및 번들 서명

- Apple Silicon REAPER 7.86에서 `PRE-GLOW QA.rpp`를 재생했다. Decay의 정규화 값은 재생 시작에서 0.155, 약 1.01초에서 0.501, 약 1.76초에서 0.720으로 바뀌었다. 트랙 출력이 확인됐고 3초 이후에 재생을 멈췄다.
- 커밋 `cbafc45`에서 빌드한 스키마 7 VST3로 REAPER 프로젝트를 저장하고, 동일 VST3 클래스 ID의 최신 빌드로 다시 열어 별도 파일로 저장했다. 프로젝트에 저장된 VST3 상태에서 스키마 7 → 8 전환을 확인했다. Dry 15%와 Decay 1.75 s가 유지됐다. 원본 VST3 상태의 Wet은 35%, Morph Y는 0이었고, 마이그레이션 후 Morph Y는 35% Wet에 해당하는 −0.3이었다. 이 시험에서 REAPER의 Wet 파라미터 표시는 85%였으며 원본과 재저장 파일의 VST3 상태는 모두 35%였다. 표시와 저장값 차이에 대한 추가 검증이 필요하다.
- macOS에서 JUCE가 `moduleinfo.json`을 생성한 뒤 완성된 VST3 번들에 다시 로컬 서명하도록 빌드를 수정했다. arm64 및 x86_64 Release 번들에서 `moduleinfo.json`이 포함된 상태로 `codesign --verify --strict`가 통과했고, 각 빌드의 CTest 2개도 통과했다. arm64 번들은 수동 재서명 없이 REAPER에 로드했다.

### 이전 VST3 Wet 표시와 저장값 재검증

- 스키마 7 VST3에서 REAPER의 `TrackFX_SetParamNormalized`로 Wet 85%를 설정했다. 재생 전과 오디오 클립을 1.5초 이상 재생한 뒤 각각 프로젝트를 저장했다. 두 시점에서 REAPER는 85%를 읽었고, 두 파일의 VST3 상태에는 Wet 35% 및 Morph Y 0이 저장됐다. 이 절차에서 오디오 처리 여부는 차이를 바꾸지 않았다.
- 같은 이전 VST3의 자체 편집기는 Wet 35%를 표시했다. 편집기에서 Wet을 60%로 바꾸고 REAPER 프로젝트를 저장하자 VST3 상태에 Wet 60%가 기록됐다. 최신 VST3로 프로젝트를 다시 열어 저장한 파일은 스키마 8, Wet 60%, Morph Y 약 0.2였다. 이전 편집기에서 입력한 값의 저장 및 마이그레이션을 확인했다.
- 이 비교에서 표시와 저장값의 차이는 이전 빌드에 대한 호스트 스크립트 설정 경로에서 재현됐다. REAPER와 플러그인 사이에서 값 전달이 어긋나는 내부 지점은 확인하지 못했다.


### Logic 편집기 표시 여부 반복 측정

같은 `pre-glow Diffusion Test` 사본과 `pre-glow-test` 루프에서 Hall과 Shimmer를 켜고 Freeze를 끈 상태로, Audio 1 PRE-GLOW의 Advanced 편집기 닫힘과 표시를 번갈아 세 번씩 측정했다. macOS `top`에서 Logic Pro 프로세스 CPU를 1초 간격으로 읽고, 각 구간의 첫 누적 샘플을 제외했다. 조건별 유효 샘플은 12개다.

| 조건 | CPU 중앙값 | 표본 범위 |
| --- | ---: | ---: |
| 편집기 닫힘 | 17.2% | 13.0–24.6% |
| Advanced 편집기 표시 | 15.8% | 12.6–24.5% |

측정 범위가 겹쳤고 세 번의 구간 비교에서 차이 방향이 일관되지 않았다. 수치는 Logic 프로세스 전체의 CPU이며, Logic Performance Meter의 Processing Threads 수치와 단위가 다르다. 재생 중 Basic에서 Advanced로 바꿨을 때 Decay 3.00 s, Dry 65%, Wet 35%, Shimmer On이 유지됐다. 청취 평가는 남아 있다. 검증 후 재생을 멈췄다.

[GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/45530449963)에서는 세 플랫폼 작업 모두 계정 결제 또는 지출 한도 문제로 시작되지 않았다.


### 편집기 모드 및 라우팅 출력 비교

Apple Silicon REAPER 7.86에서 동일한 PRE-GLOW VST3 인스턴스의 Basic 상태와 Advanced로 전환한 상태를 각각 프로젝트 사본으로 저장했다. Decay 0.10 s, Dry 65%, Wet 35%, Shimmer On이 전환 전후 일치했다. 48 kHz, 스테레오, 4초 오프라인 렌더의 최대 절댓값은 0.122768이었고 두 렌더의 최대 샘플 차이는 0이었다.

이전에 만든 48 kHz Shimmer 비교 WAV에서 Feedback 유지 구간과 Feedback → Parallel 전환 구간을 대조했다. 1초 전환 지점의 첫 샘플 차이는 0이었다. 전환 후 처음 128프레임에서 두 신호 차분의 인접 샘플 간 최대 변화는 약 3.05e-5였다. 전환 이후 전체 구간에서 최대 샘플 차이는 약 0.00784였다. 직접 청취 평가는 남아 있다.

이 Mac은 Apple M1 Pro이며 연결된 Intel Mac 또는 Windows 실행 환경이 없었다. [GitHub Actions 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/45531078948)의 세 작업은 계정 결제 또는 지출 한도 문제로 시작되지 않았다.

### 전체 Shimmer 전환 조합

Pedalboard 0.9.17에서 Hall, Shimmer 4보이스, 보이스 레벨 각각 0.5, Mix 35%, Feedback 20%로 1초 스테레오 신호를 처리했다. 44.1/48/88.2/96 kHz와 64/256/1024 프레임 각각에서 버퍼 경계에 Feedback, Post-Reverb, Pre-Reverb, Parallel 중 다른 경로로 바꿨다. 144개 전환 조건 모두에서 AU와 VST3의 샘플 차이는 0이었고 최대 출력 절댓값은 0.050756이었다. 각 전환은 기존 경로를 유지한 대조군과 다른 출력을 냈다. 조건에 따라 최대 샘플 차이는 약 7.41e-6부터 0.01124까지였다.

Basic/Advanced 렌더와 Feedback 유지 및 Feedback → Parallel 전환 렌더를 하나의 로컬 청취 파일로 정리했다. 또한 Intel Mac에서 테스트할 수 있도록 x86_64 VST3를 로컬 ZIP으로 묶고, 압축 해제 후 아키텍처와 서명을 검증했다. 실제 Intel Mac 호스트 실행과 Windows 검증은 남아 있다. [최신 확인 CI 실행](https://github.com/SongWooSeok02/pre-glow/actions/runs/45531574898)은 계정 결제 또는 지출 한도 문제로 세 작업이 시작되지 않았다.
