# Lab 2 — 워크플로 구조와 UI (2교시)

[🏠 랩 목록으로](../)


## 목표
필터로 **안 도는 것**과 **실패한 것**의 차이를 확인하고, Actions 탭에서 로그·재실행을 다뤄봅니다.

---

## 2-A. 필터 — 언제 안 돌지를 정한다

### 해보기
`.github/workflows/ci.yml`을 만듭니다. `src/` 아래가 바뀔 때만 돌게 합니다.

```yaml
name: Lab2 CI
on:
  push:
    paths:
      - 'src/**'
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - run: echo "빌드 실행됨 (src가 바뀜)"
```

### 이 트리거가 무슨 뜻일까 (한 줄씩)

1교시에서는 `workflow_dispatch`(수동)만 배웠죠. 여기서는 트리거가 **두 개**입니다.

| 키 | 뜻 |
|---|---|
| `on:` | 트리거는 **여러 개**를 나열할 수 있습니다. 여기서는 `push` 와 `workflow_dispatch` 둘 다. |
| `push:` | 코드를 **push할 때마다 자동 실행**. (Jenkins 아시는 분: SCM 트리거) |
| `paths:` | push 중에서도 **여기 지정한 파일이 바뀐 경우에만** 실행. |
| `- 'src/**'` | `src/` 폴더 아래(하위 폴더 포함)가 바뀌면 실행. `README.md`만 바뀌면 안 돎. |

| `workflow_dispatch:` | 1교시에서 배운 **수동 실행 버튼**. |

---

### `paths` vs `paths-ignore` — 방화벽으로 이해하기

저장소 구조가 아래와 같다고 가정합니다.

```text
repo/
├─ src/
│  ├─ app.js
│  └─ util.js
├─ docs/
│  └─ guide.md
└─ README.md
```

**`paths:` — 허용 목록 ("이것만 봐")**

```yaml
on:
  push:
    paths:
      - 'src/**'
```

| 변경 파일 | 실행 여부 |
|-----------|----------|
| `src/app.js` | ✅ 실행 |
| `src/util.js` | ✅ 실행 |
| `docs/guide.md` | ❌ 안 함 |
| `README.md` | ❌ 안 함 |

**`paths-ignore:` — 제외 목록 ("이것만 빼")**

```yaml
on:
  push:
    paths-ignore:
      - 'docs/**'
      - 'README.md'
```

| 변경 파일 | 실행 여부 |
|-----------|----------|
| `src/app.js` | ✅ 실행 |
| `src/util.js` | ✅ 실행 |
| `docs/guide.md` | ❌ 안 함 |
| `README.md` | ❌ 안 함 |

!!! info "어느 쪽을 쓸지 고르는 기준"
    ```text
    "소스가 src/에만 있다"  →  paths: ['src/**']
    "빌드와 무관한 건 문서뿐"  →  paths-ignore: ['docs/**', '*.md']
    ```
    짧고 잘 안 바뀌는 쪽을 목록으로 씁니다.

**같은 이벤트에 둘을 동시에 쓸 수 없다**

```yaml
# ❌ 이렇게 하면 오류
on:
  push:
    paths:
      - 'src/**'
    paths-ignore:
      - 'src/**/*.md'
```

이럴 때는 `paths:` 안에서 `!` 패턴으로 제외합니다.

```yaml
# ✅ 이렇게
on:
  push:
    paths:
      - 'src/**'
      - '!src/**/*.md'
```

| 변경 파일 | 실행 여부 |
|-----------|----------|
| `src/app.js` | ✅ 실행 |
| `src/util.js` | ✅ 실행 |
| `src/docs/readme.md` | ❌ 안 함 |

??? warning "`!` 패턴에서 순서가 중요한 이유"
    패턴은 **위에서 아래로 마지막 매칭이 이깁니다.**

    ```yaml
    paths:
      - 'src/**'         # ① src 전체 허용
      - '!src/**/*.md'   # ② 그 중 .md 제외  → ②가 마지막이므로 .md는 제외
    ```

    순서를 바꾸면 결과가 달라집니다.

    ```yaml
    paths:
      - '!src/**/*.md'   # ① .md 제외 (아직 아무것도 허용 안 함)
      - 'src/**'         # ② src 전체 허용  → ②가 마지막이므로 .md도 실행됨 ⚠️
    ```

    **규칙: 넓은 허용 먼저, 좁은 제외 나중.**

---

### `branches` / `branches-ignore` — 같은 원리

`paths` / `paths-ignore` 와 완전히 동일한 구조입니다.

