<!-- {% raw %} -->
# Lab 5 — 표준화와 재사용 (5교시)

[🏠 랩 목록으로](../)


## 목표
Lab 3에서 만든 파이프라인을 **composite action**과 **reusable workflow**로 리팩터링하고, **cache**로 시간을 줄입니다.

---

## 5-A. composite action — 반복 step 묶기

### 해보기
`.github/actions/build-app/action.yml` 을 만듭니다.

```yaml
name: 앱 빌드
description: 체크아웃 후 테스트하고 빌드한다
inputs:
  build_type:
    description: Release 또는 Debug
    required: false
    default: Release
outputs:
  size:
    description: 산출물 크기(byte)
    value: ${{ steps.info.outputs.size }}
runs:
  using: composite
  steps:
    - uses: actions/checkout@v7
    - shell: bash                    # composite step은 shell 필수
      run: make test
    - shell: bash
      run: make BUILD_TYPE=${{ inputs.build_type }}
    - id: info
      shell: bash
      run: echo "size=$(stat -c%s build/app)" >> "$GITHUB_OUTPUT"
```

워크플로에서 사용:

```yaml
name: Lab5 표준
on:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - id: b
        uses: ./.github/actions/build-app
        with:
          build_type: Release
      - run: echo "산출물 크기 ${{ steps.b.outputs.size }} byte"
```

### 눈으로 확인
여러 step이 액션 하나(`uses: ./.github/actions/build-app`)로 줄어듦. 출력값도 받아옴.

> 주의: 같은 레포 액션을 쓰려면 워크플로에서 먼저 `checkout`이 필요합니다.
> (위 예제는 액션 내부에서 checkout하므로, 워크플로에서 별도 checkout 없이 동작합니다.)

---

## 5-B. reusable workflow — 파이프라인 통째로 표준화

### 해보기
`.github/workflows/reusable-build.yml`:

```yaml
name: 재사용 빌드
on:
  workflow_call:
    inputs:
      build_type:
        type: string
        required: true
    outputs:
      size:
        value: ${{ jobs.build.outputs.size }}
jobs:
  build:
    runs-on: ubuntu-latest
    outputs:
      size: ${{ steps.b.outputs.size }}
    steps:
      - id: b
        uses: ./.github/actions/build-app
        with:
          build_type: ${{ inputs.build_type }}
```

호출하는 워크플로 `.github/workflows/caller.yml`:

```yaml
name: Lab5 호출
on:
  workflow_dispatch:
jobs:
  release-build:
    uses: ./.github/workflows/reusable-build.yml
    with:
      build_type: Release
  debug-build:
    uses: ./.github/workflows/reusable-build.yml
    with:
      build_type: Debug
  gate:
    needs:
      - release-build
    runs-on: ubuntu-latest
    steps:
      - run: echo "Release 산출물 크기 ${{ needs.release-build.outputs.size }} byte"
```

### 눈으로 확인
호출하는 쪽은 `uses:` 한 줄. 재사용 워크플로가 Debug/Release 두 번 실행되고, 출력값을 되받아옴.

### 왜
"복사"가 아니라 "참조"입니다. 표준 파이프라인을 한 곳(`reusable-build.yml`)에 두고 여러 워크플로가 `uses:` 로 부르니, 한 번 고치면 전부 반영됩니다.
둘의 구분: **job 단위로 묶을 것(러너, 순서, 승인이 있음) → reusable workflow**, **step 단위로 묶을 것(같은 러너에서 연달아) → composite action**.
(Jenkins 아시는 분: shared library 자리. library 하나가 보통 이 둘로 쪼개집니다)

---

## 5-C. cache — 반복 작업 시간 줄이기

이 예제는 빌드가 워낙 빨라 캐시 효과가 작지만, **동작과 함정**을 확인합니다.

### 해보기
빌드 산출물 디렉터리를 캐시해 봅니다. (실무에선 툴체인/의존성을 캐시)

```yaml
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - name: 캐시
        id: cache
        uses: actions/cache@v4
        with:
          path: build
          key: build-${{ hashFiles('src/**', 'Makefile') }}
      - name: 빌드
        if: steps.cache.outputs.cache-hit != 'true'
        run: make
      - run: ls -la build
```

**두 번** 실행합니다 (소스 변경 없이).

### 눈으로 확인
- 1회차: `cache-hit`이 false → 빌드 수행 → 저장
- 2회차: `cache-hit`이 true → 빌드 step **건너뜀** → 캐시에서 복원

