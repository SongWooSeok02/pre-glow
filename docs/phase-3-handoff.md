# 페이즈 3 인수인계

2026-11-14 (KST) 기준. [기능 계획표](pre-glow_기능계획서.md)의 페이즈 3 항목은 구현 완료. 최종 구현은 [18d6470](https://github.com/SongWooSeok02/pre-glow/commit/18d6470)이며, 플랫폼·DAW 검증 상태는 아래에 따로 기록한다.

| 계획 항목 | 구현 결과와 계획 대비 변경 |
| --- | --- |
| Long Decay | 페이즈 1부터 있던 0.1–120초 Decay를 재사용했다. 페이즈 3에서 상한이나 기존 자동화 범위를 바꾸지 않았다. |
| Freeze | 기본값이 꺼진 Classic Freeze 토글에 Smooth Freeze 동작을 통합했다. 약 100ms에 걸쳐 새 입력을 웻 경로에서 차단하고 피드백을 유지하며, 드라이 신호는 계속 통과한다. 최종 기능 명세의 Momentary, Evolving, 별도 전환 시간·입력량·릴리즈·톤·변조·출력 조절은 미구현이다. Shimmer와의 피드백 검증은 Shimmer 구현 이후에 진행한다. |
| Infinite Decay | 웻 입력을 계속 받는 별도 토글. 댐핑 적용 전 감쇠 기준을 600초로 설정하고, 작은 피드백 손실로 누적을 제한한다. 켜 둔 상태에서 Freeze가 우선하고, Freeze 해제 뒤 다시 적용된다. 손실 없는 유지가 필요한 경우 Freeze를 사용한다. |
| Bloom | 초기 반사를 줄이고 입력에 따라 후기 잔향이 서서히 올라온다. Amount, Attack, Release, Shape를 제공한다. 처음에는 Amount 0%로 끄는 방식이었으나 독립 토글을 추가했다. 새 인스턴스는 토글이 꺼져 있고 Amount는 75%로 준비된다. |
| Swell | 입력이 Sensitivity 기준을 넘으면 웻 신호가 Time과 Curve에 따라 페이드인한다. 독립 토글 기본값은 꺼짐이다. |

Bloom과 Swell은 켜면 해당 설정 행이 펼쳐지고 끄면 설정값을 보존한 채 접힌다. 둘 다 켜면 웻 경로별로 두 볼륨 곡선 중 낮은 값을 적용해 감쇠가 곱해지는 현상을 피한다. 네 토글을 모두 켜면 Freeze가 우선하며, 해제하면 선택한 감쇠 모드와 입력 반응이 다시 적용된다. 토글과 노브 값은 유지된다.

상태 스키마는 2에서 3으로 올렸다. 스키마 1의 Mix 마이그레이션을 유지하고, 스키마 1·2에서 Bloom Amount가 양수로 저장된 경우 Bloom을 켜서 복원한다. Swell은 기본값이 꺼짐이다. 기존 플러그인 및 파라미터 ID는 유지했다.

## 검증 상태

- Apple Silicon Release AU·VST3·Standalone 빌드, 44.1·48·88.2·96kHz DSP 테스트, Apple auval 통과. 인수인계 시점에 빌드 상태, CTest와 설치된 AU의 auval을 다시 확인했다.
- DSP에서 16개 스위치 조합의 출력 범위를 검사했고, Freeze 조합에서는 다른 스위치와 새 입력이 유지 중인 잔향에 영향을 주지 않는지 비교했다. AU·VST3 호스트에서 48kHz로 각 효과와 네 토글을 모두 켠 상태 및 Freeze 해제를 확인했다. Standalone에서 설정 행의 펼치기와 접기를 확인했다.
- 사용자가 Freeze 동작과 Bloom·Swell UI를 확인했다. 네 토글 조합에서 동작이 꼬이는 느낌을 보고한 뒤 우선순위와 볼륨 곡선 결합을 수정했다. 그 수정 이후 Logic에서 직접 듣는 확인, 창 크기 변경, 프로젝트 저장·재로드, 자동화 재생은 남아 있다. Hall·Room의 짧은 딜레이처럼 들리는 잔향도 별도 청취 항목으로 남겨 둔다.
- [Freeze CI](https://github.com/SongWooSeok02/pre-glow/actions/runs/43515439409)와 [Infinite Decay CI](https://github.com/SongWooSeok02/pre-glow/actions/runs/43515860008)는 통과했다. [최종 구현 CI](https://github.com/SongWooSeok02/pre-glow/actions/runs/43519477829)는 GitHub 계정 결제 또는 지출 한도 문제로 모든 빌드 작업이 시작되기 전에 실패했다. 최종 구현의 Intel macOS·Windows 검증은 보류 상태다.

페이즈 4의 시작점은 피치 시프터와 멀티보이스 Shimmer다. Freeze와 Shimmer를 함께 연결할 때는 기존 스위치 우선순위를 유지하고 피드백 안정성을 검증한다.