```yaml
# main 브랜치 push만 실행
on:
  push:
    branches:
      - main
```

```yaml
# docs 브랜치 push만 제외하고 실행
on:
  push:
    branches-ignore:
      - docs
```

??? question "`branches`와 `paths`를 같이 쓰면 AND 조건인가요?"
    **네, AND 조건입니다.**

    ```yaml
    on:
      push:
        branches:
          - main
        paths:
          - 'src/**'
    ```

    위 설정의 실행 조건:

    | 브랜치 | 변경 파일 | 실행 여부 |
    |--------|-----------|----------|
    | `main` | `src/app.js` | ✅ 실행 |
    | `main` | `README.md` | ❌ 안 함 (paths 불통과) |
    | `feature/x` | `src/app.js` | ❌ 안 함 (branches 불통과) |
    | `feature/x` | `README.md` | ❌ 안 함 (둘 다 불통과) |

    즉 **"main 브랜치에 src/ 아래 파일이 바뀔 때"만** 실행됩니다.

---

### 한 장으로 정리

| | `push` + `paths` | `workflow_dispatch` |
|---|---|---|
| **실행 주체** | 자동 (파일 변경 감지) | 사람 (Run workflow 버튼) |
| **필터 방향** | `paths` 허용 / `paths-ignore` 제외 | 필터 없음 (항상 실행 가능) |
| **기록 없음** | 필터 불통과 시 기록 자체 미생성 | 버튼이 없으면 실행 불가 |
| **실무 용도** | CI — 소스 변경 시 자동 빌드 | 수동 배포, 긴급 재실행 |

> **"`paths`는 실행 대상을 고르는 허용 목록이고, `paths-ignore`는 실행에서 뺄 대상을 고르는 제외 목록입니다."**

---

### "둘 다 쓰면 둘 다 되나요?" — 네

`on:` 아래에 여러 트리거를 두면 **각각 독립적으로** 동작합니다. 그래서:

| 워크플로에 넣은 것 | 자동(push) | 수동(Run workflow 버튼) |
|---|---|---|
| `push:` 만 | ✅ 됨 | ❌ 버튼 안 생김 |
| `workflow_dispatch:` 만 | ❌ 자동 안 됨 | ✅ 됨 |
| **둘 다** (이 예제) | ✅ 됨 | ✅ 됨 |

> 즉 지금 이 워크플로는 **`src/`를 고쳐 push하면 자동으로 돌고**, 그것과 별개로 **Run workflow 버튼으로 아무 때나 수동 실행**도 됩니다.
> `workflow_dispatch:` 를 빼면 자동(push)은 그대로 되지만 **수동 실행 버튼은 사라집니다.**

---

먼저 `src/dummy.txt` 같은 파일을 만들어 push → 워크플로가 **돕니다**.
그다음 `README.md`만 고쳐서 push → 워크플로가 **안 뜹니다**.

### 눈으로 확인
- `src/` 변경 push: Actions 탭에 실행 기록이 생김
- `README.md`만 push: Actions 탭에 **아무 기록도 없음**

### 왜
`paths` 필터를 통과하지 못하면 워크플로는 **아예 실행되지 않습니다**.
이것은 "실패"가 아니라 "부재"입니다. 빨간 X도 없고, 기록 자체가 없습니다.
현업에서 "워크플로가 안 돌아요" 문의의 1순위가 이 필터 또는 파일 위치입니다.

---

## 2-B. 빨간불 만들고 로그 읽기

### 해보기
`build` job에 일부러 실패하는 step을 추가합니다.

```yaml
      - name: 일부러 실패
        run: |
          echo "여기까지는 정상"
          exit 1
      - name: 이 step은 실행될까
        run: echo "앞이 실패하면 나는 건너뛴다"
```

`src/` 아래 파일을 고쳐 push (또는 workflow_dispatch).

### 눈으로 확인
- job이 **빨간 X**로 실패
- 실패한 step 로그를 펼치면 어디서 멈췄는지 보임
- 그 뒤 step은 **skipped** (회색)

### 왜
step은 위에서 아래로 순차 실행되고, 하나가 실패하면(0이 아닌 종료코드) 이후 step은 기본으로 건너뜁니다.

---

## 2-C. 고치고 재실행

### 해보기
`exit 1`을 `exit 0`으로 고치고 push. 또는 고치지 말고 실패한 실행 화면에서
**Re-run jobs → Re-run failed jobs** 를 눌러봅니다.

