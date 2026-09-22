# 강사용 도전 과제 정답표

> 이 파일은 `docs/` 밖에 있어 학생용 사이트에 배포되지 않습니다.

---

## 빠른 참조 요약

| Lab | 도전 | 핵심 키워드 |
|-----|------|-------------|
| Lab1-1 | 러너 정보 리포트 | `$GITHUB_STEP_SUMMARY`, `nproc`, `df -h` |
| Lab1-2 | Windows 러너 비교 | `runs-on: windows-latest`, `shell: pwsh` |
| Lab1-3 | cleanup 항상 실행 | `if: always()` |
| Lab2-1 | 문서 변경 CI 제외 | `paths:` + `!` 패턴 or `paths-ignore:` |
| Lab2-2 | 야간 빌드 | `schedule: cron: '0 17 * * *'`, `github.event_name == 'schedule'` |
| Lab2-3 | 체크박스 입력 | `type: boolean`, `if: inputs.verbose == true` |
| Lab3-1 | 매트릭스 exclude | `strategy.matrix.exclude` |
| Lab3-2 | 실패해도 로그 업로드 | `tee`, `if: always()`, `continue-on-error: true` |
| Lab3-3 | 빌드 시간 제한 | `timeout-minutes: 3` |
| Lab3-4 | 릴리스 노트 자동 생성 | `--generate-notes`, `--notes`, `cut -c1-7` |
| Lab4-1 | 동적 값 마스킹 | `echo "::add-mask::$값"` |
| Lab4-2 | 환경 시크릿 접근 | `environment: production`, 승인 전 빈 문자열 |
| Lab4-3 | 권한 없이 Release 실패 | `permissions: contents: write` |
| Lab5-1 | composite 입력 추가 | `if: inputs.run_tests == 'true'` (문자열 비교) |
| Lab5-2 | 재사용 워크플로 시크릿 | `workflow_call.secrets:`, `secrets: inherit` vs 명시 |
| Lab5-3 | 캐시 키 OS 추가 | `${{ runner.os }}`, `restore-keys` |
| Lab6-1 | 라벨 라우팅 | `runs-on: [self-hosted, toolchain-a]` (AND 조건) |
| Lab6-2 | Jenkinsfile 수동 변환 | `if: github.ref`, `environment:`, `if: always()` |
| Lab6-3 | VM 자동 종료 | Azure 자동 종료 or `schedule` + `az vm deallocate` |

---

## Lab 1 — 첫 워크플로

### 도전 1 ★ — 러너 정보 리포트

```yaml
jobs:
  inspect:
    runs-on: ubuntu-latest
    steps:
      - name: 러너 정보 Summary 출력
        run: |
          CORES=$(nproc)
          DISK=$(df -h / | awk 'NR==2 {print $4}')
          echo "| 항목 | 값 |" >> "$GITHUB_STEP_SUMMARY"
          echo "|------|-----|" >> "$GITHUB_STEP_SUMMARY"
          echo "| OS | ${{ runner.os }} |" >> "$GITHUB_STEP_SUMMARY"
          echo "| CPU 코어 수 | $CORES |" >> "$GITHUB_STEP_SUMMARY"
          echo "| 디스크 여유 공간 | $DISK |" >> "$GITHUB_STEP_SUMMARY"
```

**포인트:** `$GITHUB_STEP_SUMMARY`에 `>>` 로 Markdown을 이어 붙이면 Summary 화면에 렌더링됨.

---

### 도전 2 ★★ — Windows 러너에서도 돌려보기

```yaml
jobs:
  inspect-linux:
    runs-on: ubuntu-latest
    steps:
      - run: ls -la
      - uses: actions/checkout@v7
      - run: ls -la

  inspect-windows:
    runs-on: windows-latest
    defaults:
      run:
        shell: bash          # Git Bash 사용 → ls -la 그대로 쓸 수 있음
    steps:
      - run: ls -la
      - uses: actions/checkout@v7
      - run: |
          ls -la
          echo "작업 폴더: $GITHUB_WORKSPACE"
```

**포인트:** Windows 기본 셸은 `pwsh`. `shell: bash`를 명시하면 Git Bash가 사용되어 Linux 명령어 그대로 동작.

---

### 도전 3 ★★★ — 앞 job이 실패해도 마지막 job은 돌게

