# Lab 3 — 파이프라인 설계와 산출물 (3교시)

[🏠 랩 목록으로](../)

## 목표
needs·매트릭스·아티팩트·환경 승인·Release를 하나의 파이프라인으로 엮어봅니다.

## 준비 — 예제 소스 가져오기

Lab 3부터는 빌드할 소스가 필요합니다. 아래 파일들을 **본인 실습 레포 루트**에 넣습니다.

```
Makefile
include/logic.h
src/main.c
src/logic.c
tests/test_logic.c
```

이 파일들은 이 랩 자료 레포의 `starter/` 폴더에 있습니다.
아래 명령을 **본인 실습 레포 폴더 안에서** 실행하면 그 파일들만 내려받습니다. (docs는 안 받아옵니다)

```bash
BASE=https://raw.githubusercontent.com/skilleat-labs/actions-lab/main/starter
mkdir -p include src tests
curl -sSL $BASE/Makefile           -o Makefile
curl -sSL $BASE/include/logic.h    -o include/logic.h
curl -sSL $BASE/src/main.c         -o src/main.c
curl -sSL $BASE/src/logic.c        -o src/logic.c
curl -sSL $BASE/tests/test_logic.c -o tests/test_logic.c

# 커밋/푸시
git add Makefile include src tests
git commit -m "예제 소스 추가"
git push
```

---

## 3-A. 빌드 job과 아티팩트

### 해보기
`.github/workflows/pipeline.yml`:

```yaml
name: Lab3 파이프라인
on:
  workflow_dispatch:
  push:
    branches:
      - main

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - name: 테스트
        run: make test
      - name: 빌드
        run: make
      - name: 산출물 업로드
        uses: actions/upload-artifact@v7
        with:
          name: app
          path: build/app
          retention-days: 7
```

### 눈으로 확인
실행 후 **Summary** 하단에 `app` 아티팩트가 생김 → 클릭해 내려받을 수 있음.

### 왜
아티팩트는 job 격리(Lab 1-B)를 넘어 파일을 전달하고, 사람이 내려받게 하는 유일한 방법입니다.

### 🤔 생각해보기
방금 `src/` 소스와 `pipeline.yml`을 넣고 push했더니, Actions 탭에 **워크플로가 두 개**
(Lab2 CI, Lab3 파이프라인) 동시에 실행됐을 겁니다.

**왜 두 개가 동시에 돌았을까요? 이유를 생각해보세요.**
힌트: 각 워크플로 파일의 `on:` 을 다시 봅니다.

---

## 3-B. 매트릭스 — 조합을 병렬로

### 해보기
`build` job을 매트릭스로 바꿉니다.

```yaml
  build:
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        build_type:
          - Debug
          - Release
    steps:
      - uses: actions/checkout@v7
      - run: make test
      - run: make BUILD_TYPE=${{ matrix.build_type }}
      - uses: actions/upload-artifact@v7
        with:
          name: app-${{ matrix.build_type }}    # 매트릭스마다 이름이 달라야 함
          path: build/app
```

### 눈으로 확인
job이 **2개(Debug, Release)로 갈라져 병렬** 실행. 아티팩트도 2개.

> 함정: `name`에 `${{ matrix.build_type }}`를 빼면 이름이 겹쳐 업로드가 실패합니다.

> 그리고 이 `${{ matrix.build_type }}` 는 **문서 코드블록의 복사 버튼으로** 옮기세요.
> 화면 보고 `app-$` 처럼 치면 자동 치환이 안 돼, 산출물 이름이 글자 그대로 `app-$` 가 됩니다.

---

## 3-C. needs로 순서 만들기

### 해보기
빌드 결과를 받아 정리하는 job을 추가합니다.

```yaml
  collect:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - name: 모든 아티팩트 내려받기
        uses: actions/download-artifact@v8
        with:
          path: dist
      - run: |
          echo "## 산출물 목록" >> "$GITHUB_STEP_SUMMARY"
          find dist -type f | sort >> "$GITHUB_STEP_SUMMARY"
```

### 눈으로 확인
`build` 두 개가 끝난 뒤 `collect`가 시작. 실행 화면 상단 Summary에 파일 목록이 표로 뜸.

### 흐름 그림 — 산출물이 어떻게 오가나