### 눈으로 확인
- **Re-run failed jobs**: 실패한 job만 다시 돕니다 (성공한 job은 건너뜀). 시간 절약.
- **중요 — 재실행(Re-run)은 "그때 그 커밋"을 그대로 다시 돌립니다.**
  - 그래서 워크플로나 코드를 고쳐도, 재실행은 **고치기 전 버전**으로 돕니다.
  - 고친 내용을 반영하려면 **새로 커밋해서 push** 해야 합니다. 그러면 **새 실행**이 하나 생깁니다.

> 비유: 재실행 = 이미 찍은 녹화본을 다시 재생. 대본(코드)을 고쳐도 이미 찍힌 녹화는 안 바뀝니다.
> 새 버전으로 찍으려면(= 새 실행) **새 커밋을 push**해야 합니다.
>
> 정리:
> - 똑같은 코드로 그냥 다시 → **Re-run**
> - 고쳐서 돌리고 싶다 → **새 커밋 push**

---

## 2-D. workflow_dispatch 입력값

### 왜 이걸 하나
1교시의 `workflow_dispatch`는 그냥 "수동으로 누르면 실행"이었습니다.
여기에 **입력값(inputs)**을 붙이면, **Run workflow 버튼을 눌렀을 때 작은 폼이 먼저 뜨고**(드롭다운, 체크박스, 텍스트),
거기서 고른 값이 워크플로 안으로 들어옵니다. "실행하기 전에 값을 물어보는 버튼"이라고 생각하면 됩니다.
(Jenkins 아시는 분: Build Now 가 Build with Parameters 로 바뀌는 그것, 즉 파라미터 빌드입니다)

push 트리거는 사람이 개입할 틈이 없습니다 — 코드가 올라오면 정해진 대로 돕니다.
그런데 실무에선 **"이번엔 이렇게 돌려줘"라고 사람이 지정해야 하는 순간**이 있습니다:

| 상황 | Run workflow 폼에서 고르는 것 | 워크플로 안에서 |
|---|---|---|
| 오늘은 BCM만 급하게 빌드 | `ecu: [BCM ▾]` | `make TARGET=${{ inputs.ecu }}` |
| 공식 버전 번호를 사람이 지정 | `version: [1.4.2]` | `gh release create v${{ inputs.version }}` |
| 핫픽스라 20분짜리 테스트 생략 | `skip_tests: [✓]` | 테스트 step에 `if: inputs.skip_tests == false` |
| 점검 중인 HIL 장비 피해서 | `bench: [HIL-3 ▾]` | `runs-on: [self-hosted, ${{ inputs.bench }}]` |
| staging에 올릴지 production에 올릴지 | `target: [staging ▾]` | `environment: ${{ inputs.target }}` |

공통점: **워크플로 파일은 하나**인데 실행할 때 고른 값에 따라 다르게 돕니다. 입력값이 없으면 ECU마다 워크플로 파일을 따로 만들어야 합니다.
`build_type`(Release/Debug)은 이 중 가장 단순한 예시일 뿐이고, 이 실습의 포인트는 **"수동 실행할 때 사람이 값을 골라 넘기는 법"** 입니다.

> 비유: `workflow_dispatch:` 만 있으면 "누르면 기본 아메리카노가 나오는 버튼". `inputs:` 를 붙이면 누를 때 "사이즈? 샷 추가?"를 먼저 묻는 키오스크 화면.

### 해보기
`on:`에 입력을 추가합니다.

```yaml
on:
  workflow_dispatch:
    inputs:
      build_type:
        description: 빌드 타입
        type: choice
        options:
          - Release
          - Debug
        default: Release
```

step에서 사용:

```yaml
      - run: echo "빌드 타입은 ${{ inputs.build_type }}"
```

Actions 탭에서 **Run workflow** 를 누르면 드롭다운이 나옵니다.

### 눈으로 확인
- **Run workflow** 를 누르면 드롭다운(Release/Debug)이 뜨고, 고른 값이 로그에 `빌드 타입은 Release` 처럼 찍힙니다.
- 즉 사람이 고른 값이 `${{ inputs.build_type }}` 로 워크플로 안에 들어옵니다.