```yaml
jobs:
  first:
    runs-on: ubuntu-latest
    steps:
      - run: echo "first job 실행"

  second:
    needs: first
    runs-on: ubuntu-latest
    steps:
      - run: exit 1          # 일부러 실패

  cleanup:
    needs: second
    runs-on: ubuntu-latest
    if: always()             # ← 이게 핵심. second 성공/실패 무관하게 항상 실행
    steps:
      - run: echo "cleanup 항상 실행됨"
```

**포인트:** `if: always()` vs `if: failure()` 차이 — `always()`는 성공/실패/취소 모두, `failure()`는 실패할 때만.

---

## Lab 2 — 워크플로 구조와 UI

### 도전 1 ★ — 문서만 고치면 CI가 안 돌게

```yaml
on:
  push:
    paths:
      - 'src/**'             # src/만 허용 → docs/, *.md는 자동으로 트리거 안 됨
  workflow_dispatch:
```

또는 반대 방향:

```yaml
on:
  push:
    paths-ignore:
      - '**/*.md'
      - 'docs/**'
  workflow_dispatch:
```

**포인트:** `paths:`와 `paths-ignore:`는 같은 트리거에 동시에 쓸 수 없음. `paths:` 안에서 `!` 패턴으로 제외 처리 가능.

---

### 도전 2 ★★ — 야간 빌드 (KST 02:00)

```yaml
on:
  push:
    paths:
      - 'src/**'
  workflow_dispatch:
  schedule:
    - cron: '0 17 * * *'   # UTC 17:00 = KST 02:00 (KST = UTC+9)

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - run: echo "빌드 실행됨"

      - name: 야간 빌드 전용 메시지
        if: github.event_name == 'schedule'
        run: echo "야간 빌드입니다"
```

**포인트:** `schedule` cron은 **UTC 기준**. KST 02:00 = UTC 17:00. `schedule` 트리거는 기본 브랜치에만 동작.

---

### 도전 3 ★★ — 수동 실행에 체크박스 입력 추가

```yaml
on:
  workflow_dispatch:
    inputs:
      verbose:
        description: 환경변수 전체 출력 (디버깅용)
        type: boolean
        default: false

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - run: echo "빌드 실행됨"

      - name: 환경변수 전체 출력
        if: inputs.verbose == true
        run: env
```

**포인트:** `type: boolean` → Run workflow 창에 체크박스로 표시. `push` 트리거에서는 `inputs.verbose`가 null → step 건너뜀(safe).

---

## Lab 3 — 파이프라인 설계와 산출물

### 도전 1 ★★ — 매트릭스에 축 하나 더 + exclude

```yaml
strategy:
  fail-fast: false
  matrix:
    build_type: [Debug, Release]
    target: [MCU, AP]
    exclude:
      - build_type: Debug
        target: AP           # Debug × AP 조합만 제외 → 3개 job 실행
```

아티팩트 이름에 두 축 모두 포함:
```yaml
      - uses: actions/upload-artifact@v7
        with:
          name: app-${{ matrix.build_type }}-${{ matrix.target }}
          path: build/app
```

---

### 도전 2 ★★ — 테스트가 실패해도 로그는 남기기

```yaml
    steps:
      - uses: actions/checkout@v7

      - name: 테스트 (로그 저장)
        run: make test 2>&1 | tee test.log
        continue-on-error: true      # 실패해도 다음 step 진행

      - name: 테스트 로그 업로드
        if: always()                 # 성공/실패 무관 항상 업로드
        uses: actions/upload-artifact@v7
        with:
          name: test-log-${{ matrix.build_type }}
          path: test.log

      - run: make BUILD_TYPE=${{ matrix.build_type }}
```

**포인트:** `tee test.log` = 화면 + 파일 동시 저장. `continue-on-error: true` + `if: always()` 콤보.

---

### 도전 3 ★ — 빌드 시간 제한

```yaml
  build:
    runs-on: ubuntu-latest
    timeout-minutes: 3       # 3분 초과 시 job 자동 취소
```

**포인트:** 기본값 360분(6시간). step 레벨에도 동일하게 쓸 수 있음.

---

### 도전 4 ★★★ — 릴리스 노트 자동 생성

```yaml
      - name: Release 생성
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          SHA=$(echo "${{ github.sha }}" | cut -c1-7)
          gh release create "v0.${{ github.run_number }}" \
            --repo ${{ github.repository }} \
            --title "빌드 v0.${{ github.run_number }}" \
            --notes "빌드 정보: Release · 커밋 $SHA" \
            --generate-notes \
            dist/*
```

