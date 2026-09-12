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
    - uses: actions/checkout@v5
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
이것이 Jenkins **shared library** 자리에 들어가는 물건입니다.
shared library 하나가 보통 **reusable workflow(파이프라인 전체)** 와 **composite action(step 묶음)** 둘로 쪼개집니다.

---

## 5-C. cache — 반복 작업 시간 줄이기

이 예제는 빌드가 워낙 빨라 캐시 효과가 작지만, **동작과 함정**을 확인합니다.

### 해보기
빌드 산출물 디렉터리를 캐시해 봅니다. (실무에선 툴체인/의존성을 캐시)

```yaml
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v5
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


## 📖 공식 문서

- [재사용 워크플로(workflow_call)](https://docs.github.com/en/actions/reference/workflows-and-actions/reusable-workflows)
- [커스텀 액션 메타데이터(composite)](https://docs.github.com/en/actions/reference/workflows-and-actions/metadata-syntax)
- [의존성 캐시(actions/cache)](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching)

<!-- NAV -->

---

[← Lab 4 · 외부 연동과 보안](lab4-secrets-security.html)  ·  [🏠 랩 목록](../)  ·  [Lab 6 · 온프레미스 러너와 마이그레이션 →](lab6-runners-migration.html)

<!-- {% endraw %} -->