### 📖 공식 문서
- 수동 실행 입력 정의 (`type`, `options`, `default`): [Events that trigger workflows — workflow_dispatch](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#workflow_dispatch)
- 워크플로 안에서 값 꺼내기 (`inputs` 컨텍스트): [Contexts](https://docs.github.com/en/actions/reference/workflows-and-actions/contexts) — 표에서 `inputs` 항목 참고
- `on.workflow_dispatch.inputs` 문법: [Workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)

---

## 2-E. Actions 탭 둘러보기

한 번씩 눌러보며 어디에 무엇이 있는지 익힙니다.
- **Summary** — job 그래프, 소요 시간, 아티팩트
- **각 step 로그** — 접기/펼치기
- **Workflow file** 탭 — 그 실행에 쓰인 워크플로 원본 ("분명 고쳤는데 옛날대로 돈다"일 때 확인)
- 우측 **…** → **Disable workflow** — 워크플로 일시 정지

---

## 2-F. 관례 — YAML은 흐름만, 로직은 스크립트로

YAML에는 `if`/`for` 같은 로직이 없습니다(있어도 `if:` 조건 정도). 그래서 GitHub Actions의 관례는
**복잡한 로직은 셸 스크립트 파일로 빼고, YAML에는 "무엇을 순서대로 하나"(흐름)만 적는 것**입니다.
같은 일을 두 방식으로 비교해 봅니다.

### ❌ 로직을 YAML 안에 다 넣으면

```yaml
      - name: 전체 ECU 빌드 (로직이 YAML 안에)
        run: |
          if [ "${{ github.ref_name }}" = "main" ]; then TYPE=Release; else TYPE=Debug; fi
          for ecu in bcm ipc adas; do
            echo "== $ecu ($TYPE) 빌드 =="
            SIZE=$((RANDOM % 300000))
            if [ "$SIZE" -gt 200000 ]; then
              echo "::error::$ecu 크기 초과: $SIZE"; exit 1
            fi
          done
```

돌긴 합니다. 하지만 `${{ }}` 때문에 **GitHub에서만** 실행되고(노트북에서 못 돌림), YAML 안에서 if/for를 리뷰해야 하고, Jenkins에서 같은 일을 하려면 다시 짜야 합니다.

### ✅ 해보기 — 스크립트로 빼기

1. 실습 레포에 `scripts/build-all.sh` 를 만듭니다. (`src/` 가 아니라 `scripts/` 입니다)

```bash
#!/usr/bin/env bash
set -euo pipefail
BRANCH="${1:-dev}"                       # 첫 번째 인자, 없으면 dev

if [ "$BRANCH" = "main" ]; then TYPE=Release; else TYPE=Debug; fi

for ecu in bcm ipc adas; do
  echo "== $ecu ($TYPE) 빌드 =="
  SIZE=$((RANDOM % 300000))              # 실제 빌드 대신 크기를 흉내
  if [ "$SIZE" -gt 200000 ]; then
    echo "::error::$ecu 크기 초과: $SIZE"
    exit 1
  fi
  echo "   크기 $SIZE OK"
done
```

2. `ci.yml` 의 `build` job에 step을 추가합니다. YAML에는 **스크립트를 부르는 한 줄**만.

```yaml
      - name: 전체 ECU 빌드 (흐름만)
        env:
          BRANCH: ${{ github.ref_name }}
        run: bash scripts/build-all.sh "$BRANCH"
```

3. 커밋/푸시 (`scripts/` 변경은 `src/**` 필터에 안 걸리니 **Run workflow** 로 실행).

### 눈으로 확인
- 로그에 `== bcm (Release) 빌드 ==` 처럼 세 ECU가 차례로. 가끔(약 1/3 확률) 크기 초과로 빨간불 + Annotations에 `::error::` 메시지 — 재실행하면 달라짐.
- **노트북에서도 그대로 실행됩니다**: `bash scripts/build-all.sh main` → 같은 출력. GitHub에 push하고 기다릴 필요 없이 디버깅.

### 왜

| | YAML 안에 로직 | 스크립트로 분리 |
|---|---|---|
| 로컬 실행 | 불가 (`${{ }}` 때문) | 가능 — 디버깅이 빠름 |
| 리뷰 | 40줄 YAML 안의 if/for | 흐름(YAML)과 로직(sh) diff가 분리 |
| Jenkins 병행 | 다시 짜야 함 | Jenkins도 `sh 'bash scripts/build-all.sh'` 로 같은 스크립트 |
| 보안 | `${{ }}` 를 run 안에 직접 → 인젝션 위험 (Lab 4) | `env:` 로 넘기고 스크립트는 `$BRANCH` 만 봄 |

스크립트가 실패했는지는 **종료 코드**로 압니다. `set -e` 덕에 첫 실패에서 `exit 1` → step 실패 → job 실패.

### 실무 대응
- Jenkinsfile의 Groovy 블록 안 로직을 `sh './script.sh'` 로 빼던 습관과 같습니다. YAML은 로직을 못 쓰니 강제로 그렇게 됩니다.
- 전환 기간에 **Jenkins와 Actions가 같은 스크립트를 부르면** 산출물이 같은지 비교하기 쉽습니다 (Lab 7).
- 여러 리포에서 같은 스크립트가 반복되면 → composite action 으로 승격 (Lab 5).
- Windows에서 커밋하면 실행 권한(`chmod +x`)이 빠질 수 있어, `./scripts/x.sh` 대신 `bash scripts/x.sh` 로 부르는 게 안전합니다.

---

## 실무 대응
- 필터(`paths`, `branches`)는 여러 모듈을 한 레포에 담을 때 "바뀐 모듈만 빌드"에 그대로 씁니다.
- 실패 지점이 코드 줄에 표시되는 것(Annotations)은 Lab 4의 정적분석 출력과 연결됩니다.

## 체크리스트
- [ ] README만 고치면 워크플로가 안 뜨는 것을 봤다
- [ ] 빨간불과 skipped step을 봤다
- [ ] Re-run failed jobs를 써봤다
- [ ] workflow_dispatch 입력값을 넣어봤다
- [ ] 로직을 스크립트로 빼고 YAML에는 호출 한 줄만 남겨봤다


## 🔧 도전 과제

> 기본 실습을 마쳤으면 아래를 **답 코드 없이** 해봅니다.
> 이 랩에서 배운 것에 **📖 공식 문서**(또는 검색)를 조금 더하면 풀 수 있는 실무형 과제입니다.
>
> ★ 5분 · ★★ 10분 · ★★★ 15분 — 다 못 해도 됩니다. 시간이 남는 분이 하는 과제입니다.

---

### 도전 1 · ★ — 문서만 고치면 CI가 안 돌게

`*.md` / `docs/` 변경은 CI를 건너뛰고, `src/` 변경만 CI가 돌게 만듭니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `*.md`·`docs/` 변경 → CI 건너뜀, `src/` 변경 → CI 실행 |
| **완료 조건** | README만 고쳐 push → Actions 탭에 아무것도 안 생김. `src/logic.c` 고쳐 push → 실행됨 |

??? tip "힌트"
    - `paths:` 의 반대 키가 있습니다
    - 단, 두 키를 **같은 트리거에 같이 쓸 수 없다**는 제약이 문서에 있습니다 — 그럼 어떻게 써야 할까요?

---

### 도전 2 · ★★ — 야간 빌드

매일 **한국 시간 새벽 2시**에 자동 실행되는 트리거를 추가하고, 그 실행에서만 특별 메시지를 출력합니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | 새벽 2시(KST) 자동 실행 + 그 실행에서만 `"야간 빌드입니다"` 출력 step 추가 |
| **완료 조건** | ① cron 문자열이 UTC로 올바름 ② 해당 step에 `if:` 조건이 있어 수동 실행에서는 회색 ③ 파일이 기본 브랜치에 있음 |

??? tip "힌트"
    - `schedule` 트리거, cron은 **UTC 기준** (KST = UTC+9, 새벽 2시 → `17 * * * *`)
    - `github.event_name` 컨텍스트로 실행 원인 구분
    - Jenkins 경험자: `triggers { cron }` 자리
    - 실제 새벽 실행은 다음 날 Actions 탭에서 확인

---

### 도전 3 · ★★ — 수동 실행에 체크박스 입력 추가

`workflow_dispatch`에 **`verbose` 체크박스**를 달아, 켰을 때만 환경변수 전체를 출력하는 step이 돕니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `verbose`(기본 꺼짐) 체크박스 추가, 켜면 `env` 전체 출력 step 실행, 끄면 건너뜀 |
| **완료 조건** | Run workflow 창에 체크박스가 보이고, 켰을 때만 해당 step 초록 / 껐을 때는 회색 |

??? tip "힌트"
    - 입력 `type:` 에 `choice` 말고 어떤 값들이 있는지 문서 확인
    - step `if:` 에서 `inputs.verbose` 참조 방법 확인


## 📖 공식 문서

- [트리거 이벤트와 필터 (`paths`, `paths-ignore`, `branches`)](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows)
- [`schedule` 트리거 (cron, UTC 기준)](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#schedule)
- [`workflow_dispatch` 입력 (`type: boolean` 등)](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#workflow_dispatch)
- [step `if:` 조건과 컨텍스트](https://docs.github.com/en/actions/reference/workflows-and-actions/contexts)

<!-- NAV -->

---

[← Lab 1 · 첫 워크플로와 러너 관찰](lab1-first-workflow.md)  ·  [🏠 랩 목록](../)  ·  [Lab 3 · 파이프라인 설계와 산출물 →](lab3-pipeline-artifacts.md)