```text
[build job: Debug]  ── build/app 만듦 ──►  업로드: app-Debug   ┐
                                                              ├─►  GitHub 저장소
[build job: Release] ── build/app 만듦 ──►  업로드: app-Release ┘        │
                                                                        │ (needs: build)
                                                                        ▼
                                        [collect job] ── download-artifact(path: dist) ──►  dist/ 로 다 내려받음
                                                                        │
                                                                        ▼
                                             find dist  ──►  Summary에 파일 목록 출력
```

- **업로드(upload)**: build job이 만든 `build/app`을 GitHub 저장소에 올림 (job이 끝나면 러너는 사라지므로, 파일을 남기려면 올려야 함)
- **다운로드(download)**: collect job이 그 산출물들을 `dist/` 폴더로 받아옴
- 여러 개라 하위 폴더로 들어감: `dist/app-Debug/app`, `dist/app-Release/app`
- **`needs: build`** 때문에 build 두 개가 **끝난 뒤에** collect가 시작함

> 핵심: `path: dist`는 "dist에 이미 있다"가 아니라 **"dist로 받아라"**. build가 올리고 → collect가 dist로 내려받는 흐름입니다.

---

## 🤔 퀴즈 — 이 워크플로를 실행하면 어떻게 될까요?

3-C까지 했으면 아래 워크플로를 **실행하기 전에** 세 질문에 먼저 답해보세요. 그다음 실제로 돌려서 맞았는지 확인합니다.

`.github/workflows/quiz.yml` (커밋 메시지에 `[skip ci]`):

```yaml
name: Lab3 퀴즈
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - run: echo "hello" > app.txt

  test:
    runs-on: ubuntu-latest
    steps:
      - run: echo "test"

  deploy:
    needs: [build, test]
    runs-on: ubuntu-latest
    steps:
      - run: cat app.txt
```

1. `build` 와 `test` 는 어떤 순서로 도나요?
2. `deploy` 의 `cat app.txt` 는 성공할까요, 실패할까요? 이유는?
3. 성공하게 만들려면 무엇을 바꿔야 하나요?

### 눈으로 확인
- Actions 탭 job 그래프: `build` 와 `test` 가 **나란히**, `deploy` 는 그 뒤에.
- `deploy` 는 **빨간불**. 로그: `cat: app.txt: No such file or directory`, exit code 1.

### 왜
- ① **병렬**: `needs` 가 없는 job은 동시에 시작합니다 (Lab 1-C).
- ② **실패**: `deploy` 는 다른 러너 = 다른 머신 = 다른 파일시스템. `build` 가 만든 파일이 거기 없습니다 (Lab 1-B).
- ③ **아티팩트**: `build` 에서 `upload-artifact`, `deploy` 에서 `download-artifact` (3-A, 3-C). 또는 `build` 와 `deploy` 를 한 job의 step으로 합치기 — 같은 머신이라 되지만 병렬을 포기하는 것.

### 고쳐서 다시 실행

```yaml
  build:
    runs-on: ubuntu-latest
    steps:
      - run: echo "hello" > app.txt
      - uses: actions/upload-artifact@v7
        with:
          name: app
          path: app.txt

  deploy:
    needs: [build, test]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v8
        with:
          name: app
      - run: cat app.txt          # hello
```

> **Job = 러너 1대 = 파일시스템 1개.** Lab 1에서 본 그 문장이 여기서 실제 에러로 나타납니다.
> (Jenkins 아시는 분: stage는 같은 workspace를 이어 쓰니 "된다"고 답하기 쉽습니다 — 여기가 다른 지점)

---

## 3-C+. 값은 outputs 로 — 파일 말고 "값"을 넘길 때

퀴즈에서 **파일**은 아티팩트로 넘겼습니다. 그런데 넘길 게 파일이 아니라 **작은 값**(버전 문자열, 태그, 해시)이면 아티팩트는 과합니다. 그때는 `outputs`.

값을 밖으로 내보내는 데는 세 층이 있습니다. 한 단계씩 바깥으로 공개합니다:

