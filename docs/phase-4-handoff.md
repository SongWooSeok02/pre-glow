# 페이즈 4 인수인계

페이즈 4에는 ±24반음 피치 시프터, 4보이스 Shimmer, Feedback Shimmer, Feedback/Post-Reverb 선택기와 보이스 카드 UI를 포함한다. 마지막 변경은 카드의 숫자 입력칸 너비를 각 열에 맞추고 카드 하단 여백을 확보한 것이다.

- 새 프로젝트는 Voice 1만 켜지고 Shimmer Mix와 Feedback은 0이다. 상태 스키마 5는 보이스 활성 상태를 저장하며, 스키마 4에서 Level이 양수였던 보이스는 켜서 복원한다. 기존 프로젝트의 기본 라우팅은 Feedback이다.
- Freeze 중에는 Shimmer 피드백이 차단되고 피치 레이어는 유지된다. Stereo Spread와 Pitch Modulation은 미구현 목표로 남긴다.
- Logic에서 카드 조작, 경로 전환 청취, 저장·복원 확인이 남아 있다.

## 페이즈 5 시작점

Flexible Routing의 다음 구현은 Pre-Reverb Shimmer다. 피치 신호를 리버브 입력에 합치는 경로를 추가하고, 기존 Feedback/Post-Reverb 선택 순서와 저장값을 보존한다. 경로 전환은 스무딩하고, Freeze에서 새 입력이 고정된 테일에 섞이지 않는지 확인한다. 이후 Parallel, A/B·필터 라우팅, Multiband, XY Morph, Modulation Matrix, Ducking, Reverse-like는 기능계획서 범위를 따른다.
