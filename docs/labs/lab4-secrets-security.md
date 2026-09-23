# Lab 4 — 외부 연동과 보안 (4교시)

[🏠 랩 목록으로](../)


## 목표
시크릿 마스킹의 **한계**, 외부 API 호출, GITHUB_TOKEN 권한 최소화를 직접 확인합니다.

---

## 🧹 시작 전 정리 — Lab 3 파이프라인이 매번 따라 도는 문제

Lab 3의 `pipeline.yml`은 `on: push` 라서, 지금부터 워크플로 파일을 하나 push할 때마다
**Lab3 파이프라인도 같이 실행되고 `production` 승인 대기에 걸립니다.** (Actions 탭이 노란 점으로 쌓임)

이 랩에서는 **커밋 메시지에 `[skip ci]` 를 붙여서** push 트리거를 건너뜁니다.
각 해보기 섹션의 커밋 명령에 이미 `[skip ci]`가 포함되어 있습니다.

### 눈으로 확인
`[skip ci]`가 붙은 커밋을 push하면 Actions 탭에 **아무 실행도 안 생깁니다**.
그 다음 `Lab4 보안`을 **Run workflow** 로 직접 돌립니다.

> `[skip ci]`는 `push`/`pull_request` 트리거만 막고, `workflow_dispatch` 수동 실행에는 영향이 없습니다.

### 왜
커밋 메시지에 `[skip ci]`, `[ci skip]`, `[no ci]`, `[skip actions]`, `[actions skip]` 중 하나가 있으면
GitHub이 그 push에 반응하는 **`push`, `pull_request` 트리거의 워크플로 전부**를 실행하지 않습니다.
워크플로를 골라서 막는 게 아니라 "이 커밋은 CI 돌리지 마"라는 **커밋 단위 표시**입니다.
문서 오타 수정, README 편집처럼 빌드가 필요 없는 커밋에 실무에서도 씁니다.

주의:
- 한 번에 커밋 여러 개를 push하면 **마지막 커밋 메시지만** 봅니다.
- 소스가 바뀐 커밋엔 붙이지 않습니다 (테스트를 건너뛰는 셈).

> 다른 방법: Actions 탭 → `Lab3 파이프라인` → 우측 **…** → **Disable workflow** (Lab 2에서 본 기능).
> 이미 승인 대기에 걸린 실행은 열어서 **Cancel workflow** 로 정리합니다.

### 실무 대응
실제 파이프라인은 Lab 2 CI처럼 `paths:` 로 **소스가 바뀐 push에만** 돌게 좁힙니다.
펌웨어 소스(`src/**`, `include/**`, `Makefile`)가 아닌 문서, 워크플로 정리 커밋에 빌드 VM을 쓰지 않게 됩니다.

---

## 4-A. 시크릿 마스킹은 안전장치지 통제가 아니다

### 준비
레포 **Settings → Secrets and variables → Actions → New repository secret**
- 이름 `DEMO_TOKEN`, 값 `super-secret-123`

### 해보기
`.github/workflows/security.yml` 을 새로 만듭니다.

!!! tip "파일 만들고 올리기 (`[skip ci]` 필수)"
    - **웹 UI**: **Add file** → **Create new file** → `.github/workflows/security.yml` → 커밋 메시지에 `[skip ci]` 포함 → **Commit changes**
    - **CLI**: `git add .github/workflows/security.yml && git commit -m "lab4-A: 시크릿 마스킹 실습 [skip ci]" && git push`

    올린 뒤 **Run workflow** 로 수동 실행합니다.

```yaml
name: Lab4 보안
on:
  workflow_dispatch:

jobs:
  masking:
    runs-on: ubuntu-latest
    steps:
      - name: 시크릿을 일부러 출력
        env:
          TOK: ${{ secrets.DEMO_TOKEN }}
        run: |
          echo "그대로 출력: $TOK"          # → *** 로 가려짐
          echo "변형 출력:"
          echo "$TOK" | base64               # → 값이 그대로 새어나옴
```

### 눈으로 확인
- `그대로 출력` 줄: 값이 `***`로 마스킹됨
- `base64` 줄: **마스킹을 뚫고 값이 노출됨**

### 왜
공식 문서: 자동 마스킹은 **보장되지 않습니다.** base64·부분 출력·JSON은 마스킹을 뚫습니다.
그래서 JSON을 통째로 시크릿에 넣지 말라고 합니다.
워크플로가 직접 만든 토큰은 `echo "::add-mask::$값"` 으로 직접 등록해야 합니다.