```
Step  ─ run: 에서 echo "name=값" >> "$GITHUB_OUTPUT"   →  같은 job 의 뒤 step 에서 steps.<id>.outputs.name
  ↓
Job   ─ jobs.<job>.outputs.공개이름: ${{ steps.<id>.outputs.name }}   →  다른 job 에서 needs.<job>.outputs.공개이름
  ↓
Workflow ─ on.workflow_call.outputs (Lab 5-B)   →  호출한 워크플로에서
```

### 해보기
`.github/workflows/outputs.yml` (커밋 메시지에 `[skip ci]`):

```yaml
name: Lab3 outputs
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    outputs:
      version: ${{ steps.ver.outputs.number }}      # ② step 값을 job 밖으로 공개 (이름을 바꿔도 됨)
    steps:
      - id: ver                                      # ① id 가 있어야 steps.ver 로 참조 가능
        run: echo "number=1.0.$GITHUB_RUN_NUMBER" >> "$GITHUB_OUTPUT"
      - run: echo "같은 job 에서 = ${{ steps.ver.outputs.number }}"

  deploy:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - run: echo "다른 job 에서 = ${{ needs.build.outputs.version }}"   # ③ needs.<job>.outputs.<공개이름>
```

### 눈으로 확인
- `build` 두 번째 step: `같은 job 에서 = 1.0.<실행번호>`
- `deploy`: `다른 job 에서 = 1.0.<실행번호>` — 파일 없이 값만 넘어옴

### 한 줄씩 뜻풀이

| 줄 | 뜻 |
|---|---|
| `id: ver` | 이 step 에 이름표. 없으면 `steps.ver` 로 부를 수 없음 |
| `>> "$GITHUB_OUTPUT"` | GitHub 이 주는 특별한 파일. 여기에 `이름=값` 한 줄을 쓰면 이 step 의 output 이 됨 |
| `outputs: version: ${{ steps.ver.outputs.number }}` | step 안 이름 `number` 를 job 밖 이름 `version` 으로 공개 |
| `needs.build.outputs.version` | 다른 job 은 **공개된 이름**으로만 읽음. `steps.ver...` 는 build job 안에서만 |

### 🤔 미니 체크 (답은 적지 말고 말로)

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    outputs:
      docker_tag: ${{ steps.tag.outputs.value }}
    steps:
      - id: tag
        run: echo "value=v2.0" >> "$GITHUB_OUTPUT"
```

1. `value` 는 무엇의 output 인가?
2. `docker_tag` 는 무엇의 output 인가?
3. 다른 job 에서 `v2.0` 을 읽는 전체 표현식은?

### 헷갈리는 것 둘

| | `$GITHUB_ENV` | `$GITHUB_OUTPUT` |
|---|---|---|
| 뜻 | **환경변수** — 뒤 step 의 셸에서 `$VERSION` 으로 | **output** — 이 step 이 계산한 결과를 공개 |
| 어디까지 | 같은 job 안 | job 밖(다른 job, 호출자)으로 승격 가능 |

| 값 | 파일 |
|---|---|
| `v1.0.3`, `abc123`, `production`, URL → **outputs** (job 당 1 MB 상한) | `app.hex`, `dist/`, `report.zip` → **artifact** |

> 남이 만든 액션이 output 을 주면(예: `steps.login.outputs.token`) 나는 `$GITHUB_OUTPUT` 을 쓸 일 없이 그냥 읽으면 됩니다. Lab 5-A 에서 우리가 만든 액션이 `size` 를 output 으로 주는 것이 그 예.

---

## 🔧 실무 도전 — 파일 이름에 정보 넣기 (선택)

### 왜
지금은 결과물 파일 이름이 그냥 `app`이라, 폴더(`app-Debug`)로만 구분됩니다.
실무에선 **파일 이름 자체**에 변형·버전·커밋을 박습니다. 이유:
- **오배포 방지** — `app`이 여러 개면 어느 걸 쓸지 헷갈림
- **추적성** — 파일만 봐도 "어느 빌드·어느 커밋"인지 알 수 있어야 함 (감사 대응)

목표 파일 이름 예: `app-Debug-r42-a1b2c3d` (타입 · 실행번호 · 짧은 커밋)

### 해보기
`build` job에서 **빌드 step 뒤에** 이름 바꾸는 step을 추가하고, 업로드 `path`를 바꿉니다.

```yaml
    steps:
      - uses: actions/checkout@v7
      - run: make test
      - run: make BUILD_TYPE=${{ matrix.build_type }}

      - name: 결과물 이름에 정보 넣기
        run: |
          SHA=$(echo "${{ github.sha }}" | cut -c1-7)
          mv build/app "build/app-${{ matrix.build_type }}-r${{ github.run_number }}-$SHA"

      - uses: actions/upload-artifact@v7
        with:
          name: app-${{ matrix.build_type }}
          path: build/app-*          # ← 이름이 바뀌었으니 glob(*)로 매칭