### 함정 (중요)
- 캐시는 **브랜치 스코프**입니다. `main`에서 만든 캐시는 모든 브랜치가 읽지만,
  feature 브랜치 캐시는 그 브랜치와 그 PR만 읽습니다.
- 그래서 **main에서 캐시를 채우는 워크플로**가 없으면 PR 빌드가 매번 캐시 미스가 납니다.
- 실무에선 표준 도구는 `setup-*` 액션의 내장 캐시(예: `cache: gradle`)를 먼저 고려합니다.

---

## 실무 대응
- 표준 워크플로 전용 레포(예: `your-org/ci-standards`)를 만들고 `uses: org/repo/...@v1` 로 참조.
- 액션·재사용 워크플로는 **커밋 SHA로 고정** + Dependabot으로 자동 갱신.
- 표준 setup 액션이 없는 특수 툴체인은 `actions/cache`로 직접 설계합니다.

## 체크리스트
- [ ] composite action으로 step을 묶고 출력값을 받았다
- [ ] reusable workflow를 매트릭스처럼 두 번 호출했다
- [ ] cache-hit false→true 를 두 번 실행으로 확인했다


## 🔧 도전 과제 — 문서를 찾아 직접 구성하기

기본 실습을 마쳤으면 아래를 **답 코드 없이** 해봅니다. 이 랩에서 배운 것에 아래 **📖 공식 문서**(또는 검색)를 조금 더하면 풀 수 있는 실무형 과제입니다.
난이도: ★ 5분, ★★ 10분, ★★★ 15분. 다 못 해도 됩니다. 시간이 남는 사람이 하는 과제입니다.

### 도전 1 ★★ composite action에 입력 하나 더
**요구사항**: `build-app` 액션에 **`run_tests`(기본 true)** 입력을 추가합니다. `false` 로 넘기면 `make test` step을 건너뛰고 빌드만 합니다.
**완료 조건**: 워크플로에서 `run_tests: false` 로 호출하면 테스트 step이 로그에 없음(또는 회색). 생략하면 테스트가 돎.
**힌트**: composite 안의 step에도 `if:` 를 쓸 수 있습니다. 단 입력값은 **문자열**로 들어온다는 점(`'true'` 와 비교)이 문서에 있습니다.

### 도전 2 ★★★ 재사용 워크플로에 시크릿 넘기기
**요구사항**: `reusable-build.yml` 에 "빌드 후 외부 시스템에 알림" step을 추가합니다. 실제 전송 대신 `curl https://httpbin.org/post -H "Authorization: Bearer $TOK"` 로 흉내 냅니다. 토큰은 **호출하는 쪽**에서 넘겨야 합니다 (Lab 4의 `DEMO_TOKEN`).
**완료 조건**: 호출 워크플로에서 시크릿을 넘기지 않으면 `$TOK` 가 비고, 넘기면 httpbin 응답에 `Bearer ***` 가 보임.
**힌트**: `workflow_call` 에는 `inputs` 말고 `secrets` 절이 따로 있고, 호출 쪽에도 대응하는 키가 있습니다. 전부 넘기는 한 줄짜리 방법도 있는데, 어느 쪽이 "최소 권한"에 맞을지 생각해보세요.

### 도전 3 ★★ 캐시 키가 바뀌는 순간 보기
**요구사항**: 5-C 캐시가 히트하는 상태에서 (1) `src/logic.c` 를 한 줄 바꿔 push → 미스 (2) `README.md` 만 바꿔 push → 히트, 를 순서대로 확인합니다. 그다음 캐시 키에 **러너 OS** 를 넣어, 나중에 Windows 러너를 추가해도 캐시가 섞이지 않게 합니다.
**완료 조건**: Actions → Caches 페이지에서 키 이름에 OS가 들어간 캐시가 보임.
**힌트**: `hashFiles()` 가 무엇을 보는지, `runner.os` 컨텍스트, 그리고 `restore-keys` 가 왜 있는지.

## 📖 공식 문서

- [재사용 워크플로(workflow_call)](https://docs.github.com/en/actions/reference/workflows-and-actions/reusable-workflows)
- [커스텀 액션 메타데이터(composite)](https://docs.github.com/en/actions/reference/workflows-and-actions/metadata-syntax)
- [의존성 캐시(actions/cache)](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching)

<!-- NAV -->

---

[← Lab 4 · 외부 연동과 보안](lab4-secrets-security.html)  ·  [🏠 랩 목록](../)  ·  [Lab 6 · 온프레미스 러너와 마이그레이션 →](lab6-runners-migration.html)

<!-- {% endraw %} -->