---

## 4-B. 외부 API 호출 (사내 시스템 연동 대응)

### 해보기
`security.yml`에 `call_api` job을 추가합니다.

!!! tip "수정 후 커밋하고 올리기"
    - **웹 UI**: `security.yml` ✏️ → `call_api` job 추가 → **Commit changes**
    - **CLI**: `git add .github/workflows/security.yml && git commit -m "lab4-B: 외부 API 호출 job 추가 [skip ci]" && git push`

공개 테스트 API(httpbin)로 "외부 시스템에 인증해서 전송"을 흉내 냅니다.

```yaml
  call_api:
    runs-on: ubuntu-latest
    steps:
      - name: 외부 시스템 호출
        env:
          TOK: ${{ secrets.DEMO_TOKEN }}
        run: |
          curl -sS -X POST https://httpbin.org/post \
            -H "Authorization: Bearer $TOK" \
            -d "build=v0.${{ github.run_number }}" | head -20
```

### 눈으로 확인
httpbin이 우리가 보낸 헤더/바디를 그대로 돌려줌 → 인증 헤더로 외부 시스템에 데이터를 보내는 패턴 확인.

### 실무 대응
`httpbin.org` 자리에 **사내 시스템 API**(배포·형상관리·아티팩트 저장소 등)가 들어가고, `DEMO_TOKEN` 자리에 사내 발급 토큰이 들어갑니다. Jenkins 마지막 stage의 전송과 같은 일입니다.

---

## 4-C. GITHUB_TOKEN 권한 최소화 — "명시하면 나머지는 none"

### 해보기
`security.yml`에 `perm_fail` job을 추가합니다.

!!! tip "수정 후 커밋하고 올리기"
    - **웹 UI**: `security.yml` ✏️ → `perm_fail` job 추가 → **Commit changes**
    - **CLI**: `git add .github/workflows/security.yml && git commit -m "lab4-C: 권한 최소화 실습 [skip ci]" && git push`
    - `issues: write` 추가 후: `git add .github/workflows/security.yml && git commit -m "lab4-C: issues:write 추가 [skip ci]" && git push`

권한을 **읽기 전용**으로 낮춘 job에서 쓰기 작업(라벨 생성)을 시도합니다.

```yaml
  perm_fail:
    runs-on: ubuntu-latest
    permissions:
      contents: read           # 이것만 명시 → 나머지는 전부 none
    steps:
      - env:
          GH_TOKEN: ${{ github.token }}
        run: |
          if gh label create lab4-test --repo ${{ github.repository }} --force; then
            echo "✅ 라벨 생성 성공"
          else
            echo "❌ 권한 부족으로 실패"
          fi
```

그다음, `permissions`에 `issues: write`를 추가하고 다시 실행합니다.

```yaml
    permissions:
      contents: read
      issues: write            # 라벨 생성에 필요
```

### 눈으로 확인
- 처음: `HTTP 403: Resource not accessible by integration` 뒤에 **`❌ 권한 부족으로 실패`**
- `issues: write` 추가 후: **`✅ 라벨 생성 성공`** → 레포 **Issues → Labels** 에 `lab4-test`가 생김

> `gh`는 CI 안에서는 성공 메시지를 안 찍습니다. 그래서 `if`로 성공/실패를 직접 출력하게 했습니다.
> `--force`는 이미 있는 라벨이면 덮어써서, 재실행 때 "already exists"로 헷갈리지 않게 합니다.

결과 줄은 step의 머리 부분(`Run gh label create …`, `shell:`, `env:`) **아래**에 나옵니다. 안 보이면 step 제목을 클릭해 펼치세요.

토큰이 실제로 받은 권한은 로그 맨 위 **`Set up job` → `GITHUB_TOKEN Permissions`** 를 펼치면 보입니다.

```
처음                          issues: write 추가 후
  Contents: read                Contents: read
  Metadata: read                Issues: write
                                Metadata: read
```
(`Metadata: read`는 항상 자동으로 붙습니다)

### 왜
`permissions:` 는 워크플로가 자동으로 받는 임시 출입증(`GITHUB_TOKEN`)에 **어떤 권한을 줄지** 적는 곳입니다.
"이 워크플로가 실행되는 동안 GitHub에 무엇을 할 수 있나"를 정하는 것입니다. 권한은 레포의 **영역(scope)** 단위로 나뉘고,
각각 `read` / `write` / `none` 중 하나입니다.