```

- `github.sha` = 이번 커밋 전체 SHA → `cut -c1-7` 로 앞 7자리만 (짧은 커밋)
- `github.run_number` = 이 워크플로의 실행 순번
- `mv` = 러너에서 `build/app` 파일 이름을 바꿈
- `path: build/app-*` = 이름이 바뀌었으니 `*`(와일드카드)로 잡음

### 눈으로 확인
다시 push하면, collect의 Summary 산출물 목록이 이렇게 바뀝니다:
```
dist/app-Debug/app-Debug-r43-a1b2c3d
dist/app-Release/app-Release-r43-a1b2c3d
```
→ 파일 이름만 봐도 **타입·실행번호·커밋**을 알 수 있습니다.

### 실무 대응
- 여기선 `app-Debug-r43-...` 지만, 임베디드 실무에선 `firmware-bcm-Release-v1.2.3-a1b2c3d.hex` 처럼
  **변형(ECU) · 타입 · 버전 · 커밋**을 파일명에 박습니다.
- 이렇게 하면 나중에 "이 hex가 뭐냐"를 파일 하나로 추적할 수 있습니다 (ISO 26262 / A-SPICE 감사 대응).

---

## 3-D. environment 승인 게이트

### 준비 (레포 설정) — 이 단계가 없으면 deploy가 안 멈춥니다

1. 레포 **Settings → Environments → New environment** → 이름 `production` 입력 → **Configure environment**
2. 설정 화면에서 **`Required reviewers` 체크박스를 켭니다** (기본은 꺼져 있음 — 여기가 핵심)
3. 체크하면 입력칸이 생깁니다 → **본인 GitHub 아이디**를 입력해 승인자로 추가
4. 초록색 **`Save protection rules`** 버튼 클릭

> ⚠️ `production` 환경만 만들고 **`Required reviewers`를 안 켜면**, 환경은 있어도 **승인 규칙이 없어서**
> deploy가 그냥 지나갑니다. 반드시 체크박스를 켜고 본인을 추가한 뒤 저장하세요.

### 해보기
배포 job을 추가합니다.

```yaml
  deploy:
    needs: collect
    runs-on: ubuntu-latest
    environment: production        # ← 여기서 승인 대기
    steps:
      - run: echo "승인이 나야 이 줄이 실행된다"
