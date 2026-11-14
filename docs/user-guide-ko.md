# PRE-GLOW 0.1.3 베타 사용설명서

PRE-GLOW는 보컬, 기타, 신스 같은 소리에 공간감과 잔향을 더하는 리버브 플러그인입니다.

## 1. 내 환경에 맞는 파일 고르기

이번 베타 ZIP은 Apple Silicon(M 시리즈) Mac용입니다. Apple 메뉴 → **이 Mac에 관하여**에서 칩을 확인할 수 있습니다. 아래에서 사용할 형식 하나를 받아 압축을 풉니다.

| 사용 환경 | ZIP 이름 끝부분 | 설치 방법 |
| --- | --- | --- |
| Logic Pro | `-AU.zip` | `pre-glow.component`를 `~/Library/Audio/Plug-Ins/Components/`에 복사 |
| Ableton Live, Cubase 등 VST3 지원 DAW | `-VST3.zip` | `pre-glow.vst3`를 `~/Library/Audio/Plug-Ins/VST3/`에 복사 |
| DAW 없이 오디오 입력으로 확인 | `-Standalone.zip` | `pre-glow.app`을 응용 프로그램 폴더에 옮겨 실행 |

Finder에서 **⌘⇧G**를 누르면 설치 경로를 붙여넣을 수 있습니다. 폴더가 없으면 상위 폴더에서 만들어 주세요. 설치 후 DAW를 완전히 종료했다가 다시 열고 오디오 트랙의 이펙트 목록에서 **PRE-GLOW**를 찾으세요. VST3가 안 보이면 DAW의 플러그인 설정에서 VST3 스캔이 켜져 있는지 확인하고 다시 스캔하세요.

이 베타는 Apple 공증을 받지 않았습니다. macOS나 DAW가 로딩을 차단하면 표시된 메시지를 개발자에게 보내 주세요. Intel Mac과 Windows용 파일은 이번 패키지에 포함되어 있지 않습니다.

## 2. 첫 리버브 들어보기

1. DAW에서 보컬이나 악기 클립을 재생하고 같은 트랙에 **PRE-GLOW**를 이펙트로 추가합니다. Standalone에서는 오디오 설정에서 입력과 출력을 선택하고 소리를 넣어 보세요. 마이크 입력을 쓸 때는 헤드폰을 사용하세요.
2. 스펙트럼 아래 **Presets**를 열고 **Factory → Hall → Load**를 선택합니다. 짧은 공간감은 **Room**, 길고 넓은 잔향은 **Cloud**로 비교해 보세요.
3. **Decay**를 돌려 잔향 길이를 바꾸고 **Wet**으로 잔향의 양을 조절합니다. 원음은 **Dry**로 조절합니다. 센드/리턴 트랙에 꽂았다면 Dry를 0으로 두고 리턴 레벨로 섞어 보세요.
4. **Shimmer On**을 켜서 피치가 바뀐 잔향을 들어 보세요. 소스가 재생되는 중에 **Freeze**를 켜면 그때의 잔향이 유지됩니다. 원음은 계속 들리고, Freeze를 끄면 새 소리가 다시 잔향에 들어갑니다.

## 3. 소리 다듬고 저장하기

| 화면 또는 기능 | 하는 일 |
| --- | --- |
| **Reverb A/B** | A와 B의 리버브를 섞습니다. |
| **Basic / Advanced** | 제목 아래 버튼으로 전환합니다. Advanced에서 Damping, Early/Late Width, Multiband, Shimmer 등의 세부 설정을 엽니다. |
| **Damping** | 잔향의 고역이 얼마나 빨리 줄어드는지 조절합니다. |
| **Shimmer Mix** | 피치가 바뀐 소리의 양을 조절합니다. Advanced의 Shimmer 패널에서 각 보이스의 Pitch와 Level도 바꿀 수 있습니다. |
| **Presets → Save As** | 현재 설정을 사용자 프리셋으로 저장합니다. 다시 불러올 때는 **User**에서 골라 **Load**를 누릅니다. |

## 4. 문제가 생겼을 때

- **잔향이 안 들려요.** 트랙에 신호가 들어오는지 확인하고, DAW의 이펙트 바이패스와 PRE-GLOW의 Freeze 상태를 확인하세요. Wet을 올린 뒤 Hall 프리셋으로 비교해 보세요.
- **플러그인이 안 보여요.** ZIP을 푼 실제 번들을 형식에 맞는 경로에 넣었는지, Mac이 Apple Silicon인지 확인하고 DAW를 다시 시작하세요.
- **테스트 결과를 보내 주세요.** Mac 칩과 macOS 버전, DAW 이름과 버전, 사용 형식(AU/VST3/Standalone), 문제가 생긴 순서를 알려주면 재현에 도움이 됩니다. 오류가 떴다면 메시지도 함께 보내 주세요.