**포인트:** `--notes`(직접 작성) + `--generate-notes`(커밋 목록 자동) 동시 사용 가능. `cut -c1-7` = SHA 앞 7자리.

---

## Lab 4 — 외부 연동과 보안

### 도전 1 ★ — 워크플로가 직접 만든 값 가리기

```yaml
    steps:
      - name: 임시 토큰 생성 및 마스킹
        run: |
          TMP=$(openssl rand -hex 8)
          echo "::add-mask::$TMP"        # ← 핵심: 이후 모든 로그에서 마스킹
          echo "TMP_TOKEN=$TMP" >> "$GITHUB_ENV"

      - name: 다음 step에서 출력
        run: echo "값: $TMP_TOKEN"       # → *** 로 가려짐
```

**포인트:** `echo "::add-mask::$값"` 은 workflow command. 마스킹 등록과 환경변수 등록을 **같은 step**에서 해야 함.

---

### 도전 2 ★★ — 승인 후에만 보이는 시크릿

**준비:** Settings → Environments → `production` → Secrets에 `DEPLOY_KEY` 추가 (레포 시크릿에는 같은 이름 만들지 말 것)

```yaml
  no-env:
    runs-on: ubuntu-latest
    # environment 없음 → 환경 시크릿 접근 불가 → 빈 문자열
    steps:
      - run: |
          [ -z "$KEY" ] && echo "빈 문자열" || echo "값 있음: $KEY"
        env:
          KEY: ${{ secrets.DEPLOY_KEY }}

  with-env:
    runs-on: ubuntu-latest
    environment: production   # 승인 후 환경 시크릿 접근 가능
    steps:
      - run: |
          [ -z "$KEY" ] && echo "빈 문자열" || echo "값 있음: $KEY"
        env:
          KEY: ${{ secrets.DEPLOY_KEY }}
```

**포인트:** `environment:` job은 승인 전 **러너 자체가 시작되지 않음** → 시크릿에 접근조차 불가.

---

### 도전 3 ★★ — 권한 없이 Release 시도하기

> 먼저 Settings → Actions → General → Workflow permissions → **Read repository contents** (read-only) 확인

**실패:** `permissions:` 블록 제거 → `HTTP 403: Resource not accessible by integration`

**성공:**
```yaml
  deploy:
    permissions:
      contents: write          # Release 생성에 필요
```

**포인트:** 권한 하나라도 명시하면 나머지 모두 none. `Set up job` 로그 → `GITHUB_TOKEN Permissions`에서 실제 권한 확인.

---

## Lab 5 — 표준화와 재사용

### 도전 1 ★★ — composite action에 run_tests 스위치

`.github/actions/build-app/action.yml`:

```yaml
inputs:
  run_tests:
    description: 테스트 실행 여부
    required: false
    default: 'true'            # 입력값은 문자열 — 따옴표 필수

runs:
  using: composite
  steps:
    - uses: actions/checkout@v7
    - name: 테스트
      if: inputs.run_tests == 'true'   # 문자열 'true'와 비교 (boolean true 아님)
      shell: bash
      run: make test
    - shell: bash
      run: make BUILD_TYPE=${{ inputs.build_type }}
```

호출:
```yaml
        uses: ./.github/actions/build-app
        with:
          run_tests: 'false'   # 테스트 건너뜀
```

**포인트:** composite action 입력값은 항상 **문자열**. `== 'true'` (따옴표 있음), `== true` (따옴표 없음)는 다름.

---

### 도전 2 ★★★ — 재사용 워크플로에 시크릿 넘기기

`reusable-build.yml`에 secrets 절 추가:
```yaml
on:
  workflow_call:
    inputs:
      build_type:
        type: string
        required: true
    secrets:
      notify_token:
        required: false

jobs:
  build:
    ...
    steps:
      - name: 외부 알림
        if: secrets.notify_token != ''
        env:
          TOK: ${{ secrets.notify_token }}
        run: curl -sS https://httpbin.org/post -H "Authorization: Bearer $TOK"
```

`caller.yml`에서 명시적 전달 (최소 권한):
```yaml
  release-build:
    uses: ./.github/workflows/reusable-build.yml
    with:
      build_type: Release
    secrets:
      notify_token: ${{ secrets.DEMO_TOKEN }}
```

전부 넘기는 방법 (간단하지만 최소 권한 아님):
```yaml
    secrets: inherit
```