```

push 또는 Run workflow.

### 눈으로 확인
- push(또는 Re-run)하면 build, collect까지 돌고 **`deploy` job이 노란색 대기 상태로 멈춥니다.**
- 실행 화면 위쪽에 **`Review deployments`** 버튼이 뜸 → 클릭 → `production` 체크 → **Approve and deploy**
- 승인하면 그제서야 `deploy`가 시작됩니다.
- 만약 **안 멈추고 초록불로 지나갔다면** → 위 "준비" 단계에서 `Required reviewers`를 안 켠 것입니다. 다시 확인하세요.

### 왜
**승인 전에는 job 자체가 시작되지 않습니다.** 러너도 안 잡고, 그 환경의 시크릿에 접근조차 못 합니다 (Lab 4와 연결).
"승인이 나면 그때부터 시작"이지 "실행 중에 멈춰서 기다림"이 아닙니다.
(Jenkins 아시는 분: `input` 과 역할은 같습니다. 다른 점 세 가지 — ① 통제 위치: `input` 은 Jenkinsfile 코드 안이라 파이프라인 작성자가 지울 수 있지만, environment 규칙은 Settings 에 있어 워크플로 파일로는 우회 불가 ② 비밀값: `input` 전에 이미 credentials 가 로드된 경우가 많지만, 환경 시크릿은 승인 뒤에야 풀림 ③ 감사: 승인자/시각이 배포 이력에 자동 기록. 즉 "통제 지점이 코드에서 설정으로" 옮겨 가는 것이고, `input` 뒤에 하던 일은 승인 뒤 job 의 step 으로 그대로 옮기면 됩니다)

### 리뷰어에게는 어떻게 알림이 가나
- `Required reviewers`로 지정된 사람에게 **자동으로 알림**이 갑니다.
  - **GitHub 알림**(우상단 🔔) + **이메일**(리뷰어의 알림 설정에 따라)
- 리뷰어는 알림이나 실행 화면의 **`Review pending deployments`** 를 눌러 승인/거부합니다.
- 여러 명을 지정하면 **그중 한 명만 승인**하면 진행됩니다 (전원 승인 아님).
- 승인 없이 두면 최대 **30일** 대기 후 자동 취소됩니다.

### 실무 대응
- 혼자가 아니라 **팀이나 여러 명**을 리뷰어로 지정합니다 (예: 배포 담당팀).
- 사내에선 이 승인이 **Manual Approval / 결재**에 대응합니다.
- 알림을 **Slack/Teams**로 받고 싶으면 GitHub 알림 연동이나 별도 앱을 씁니다.

---

## 3-E. GitHub Release 만들기

### 해보기
`deploy` job을 Release 생성으로 바꿉니다.

```yaml
  deploy:
    needs: collect
    runs-on: ubuntu-latest
    environment: production
    permissions:
      contents: write            # 릴리스 생성 권한 (없으면 403)
    steps:
      - uses: actions/download-artifact@v8
        with:
          pattern: app-*
          path: dist
          merge-multiple: true
      - name: Release 생성
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          gh release create "v0.${{ github.run_number }}" \
            --repo ${{ github.repository }} \
            --title "빌드 v0.${{ github.run_number }}" \
            --generate-notes \
            dist/*
```

> `--repo ${{ github.repository }}` 가 중요합니다. deploy job은 `checkout`을 안 해서 `.git`이 없는데,
> 이걸 안 주면 `gh`가 "어느 레포인지" 못 찾아 `fatal: not a git repository` 로 실패합니다.
> (또는 `- uses: actions/checkout@v7` 를 먼저 넣어도 되지만, Release만 만들 거면 `--repo`가 더 가볍습니다.)

### 눈으로 확인
승인 후, 레포 **Releases** 에 새 릴리스가 생기고 산출물이 첨부됨. 릴리스 노트는 자동 생성.

### 왜
정확히 말하면 Jenkins가 **못 하는** 게 아니라 **나눠서** 합니다.
- Jenkins: 빌드 기록(archiveArtifacts) + 외부 저장소(Artifactory/Nexus) + 위키(변경 내역) 로 분산
- GitHub Release: **태그 + 변경 내역 + 파일이 소스 저장소 안 한 화면**에, 커밋으로 바로 이동

> 그렇다고 Artifactory를 버리는 건 아닙니다. 규제상 사내에 정본을 둬야 하면 **Release와 Artifactory를 같이** 쓰면 됩니다 (Lab 4에서 다룸).

---

## 실무 대응
- `build/app` → 실무에서는 실제 빌드 산출물(바이너리, 컨테이너 이미지 등).
- `gh release create` → 사내 **아티팩트 저장소 업로드 + 배포·형상 시스템 전송**으로 바뀝니다 (Lab 4).
- environment 승인 → 사내 **Manual Approval / 결재**에 그대로 대응됩니다.

## 🧹 다음 랩으로 가기 전에

이 랩의 `pipeline.yml`은 `on: push` 라서, **이후 랩에서 워크플로 파일을 push할 때마다 같이 실행되고 `production` 승인 대기에 걸립니다.**
둘 중 하나로 정리해 두세요:

- 커밋 메시지에 **`[skip ci]`** 를 붙여 push (push 트리거만 건너뜀, Run workflow는 그대로) — Lab 4 첫 절에서 자세히
- 또는 Actions 탭 → `Lab3 파이프라인` → 우측 **…** → **Disable workflow** (다시 쓸 때 Enable)

## 체크리스트
- [ ] 아티팩트를 업로드/다운로드했다
- [ ] 매트릭스로 job이 갈라지는 것을 봤다
- [ ] 퀴즈: deploy의 cat이 왜 실패하는지 말할 수 있다
- [ ] outputs 로 값을 다른 job 에 넘겨봤다 (값 = outputs, 파일 = artifact)
- [ ] deploy가 승인 대기에서 멈추는 것을 봤다
- [ ] Release가 생성되고 파일이 첨부된 것을 봤다

## 🔧 도전 과제

> 기본 실습을 마쳤으면 아래를 **답 코드 없이** 해봅니다.
> 이 랩에서 배운 것에 **📖 공식 문서**(또는 검색)를 조금 더하면 풀 수 있는 실무형 과제입니다.
>
> ★ 5분 · ★★ 10분 · ★★★ 15분 — 다 못 해도 됩니다. 시간이 남는 분이 하는 과제입니다.

---

### 도전 1 · ★★ — 매트릭스에 축 하나 더

`build_type` 에 `target` 축을 추가해 조합을 늘리되 특정 조합은 제외합니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `build_type(Debug, Release)` × `target(MCU, AP)` 4조합 생성, **`Debug × AP` 제외**, 아티팩트 이름 겹침 없음 |
| **완료 조건** | build job이 **3개**로 갈라지고, 아티팩트 3개 이름에 두 축이 모두 포함 (`echo "target=${{ '{{' }} matrix.target {{ '}}' }}"` 으로 대신) |

??? tip "힌트"
    - `strategy.matrix` 의 `exclude` 키 (반대는 `include`)
    - 실무에서 ECU × 빌드타입 × 컴파일러 조합이 이 형태입니다

---

### 도전 2 · ★★ — 테스트가 실패해도 로그는 남기기

테스트가 **실패해도** 로그 파일이 아티팩트로 올라가게 만듭니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `make test` 출력을 `test.log`에 저장, 테스트 실패해도 아티팩트로 업로드 (실패는 `tests/test_logic.c` CHECK 하나 틀리게) |
| **완료 조건** | job 빨간불인데 Summary 하단에 `test-log` 아티팩트가 있음, 내려받으면 실패한 CHECK가 적혀 있음 |

??? tip "힌트"
    - `tee` 명령으로 출력을 파일에 동시 저장
    - step `if:` 의 상태 함수: `always()` vs `failure()` — "성공/실패 상관없이"는 어느 쪽?
    - Jenkins 경험자: `post { always { archiveArtifacts } }` 자리

---

### 도전 3 · ★ — 빌드 시간 제한

build job이 **3분**을 넘기면 자동으로 취소되게 합니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | build job에 3분 시간 제한 설정, `sleep 200` step으로 실제로 끊기는지 확인 |
| **완료 조건** | 3분쯤에 job이 취소(빨간 X)되고 로그에 "timed out" 류 메시지 |

??? tip "힌트"
    - job 레벨 키 하나로 설정합니다
    - 기본값은 **360분** — 임베디드 빌드가 걸려서 6시간 동안 러너를 점령하는 사고를 막는 키입니다

---

### 도전 4 · ★★★ — 릴리스 노트 자동 생성

3-E의 Release에 커밋 기반 **본문(노트)** 을 자동으로 채웁니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | ① 이전 릴리스 이후 커밋 목록 자동 삽입 ② 첫 줄에 빌드 타입 + 커밋 SHA 앞 7자리 포함 |
| **완료 조건** | Releases 페이지에서 노트가 채워진 릴리스가 보임 |

??? tip "힌트"
    - `gh release create` 옵션 중 노트를 **자동 생성**하는 것과 **직접 넣는** 것이 있습니다
    - SHA 7자리: `${{ '{{' }} github.sha {{ '}}' }}` 를 셸에서 자르면 됩니다 (Lab 3 실무 도전과 같은 기법)

## 📖 공식 문서

- [needs, strategy.matrix 문법](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)
- [아티팩트 업로드/다운로드](https://github.com/actions/upload-artifact)
- [환경(environment)과 승인 게이트](https://docs.github.com/en/actions/how-tos/deploy/configure-and-manage-deployments/manage-environments)

<!-- NAV -->

---

[← Lab 2 · 워크플로 구조와 UI](lab2-structure-ui.md)  ·  [🏠 랩 목록](../)  ·  [Lab 4 · 외부 연동과 보안 →](lab4-secrets-security.md)