| 영역 | 이 권한으로 할 수 있는 것 |
|---|---|
| `contents` | 코드 읽기(checkout) / 쓰기(커밋, 태그, Release, Lab 3-E) |
| `issues` | 이슈, **라벨**, 마일스톤 |
| `pull-requests` | PR 코멘트, 리뷰 |
| `packages` | 패키지(컨테이너 이미지 등) 올리기 |
| `id-token` | OIDC 토큰 발급 (클라우드 인증) |
| `actions` | 워크플로 실행 취소, 캐시 삭제 |

라벨인데 `issues`인 이유: GitHub API에서 라벨은 이슈의 부속 기능이라 `labels` 영역이 따로 없습니다.

권한을 **하나라도 명시하면 명시하지 않은 나머지는 모두 none**이 됩니다.
그래서 `id-token: write`만 적었더니 `checkout`이 실패하는 일이 생깁니다 (contents: read가 사라져서).
권장: 기본을 read-only로 두고, 필요한 job에서만 올린다.

---

## 4-D. 산출물을 외부 저장소에 올리기 — Azure Blob

Lab 3의 아티팩트는 GitHub **안**에만 남습니다(보관 기간 지나면 삭제).
실무에서는 빌드 결과를 **Artifactory 같은 외부 저장소**로 보내 오래 보관합니다.
여기서는 Azure Blob Storage를 "외부 저장소" 역할로 써서 같은 패턴을 해봅니다.
(AWS S3, GCS, Artifactory도 "CLI + 토큰 시크릿" 구조는 똑같습니다)

### 흐름 그림

```
  build job (러너 A)                  publish job (러너 B)                 Azure Blob Storage
  ┌──────────────────┐  artifact     ┌──────────────────────┐   SAS 토큰    ┌──────────────────┐
  │ checkout → make  │ ──────────▶   │ download-artifact    │ ──────────▶   │ firmware/        │
  │ upload-artifact  │  (GitHub 안)  │ az storage blob      │  HTTPS 443    │   v0.42/app      │
  └──────────────────┘               │   upload-batch       │               │   v0.43/app  …   │
                                     └──────────────────────┘               └──────────────────┘
                                          ▲ secrets.AZ_SAS
```

### 준비 0 — Azure 로그인

