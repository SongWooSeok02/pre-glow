# 페이즈 5 마감 및 페이즈 6 인수인계

2026-11-14 기준으로 페이즈 5의 합의된 구현 범위를 마감했다. 기준 코드 커밋은 `ba33b19`이다.

## 페이즈 5에 들어간 기능

- **라우팅** — Feedback, Post-Reverb, Pre-Reverb, Parallel을 선택할 수 있다. Shimmer On으로 저장된 보이스와 믹스 설정을 유지한 채 효과를 켜고 끈다. Reverb A/B는 두 캐릭터의 출력 비율을 혼합한다. Low Cut은 Reverb Input 또는 Wet Output에 배치한다.
- **Multiband** — 공유 FDN 기반 Low·Mid·High 제어. 조절 가능한 두 경계의 기본값은 250 Hz와 4 kHz다. 각 대역에 Decay, Feedback Damping, Wet Level, Shimmer Send, Mod Depth가 있다. Mod Depth는 Wet 레벨을 느리게 변조한다.
- **XY Morph** — X는 A 100% → 50:50 → B 100%, Y는 Wet 0% → 50% → 100%다. 왼쪽 A/B와 Wet 컨트롤이 패드와 같은 파라미터를 사용한다. 패드는 토글로 표시하며, Multiband나 Shimmer가 열리면 스펙트럼 아래에 배치한다.
- **계측과 Freeze** — Dry/Reverb 스펙트럼은 믹스에 들어가는 두 신호를 보여준다. Freeze 중 새 입력은 별도 경로에서 같은 리버브 설정을 적용받고, 고정된 잔향과 출력에서 합쳐진다.
- **Dynamics** — Ducking에 On/Off, Threshold, Amount, Attack, Release를 제공한다. Reverse-like는 새 어택에서 Wet 상승을 다시 시작하는 엔벌로프 효과다. Rise를 조절할 수 있고 추가 레이턴시는 없다. 어택 중복 감지 방지 간격은 25 ms다.

Mod Matrix는 사용자 결정으로 제거했다. 현재 XY는 A/B와 Wet에 고정되어 있으며, 확장 매핑과 독립 멀티밴드 엔진은 이번 마감 범위에서 제외했다.

## 호환성과 검증

- 상태 스키마는 8이다. 스키마 7은 기존 저장 A/B 비율을 Morph X로 옮겼고, 스키마 8은 기존 저장 Wet 값을 Morph Y로 옮긴다. 저장된 값은 마이그레이션되지만 기존 A/B Blend·Wet 오토메이션 레인은 호스트에서 Morph X (A/B)·Morph Y (Wet)으로 다시 매핑해야 한다.
- macOS Standalone·AU·VST3 빌드와 기존 DSP 테스트 `reverb_impulse`, `pitch_shifter`가 통과했다. Logic Pro용 AU를 설치했고 사용자는 Reverse-like 연타 수정 후 잘 동작한다고 확인했다.
- 전체 DAW 조합, 이전 Logic 프로젝트 복원, AU/VST3 간 프리셋 공유, 성능은 별도 검증이 필요하다. 최신 DSP 커밋의 [GitHub Actions](https://github.com/SongWooSeok02/pre-glow/actions/runs/43539831409)는 실패로 표시됐다. 원인과 타 플랫폼 통과 여부는 확인하지 않았다.

## 페이즈 6 시작 순서

1. **Basic / Advanced 전환부터 구현한다.** 기존 `Source/PluginEditor.cpp`의 레이아웃을 출발점으로 삼는다. Basic은 캐릭터, Decay, Dry/Wet, Shimmer On, Freeze 같은 자주 쓰는 기존 컨트롤 중심으로 구성한다. Advanced는 현재의 세부 컨트롤과 접이식 섹션을 제공한다. 모드 전환은 표시와 레이아웃만 바꾸며 APVTS 값과 호스트 파라미터 ID를 유지한다. 두 모드에서 같은 세션을 열고 전환 전후 소리와 값이 같은지 확인한다.
2. **프리셋 브라우저를 추가한다.** `getStateInformation`과 `setStateInformation`의 상태 마이그레이션을 재사용한다. Factory 읽기 전용 및 User 저장·불러오기부터 구현하고, 검색·분류·이름 변경·삭제를 이어 붙인다. 포맷 버전, 잘못된 파일 처리, AU/VST3 간 재사용을 검증한다. 파일 접근은 오디오 콜백 밖에서 처리한다.
3. **시각화를 확장한다.** 기존 Dry/Reverb 스펙트럼과 UI 타이머를 기반으로 입력/출력 레벨부터 추가한다. 이후 테일, 스테레오 상관도, 라우팅, 멀티밴드 표시를 순서대로 검토한다. 오디오 스레드는 할당·잠금·파일 접근 없이 스냅샷만 전달하고 화면 계산과 그리기는 UI에서 처리한다.

첫 작업의 완료 기준은 **Basic/Advanced를 전환해도 소리와 파라미터 값이 유지되고, 양쪽에서 필수 조작에 접근할 수 있는 것**이다. 현재 그래프 높이 고정과 XY 표시 규칙도 함께 확인한다. 전체 DAW, 포맷, 성능 검증은 페이즈 7에서 이어간다.
