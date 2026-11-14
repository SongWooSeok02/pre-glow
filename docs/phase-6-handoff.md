# 페이즈 6 마감 및 페이즈 7 인수인계

페이즈 6의 구현 기준 커밋은 `18366f3`이다. 페이즈 7의 DSP 회귀 테스트와 단일 엔진 성능 기준 측정도 시작했다.

## 구현 범위

- Basic이 기본 화면이다. 제목 PRE-GLOW 아래에서 Advanced로 전환하면 세부 컨트롤과 기존에 펼쳐 둔 섹션에 접근할 수 있다. 두 모드는 같은 파라미터 연결을 사용한다.
- Presets에서 Factory 4종(Hall, Room, Plate, Cloud)과 User 프리셋을 검색하고 불러올 수 있다. User 프리셋은 저장, 이름 변경, 휴지통 이동을 지원한다. macOS의 AU, VST3, Standalone이 같은 저장 폴더와 버전 관리형 `.preglowpreset` 상태 포맷을 사용한다.
- 스펙트럼 아래에 Input, Output, Tail 피크와 Stereo 상관도를 표시한다. Tail은 최종 믹스에 들어가는 Wet 신호를 측정한다. Stereo 막대에는 350ms 표시 평활화를 적용했다.
- Advanced에서 Shimmer를 열면 선택된 신호 경로를 표시한다. Logic Pro에서 발견한 화살표와 구분점의 문자 깨짐은 UTF-8 디코딩을 적용해 수정했다. Multiband를 열면 스펙트럼에 Low·Mid·High 구간과 크로스오버 경계가 표시된다.

## 검증 상태

- Apple Silicon Release의 Standalone, AU, VST3 빌드와 `reverb_impulse`, `pitch_shifter` 테스트가 통과했다. Logic Pro용 PRE-GLOW AU를 설치했고 `auval -v aufx PrGl Pglo`가 통과했다.
- Logic Pro에서 수정 후 라우팅 표시와 크로스오버 이동, 미터 반응, 모드 전환 시 청취 및 프로젝트 복원을 확인한다. 프리셋의 AU/VST3 왕복과 타 플랫폼 검증도 남아 있다.

## 페이즈 7 시작점

1. **기준 확인.** README의 Release 빌드·CTest 명령을 실행한다. Apple M1 Pro에서 단일 기본 Hall 엔진의 10초 처리 시간 중앙값은 48 kHz에서 29.2 ms, 96 kHz에서 52.8 ms였다. 비교 시 `Reverb::prepare` 후 1초 워밍업을 하고, 왼쪽에 4096샘플마다 0.5 임펄스, 오른쪽에 무음을 넣어 `processSources`를 실행한다. Wet 출력 절댓값 합을 사용해 컴파일러가 계산을 생략하지 않도록 하고, C++17 `-O3 -DNDEBUG` 빌드에서 각 조건을 3회 측정한다.
2. **Logic Pro 성능 측정.** 같은 소스를 반복 재생하고 샘플레이트, I/O 버퍼 크기, 인스턴스 수를 기록한다. 같은 프로젝트에서 PRE-GLOW 바이패스와 활성 상태를 비교한 뒤 기본 설정, Shimmer 4보이스, Freeze, 에디터 열림/닫힘을 한 조건씩 바꿔 Logic 성능 미터를 비교한다. 위 단일 엔진 수치와 별도로 기록한다.
3. **병목 최적화 및 회귀 확인.** 측정으로 확인한 병목부터 수정한다. 같은 조건에서 다시 측정하고 DSP 테스트, Release 빌드, AU 검증을 반복한다.
4. **포맷과 DAW 검증.** Logic AU와 VST3 호스트에서 이전 세션 복원, 파라미터 자동화, 프리셋 왕복, UI와 오디오 동작을 확인한다. 호스트 버전과 결과를 기록한다.