**[portal.azure.com](https://portal.azure.com)** 에 교육용 계정으로 로그인합니다.

| 항목 | 값 |
|------|-----|
| 사용자 이름 | `user01@nrkim0615outlook.onmicrosoft.com` ~ `user04@nrkim0615outlook.onmicrosoft.com` |
| 배정 | 강사가 알려주는 번호 하나를 사용 (예: 2번이면 `user02@…`) |
| 비밀번호 | 강사가 현장에서 전달 |
| 리소스 그룹 | 본인 계정에 맞는 것 하나만 보입니다 (예: `user02-rg`) — 이후 단계에서 그 그룹을 선택 |

!!! warning "계정을 섞어 쓰지 마세요"
    각 계정은 **자기 리소스 그룹에만** 권한이 있습니다. 다른 번호의 그룹을 고르면 "권한이 없습니다"(AuthorizationFailed) 로 실패합니다.
    새 리소스 그룹을 만들 권한도 없으니, **이미 있는 본인 그룹**을 그대로 씁니다.

??? tip "Cloud Shell / CLI 로 할 경우"
    포털 오른쪽 위 **`>_` (Cloud Shell)** 를 열면 이미 로그인된 상태입니다(설치 불필요). 본인 PC 의 `az` 를 쓰려면 `az login` 먼저.

    ```bash
    az account show --query '{user:user.name, subscription:name}' -o table
    az group list --query '[].name' -o tsv      # 본인 리소스 그룹 이름 확인
    ```

### 준비 1 — 저장소 만들기 (Azure Portal)

**[portal.azure.com](https://portal.azure.com)** 에서 진행합니다. (준비 0 에서 로그인한 상태)

1. 상단 검색창 → **스토리지 계정** → **만들기**
2. 기본 탭 입력:

    | 항목 | 값 |
    |------|-----|
    | 리소스 그룹 | 본인 계정의 리소스 그룹 (예: `user02-rg`) |
    | 스토리지 계정 이름 | `stlab` + 영숫자 6자 (전 세계 유일해야 함) ← **메모** |
    | 지역 | Korea Central |
    | 중복성 | LRS |

3. **검토 + 만들기** → **만들기** → 배포 완료 후 **리소스로 이동**

4. 왼쪽 메뉴 **데이터 스토리지 → 컨테이너** → **+ 컨테이너**
    - 이름: `firmware`
    - 익명 액세스 수준: 프라이빗(기본값 유지)
    - **만들기**

??? tip "Cloud Shell 로 할 경우"
    ```bash
    RG=user02-rg          # 본인 계정 번호에 맞게
    ACCT=stlab$(openssl rand -hex 3)
    echo "저장소 계정 이름: $ACCT  ← 메모"

    az storage account create -g $RG -n $ACCT -l koreacentral --sku Standard_LRS --kind StorageV2 --allow-blob-public-access false
    KEY=$(az storage account keys list -g $RG -n $ACCT --query '[0].value' -o tsv)
    az storage container create -n firmware --account-name $ACCT --account-key "$KEY"
    ```

### 준비 2 — 기간 한정 토큰(SAS) 발급 (Azure Portal)

SAS는 "이 컨테이너에, 이 권한으로, 이 날짜까지"만 허용하는 **기간 한정 출입증**입니다.
계정 키(마스터 키)를 워크플로에 넣지 않기 위해 씁니다.

1. 준비 1에서 만든 스토리지 계정 → **데이터 스토리지 → 컨테이너** → `firmware` 클릭
2. 왼쪽 메뉴 **공유 액세스 토큰** 클릭
3. 권한 체크: **읽기 ✅ 추가 ✅ 만들기 ✅ 쓰기 ✅ 나열 ✅** (삭제는 체크 해제)
4. 만료 날짜: 오늘로부터 **30일 후**로 설정
5. **SAS 토큰 및 URL 생성** 클릭
6. 아래 나타나는 **SAS 토큰** (`sp=…` 로 시작하는 긴 문자열) → **복사해서 메모**

??? tip "Cloud Shell 로 할 경우"
    ```bash
    END=$(date -u -d '+30 days' '+%Y-%m-%dT%H:%MZ')   # Cloud Shell(Linux). macOS 터미널이면: date -u -v+30d '+%Y-%m-%dT%H:%MZ'
    SAS=$(az storage container generate-sas -n firmware --account-name $ACCT --account-key "$KEY" \
            --permissions racwl --expiry $END -o tsv)
    echo "$SAS"
    ```

### 준비 3 — 실습 레포에 시크릿과 변수 등록 (GitHub Settings)

실습 레포 → **Settings → Secrets and variables → Actions**

1. **Secrets 탭** → **New repository secret**
    - 이름: `AZ_SAS`
    - 값: 준비 2에서 복사한 SAS 토큰 (`sp=…` 로 시작하는 긴 문자열)
    - **Add secret**

2. **Variables 탭** → **New repository variable**
    - 이름: `AZ_STORAGE_ACCOUNT`
    - 값: 준비 1에서 메모한 스토리지 계정 이름 (`stlab…`)
    - **Add variable**

> 왜 둘을 나누나: 저장소 **이름**은 비밀이 아니니 `vars`(로그에 보임), **토큰**은 `secrets`(마스킹). 비밀이 아닌 설정값까지 시크릿에 넣으면 로그에서 안 보여 디버깅이 힘들어집니다.

??? tip "CLI로 등록할 경우 (gh 로그인 상태)"
    ```bash
    gh secret set AZ_SAS --body "<SAS 토큰>"
    gh variable set AZ_STORAGE_ACCOUNT --body "<스토리지 계정 이름>"
    ```

### 해보기
`.github/workflows/publish.yml` 을 새로 만듭니다.

!!! tip "파일 만들고 올리기"
    - **웹 UI**: **Add file** → **Create new file** → `.github/workflows/publish.yml` → **Commit changes**
    - **CLI**: `git add .github/workflows/publish.yml && git commit -m "lab4-D: Azure Blob 업로드 [skip ci]" && git push`

    > `[skip ci]` 를 커밋 메시지에 넣으면 `push` 트리거가 자동 실행되지 않습니다. 올린 뒤 **Run workflow** 로 수동 실행합니다.

```yaml
name: Lab4 외부 저장소 업로드
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - run: make
      - uses: actions/upload-artifact@v7
        with:
          name: app
          path: build/app

  publish:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v8
        with:
          name: app
          path: dist
      - name: Azure Blob에 업로드
        env:
          ACCT: ${{ vars.AZ_STORAGE_ACCOUNT }}
          SAS: ${{ secrets.AZ_SAS }}
          VERSION: v0.${{ github.run_number }}
        run: |
          az storage blob upload-batch \
            --account-name "$ACCT" --sas-token "$SAS" \
            --destination firmware --destination-path "$VERSION" \
            --source dist
      - name: 올라간 파일 목록을 Summary에
        env:
          ACCT: ${{ vars.AZ_STORAGE_ACCOUNT }}
          SAS: ${{ secrets.AZ_SAS }}
        run: |
          echo "## firmware 컨테이너" >> "$GITHUB_STEP_SUMMARY"
          az storage blob list --container-name firmware \
            --account-name "$ACCT" --sas-token "$SAS" \
            --query '[].name' -o tsv | sort >> "$GITHUB_STEP_SUMMARY"
```

### 한 줄씩 뜻풀이

| 줄 | 뜻 |
|---|---|
| `needs: build` | build가 끝난 뒤, **다른 새 머신**에서 publish 시작 (Lab 3-C) |
| `download-artifact` | GitHub에 올린 `app`을 이 머신의 `dist/` 로 내려받음 |
| `vars.AZ_STORAGE_ACCOUNT` | 레포 **변수** (비밀 아님, 로그에 그대로 보임) |
| `secrets.AZ_SAS` | 레포 **시크릿** (로그에 `***`) |
| `az storage blob upload-batch` | 폴더째 업로드. `az` CLI는 GitHub 호스티드 러너에 **미리 설치**돼 있음 |
| `--destination-path "$VERSION"` | 컨테이너 안에 `v0.42/` 같은 실행 번호 폴더를 만들어 버전별로 쌓음 |
| `$GITHUB_STEP_SUMMARY` | 실행 화면 Summary에 파일 목록 표시 (Lab 3-C와 같은 기법) |

### 눈으로 확인
- 실행 Summary에 `v0.<번호>/app` 이 보임
- 한 번 더 실행하면 `v0.<번호+1>/app` 이 **추가**됨 (GitHub 아티팩트와 달리 지워지지 않고 쌓임)
- 로그의 `SAS: ***` — 토큰은 가려지고, `ACCT: stlab…` 은 그대로 보임
- 본인 터미널에서도 확인:

```bash
az storage blob list -c firmware --account-name $ACCT --sas-token "$SAS" --query '[].name' -o tsv
```

### 🤔 생각해보기
- 30일이 지나 SAS가 만료되면 이 워크플로는 어떻게 될까요? 실무에서 이 만료를 어떻게 관리해야 할까요?
- `publish` job을 Lab 6-A의 self-hosted 러너에서 돌리려면 무엇이 더 필요할까요? (힌트: "미리 설치돼 있음")

### 왜
- 이 job이 하는 일은 **"CLI 하나 + 토큰 하나"** 입니다. 4-B의 `curl` 과 구조가 같고, Jenkins 마지막 stage의 "Artifactory 업로드"와 같은 일입니다.
- SAS 같은 **기간 한정, 권한 한정 토큰**을 쓰면 유출돼도 피해 범위가 좁습니다. 마스터 키를 시크릿에 넣지 않는 이유입니다.
- 더 나아가면 토큰 자체를 없앨 수 있습니다 — **OIDC**: 워크플로가 GitHub이 발급한 신원 토큰으로 클라우드에 직접 로그인 (`azure/login`, `aws-actions/configure-aws-credentials`). 저장할 시크릿이 0개가 됩니다.

### 실무 대응
- 사내에서는 Azure Blob 자리에 **Artifactory** 가 들어갑니다: `jfrog/setup-jfrog-cli` + `jf rt upload`. 저장소는 그대로 두고 CI만 바꿉니다.
- 토큰은 **environment 시크릿**(Lab 3-D)에 두면 승인 후에만 접근됩니다 → "승인 → 업로드" 게이트가 자연스럽게 생깁니다.
- 폐쇄망 GHES라면 러너에서 저장소까지 **아웃바운드 443** 하나만 열면 됩니다.

### 정리 (비용)
실습이 끝나면 저장소를 지웁니다.

```bash
az storage account delete -g $RG -n $ACCT --yes
```

---

## 4-E. 다른 레포의 소스 가져오기 — GITHUB_TOKEN 의 경계

실무에서 흔한 상황입니다. **공통 스크립트나 설정이 다른 레포에 있고**, 빌드할 때 그걸 같이 가져와야 합니다.
4-C 에서 본 `GITHUB_TOKEN` 은 **이 레포 전용**이라 다른 레포에는 쓸 수 없습니다. 직접 실패시켜 보고 해결합니다.

### 준비 — 두 번째 레포 만들기

1. GitHub 오른쪽 위 **+ → New repository**
    - 이름: `shared-scripts`
    - **Private** 선택 (핵심입니다. Public 이면 토큰 없이도 받아져서 실습이 성립하지 않습니다)
    - **Add a README file** 체크 → **Create repository**
2. 만들어진 레포에서 **Add file → Create new file**
    - 파일 이름: `scripts/version.sh`
    - 내용:

    ```bash
    #!/usr/bin/env bash
    echo "공통 스크립트 v1.0.0 (shared-scripts 레포에서 옴)"
    ```

    - **Commit changes**

### 해보기 1 — 그냥 가져와 보기 (실패합니다)

실습 레포에 `.github/workflows/multi-repo.yml` 을 만듭니다.

!!! tip "파일 만들고 올리기"
    - **웹 UI**: **Add file** → **Create new file** → `.github/workflows/multi-repo.yml` → **Commit changes**
    - **CLI**: `git add .github/workflows/multi-repo.yml && git commit -m "lab4-E: 다른 레포 checkout [skip ci]" && git push`

```yaml
name: Lab4 다른 레포 가져오기
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: 내 레포 체크아웃
        uses: actions/checkout@v7

      - name: 공통 스크립트 레포 체크아웃
        uses: actions/checkout@v7
        with:
          repository: <내-계정-또는-조직>/shared-scripts   # ← 본인 것으로
          path: shared                                     # ← 이 폴더로 받음

      - name: 가져온 스크립트 실행
        run: bash shared/scripts/version.sh
```

**Run workflow** 로 실행합니다.

#### 눈으로 확인 — 실패 메시지

```
remote: Repository not found.
fatal: repository 'https://github.com/<계정>/shared-scripts/' not found
Error: The process '/usr/bin/git' failed with exit code 128
```

레포는 분명히 있는데 **"not found"** 라고 나옵니다.
권한이 없으면 GitHub 은 "없다"고 답합니다 — 레포의 존재 여부까지 숨기기 위해서입니다.

#### 왜 실패하나

`actions/checkout` 은 기본으로 `GITHUB_TOKEN` 을 씁니다. 그 토큰은 **이 워크플로가 도는 레포 하나**에만 쓸 수 있습니다.
`permissions: contents: read` 를 아무리 올려도 **다른 레포에는 적용되지 않습니다.** 범위 자체가 다릅니다.

### 해보기 2 — 토큰을 따로 줘서 성공시키기

#### 2-1. 토큰 발급 (fine-grained PAT)

본인 GitHub 계정 → 오른쪽 위 프로필 → **Settings**(레포 설정 아님, **계정** 설정) →
왼쪽 맨 아래 **Developer settings** → **Personal access tokens → Fine-grained tokens** → **Generate new token**

| 항목 | 값 |
|------|-----|
| Token name | `lab4-shared-scripts` |
| Expiration | 7 days (실습용) |
| Resource owner | 레포 소유자 (본인 계정 또는 조직) |
| Repository access | **Only select repositories** → `shared-scripts` 하나만 |
| Permissions → Repository permissions → **Contents** | **Read-only** |

**Generate token** → 표시된 토큰(`github_pat_…`)을 **복사**합니다. 이 화면을 벗어나면 다시 못 봅니다.

??? warning "조직 레포라서 토큰 발급이 막히면"
    조직(Organization) 소유 레포는 fine-grained PAT 사용을 조직이 허용해야 하고, 승인 대기가 걸릴 수 있습니다.
    실습 중이라면 두 가지 우회가 있습니다.

    - **두 번째 레포를 개인 계정에 만들기** — 가장 간단합니다.
    - **classic PAT 쓰기** — Developer settings → Tokens (classic) → Generate → 스코프 `repo` 체크.
      단 classic PAT 은 **내 모든 레포에 대한 넓은 권한**이라 실무에서는 권장되지 않습니다(다음 "왜" 참고).

#### 2-2. 시크릿으로 등록

실습 레포 → **Settings → Secrets and variables → Actions → New repository secret**

- 이름: `SHARED_REPO_TOKEN`
- 값: 방금 복사한 토큰
- **Add secret**

#### 2-3. 워크플로에 `token:` 한 줄 추가

```yaml
      - name: 공통 스크립트 레포 체크아웃
        uses: actions/checkout@v7
        with:
          repository: <내-계정-또는-조직>/shared-scripts
          path: shared
          token: ${{ secrets.SHARED_REPO_TOKEN }}   # ← 이 한 줄
```

### 눈으로 확인
- job 초록불, 로그에 `공통 스크립트 v1.0.0 (shared-scripts 레포에서 옴)`
- 작업 폴더 구조를 찍어보면 두 레포가 나란히 있습니다.

    ```yaml
          - run: ls -la && echo "---" && ls -la shared
    ```

    ```
    .            ← 내 레포 (Makefile, src, tests …)
    shared/      ← 다른 레포 (scripts/version.sh)
    ```

### 왜 — 세 가지 방법과 선택 기준

| 방법 | 어떻게 | 장점 | 단점 |
|------|--------|------|------|
| **PAT** (이번 실습) | 사람이 발급 → 시크릿에 저장 → `token:` | 설정이 가장 쉬움 | **사람에 묶임** — 발급자가 퇴사/권한 변경되면 파이프라인이 멈춤. 만료 관리 필요 |
| **Deploy key** | 레포마다 SSH 키 한 쌍, 공개키를 대상 레포에 등록 → `ssh-key:` | 레포 1:1, 읽기 전용 가능, 사람과 무관 | 레포 수만큼 키 관리 |
| **GitHub App** | 조직에 앱 설치 → `actions/create-github-app-token` 으로 1시간 토큰 발급 | 사람과 무관, 범위·권한 세분, 단기 토큰 | 초기 설정이 가장 무거움 |

실무 권장 순서는 **GitHub App > Deploy key > PAT** 입니다. 이번 실습이 PAT 인 이유는 5분 안에 되기 때문입니다.

!!! note "4교시 토큰 슬라이드와 연결"
    `GITHUB_TOKEN` 은 **이 레포만 · 실행 끝나면 만료**, PAT 은 **그 사람의 모든 레포 · 길게 유효**.
    그래서 "다른 레포 접근"과 "후속 워크플로 트리거" 두 경우에만 예외적으로 PAT/App 을 씁니다.

### 실무 대응
- 사내에서는 공통 파이프라인 스크립트를 **별도 레포(ci-standards 등)** 에 두는 경우가 많습니다 — 이 실습이 그 구조입니다.
- 다만 **워크플로 자체를 공유**하는 게 목적이라면 checkout 이 아니라 **reusable workflow**(Lab 5-B)가 정답입니다. checkout 은 "파일을 가져오는" 방법입니다.
- 토큰은 **만료일을 캘린더에 적어두거나**, GitHub App 으로 옮겨 만료 관리를 없앱니다.
- GHES 폐쇄망도 동일합니다. 다른 점은 URL 이 사내 GHES 주소라는 것뿐입니다.

### 🤔 생각해보기
- `token:` 을 빼면 왜 "not found" 이지 "권한 없음(403)" 이 아닐까요?
- 이 토큰이 유출되면 무엇을 할 수 있을까요? fine-grained 로 `shared-scripts` 만, Contents Read-only 로 좁힌 이유가 여기 있습니다.
- 공통 스크립트가 아니라 **공통 워크플로**를 나눠 쓰고 싶다면 어떤 방법이 맞을까요? (Lab 5-B)

---

## 실무 대응 (보안팀 관점 정리)
- 시크릿은 **환경변수 경유**로 쓰고, 환경 시크릿은 **승인 후에만** 접근 (Lab 3-D).
- 외부 연동은 `curl`/CLI + 시크릿(또는 OIDC)으로.
- GITHUB_TOKEN은 **기본 read-only**, 필요한 job만 상향.
- 서드파티 액션은 **커밋 SHA로 고정** + 조직 허용 목록 (Lab 5·6에서 이어짐).

## 체크리스트
- [ ] 마스킹이 base64에서 뚫리는 것을 봤다
- [ ] 외부 API에 인증 헤더로 전송해봤다
- [ ] 권한 부족으로 실패 → 권한 추가 후 성공을 봤다
- [ ] 산출물을 외부 저장소(Azure Blob)에 버전별로 올려봤다
- [ ] 다른 레포 checkout 이 GITHUB_TOKEN 으로는 안 되는 것을 보고, 토큰을 따로 줘서 성공시켰다


## 🔧 도전 과제

> 기본 실습을 마쳤으면 아래를 **답 코드 없이** 해봅니다.
> 이 랩에서 배운 것에 **📖 공식 문서**(또는 검색)를 조금 더하면 풀 수 있는 실무형 과제입니다.
>
> ★ 5분 · ★★ 10분 · ★★★ 15분 — 다 못 해도 됩니다. 시간이 남는 분이 하는 과제입니다.

---

### 도전 1 · ★ — 워크플로가 직접 만든 값 가리기

step에서 동적으로 생성한 값을 **다음 step에서도 마스킹**되게 만듭니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `TMP=$(openssl rand -hex 8)` 으로 임시 토큰 생성 후, 다음 step에서 `echo` 해도 로그에 `***` 로 표시 |
| **완료 조건** | 로그에 값 대신 `***` — 마스킹 명령 빼면 값이 그대로 보이는 것과 비교 |

??? tip "힌트"
    - 4-A "왜" 절에 답이 한 줄 있습니다
    - 값을 다음 step으로 넘기는 건 `$GITHUB_ENV`

---

### 도전 2 · ★★ — 승인 후에만 보이는 시크릿

환경(environment) 시크릿은 **승인 전에는 접근 자체가 불가**한 것을 눈으로 확인합니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | `production` 환경에 **환경 시크릿** `DEPLOY_KEY` 생성, 환경 없는 job과 `environment: production` job 양쪽에서 출력 비교 |
| **완료 조건** | 환경 없는 job → 빈 문자열 / production job → 승인 대기 → 승인 후 `***` |

??? warning "주의"
    같은 이름의 **레포 시크릿은 만들지 마세요** — 만들면 비교가 안 됩니다.

??? tip "힌트"
    - Settings → Environments → production 안에 시크릿 칸이 따로 있습니다
    - "승인 전에는 시크릿에 접근조차 못 한다"를 눈으로 확인하는 과제입니다

---

### 도전 3 · ★★ — 권한 없이 Release 시도하기

`permissions:` 블록을 지웠을 때 **403 실패**를 직접 만들고, 다시 넣어 성공시킵니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | ① 레포 설정에서 `GITHUB_TOKEN` 기본 권한 확인 ② Lab 3-E `deploy` job의 `permissions:` 제거 → 403 실패 ③ 다시 추가 → 성공, 두 실행의 로그 `GITHUB_TOKEN Permissions` 비교 |
| **완료 조건** | permissions 없이 → `HTTP 403: Resource not accessible by integration` / 있으면 성공, 로그 권한 목록이 `Contents: read` ↔ `Contents: write` 로 다름 |

??? tip "힌트"
    - Settings → Actions → General → 맨 아래 **Workflow permissions** (위쪽 "Actions permissions"와 다른 설정)
    - 필요한 권한 영역은 4-C 표에서 확인
    - 오래된 조직 레포는 기본이 "Read and write"일 수 있음 — 그 경우 read-only로 바꾸는 것까지가 실무 권장

## 📖 공식 문서

- [시크릿 사용(조직/리포/환경)](https://docs.github.com/en/actions/concepts/security/secrets)
- [보안 강화(마스킹, 스크립트 인젝션, SHA 고정)](https://docs.github.com/en/actions/reference/security/secure-use)
- [워크플로 실행 건너뛰기 (`[skip ci]`)](https://docs.github.com/en/actions/managing-workflow-runs-and-deployments/managing-workflow-runs/skipping-workflow-runs)
- [변수(vars) 사용](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-variables)
- [OIDC로 Azure 인증(시크릿 없이)](https://docs.github.com/en/actions/how-tos/secure-your-work/security-harden-deployments/oidc-in-azure)
- [GITHUB_TOKEN 권한(permissions) 문법](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)

<!-- NAV -->

---

[← Lab 3 · 파이프라인 설계와 산출물](lab3-pipeline-artifacts.md)  ·  [🏠 랩 목록](../)  ·  [Lab 5 · 표준화와 재사용 →](lab5-reuse-standardize.md)