**포인트:** 명시적 전달이 "최소 권한 원칙"에 맞음. `secrets: inherit`은 불필요한 시크릿까지 노출.

---

### 도전 3 ★★ — 캐시 키에 OS 추가

```yaml
      - uses: actions/cache@v4
        id: cache
        with:
          path: build
          key: build-${{ runner.os }}-${{ hashFiles('src/**', 'Makefile') }}
          restore-keys: |
            build-${{ runner.os }}-
```

**포인트:**
- `src/logic.c` 변경 → `hashFiles` 값 변경 → 캐시 미스
- `README.md` 변경 → `hashFiles`가 src/만 봄 → 캐시 히트
- `${{ runner.os }}` 키 포함 → ubuntu/windows/macOS 간 캐시 충돌 방지
- Actions 탭 → **Caches** 메뉴에서 키 이름 확인

---

## Lab 6 — 온프레미스 러너와 마이그레이션

### 도전 1 ★★ — 라벨로 라우팅

**준비:** Settings → Actions → Runners → 러너 클릭 → 라벨 편집 → `toolchain-a` 추가

```yaml
jobs:
  on-toolchain-a:
    runs-on: [self-hosted, toolchain-a]   # AND 조건 — 두 라벨 모두 가진 러너
    steps:
      - run: hostname

  on-toolchain-b:
    runs-on: [self-hosted, toolchain-b]   # toolchain-b 없음 → 큐 대기(노란색)
    steps:
      - run: echo "실행 안 됨"
```

**포인트:** 배열 `runs-on`은 **모든 라벨 AND** 조건. 없는 라벨이면 24시간 후 자동 취소.

---

### 도전 2 ★★★ — Jenkinsfile 수동 변환

원본 Jenkinsfile의 세 가지 미변환 패턴:

| Jenkinsfile | GitHub Actions 변환 |
|---|---|
| `when { branch 'main' }` | `if: github.ref == 'refs/heads/main'` |
| `input message: '배포할까요?'` | `environment: production` (Settings에서 Required reviewers 설정) |
| `post { always { } }` | `if: always()` + 별도 job |

```yaml
  deploy:
    needs: test
    if: github.ref == 'refs/heads/main'
    environment: production
    steps:
      - uses: actions/checkout@v7
      - run: make deploy

  cleanup:
    needs: [build, test, deploy]
    runs-on: ubuntu-latest
    if: always()
    steps:
      - run: echo "done"
```

---

### 도전 3 ★ — 러너 정리 자동화

**방법 A (추천): Azure Portal 자동 종료**

VM → 자동 종료 → 20:00 KST 설정. Actions 워크플로 불필요.

**방법 B: GitHub Actions schedule**

```yaml
on:
  schedule:
    - cron: '0 11 * * *'   # UTC 11:00 = KST 20:00

jobs:
  shutdown:
    runs-on: ubuntu-latest   # self-hosted가 꺼져있을 수 있으므로 호스팅 러너 사용
    steps:
      - uses: azure/login@v2
        with:
          client-id: ${{ secrets.AZURE_CLIENT_ID }}
          tenant-id: ${{ secrets.AZURE_TENANT_ID }}
          subscription-id: ${{ secrets.AZURE_SUBSCRIPTION_ID }}
      - run: az vm deallocate -g ${{ vars.RUNNER_RG }} -n runner-01
```

---

## Lab 7 — 마이그레이션 워크시트

Lab 7은 코드 정답이 없는 **조직 상황 기입형** 과제입니다. 아래는 작성 예시입니다.

| Jenkins | GitHub Actions |
|---------|---------------|
| 빌드 VM | self-hosted 러너 (기존 서버에 러너 앱만 설치) |
| Jenkinsfile | `.github/workflows/pipeline.yml` |
| stage | job (`needs`로 순서 지정) |
| shared library | reusable workflow + composite action |
| credentials() | secrets / OIDC |
| Manual Approval | `environment` + Required reviewers |
| archiveArtifacts | `actions/upload-artifact` |
| Artifactory 업로드 | 마지막 job에서 `jf rt upload` 그대로 |

**전환 순서:** 평가 → PoC(파일럿 1개) → 표준화(reusable workflow) → 확산 → Jenkins 폐기

> **핵심 원칙:** CI 전환과 소스 저장소 이관을 동시에 하지 않는다. 문제 발생 시 원인 특정 불가.
