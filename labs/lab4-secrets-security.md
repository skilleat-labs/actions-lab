<!-- {% raw %} -->
# Lab 4 — 외부 연동과 보안 (4교시)

[🏠 랩 목록으로](../)


## 목표
시크릿 마스킹의 **한계**, 외부 API 호출, GITHUB_TOKEN 권한 최소화를 직접 확인합니다.

---

## 🧹 시작 전 정리 — Lab 3 파이프라인이 매번 따라 도는 문제

Lab 3의 `pipeline.yml`은 `on: push` 라서, 지금부터 워크플로 파일을 하나 push할 때마다
**Lab3 파이프라인도 같이 실행되고 `production` 승인 대기에 걸립니다.** (Actions 탭이 노란 점으로 쌓임)

이 랩에서는 커밋 메시지에 **`[skip ci]`** 를 붙여서 push 트리거를 건너뜁니다.

```bash
git add .github/workflows/security.yml
git commit -m "Lab4 보안 워크플로 [skip ci]"
git push
```

### 눈으로 확인
push했는데 Actions 탭에 **아무 실행도 안 생김**. 그 다음 `Lab4 보안`을 **Run workflow**로 직접 돌립니다.
(`[skip ci]`는 `push`/`pull_request` 트리거만 막고, `workflow_dispatch` 수동 실행에는 영향이 없습니다.)

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
`.github/workflows/security.yml`:

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
Jenkins로 치면 "빌드 실행 계정에 어떤 권한을 줄지"입니다. 권한은 레포의 **영역(scope)** 단위로 나뉘고,
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

## 4-D. pull_request_target 위험과 checkout v7 (읽기 + 관찰)

이 부분은 포크가 필요해 무료 실습으로 재현이 번거롭습니다. **원리만 확인**합니다.

- `pull_request_target`은 **기본 브랜치 컨텍스트**로 돌아 시크릿과 쓰기 토큰을 가집니다.
- 여기서 **PR의 코드를 checkout해서 실행**하면 공격자가 시크릿을 탈취할 수 있습니다 (pwn request).
- **actions/checkout v7**은 `pull_request_target`·`workflow_run`에서 **포크 PR checkout을 기본 차단**합니다.
  뚫으려면 `allow-unsafe-pr-checkout`를 명시해야 하는데, 그 옵션은 켜지 마세요.

> 확인해보기(선택): 본인 레포에 `pull_request_target` 워크플로를 만들고, 다른 계정으로 포크→PR을 올리면
> checkout이 차단되는 것을 볼 수 있습니다. 사내 폐쇄망은 포크 PR이 드물지만 협력사 계정이 있으면 유효합니다.

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
- [ ] pull_request_target / checkout v7 원리를 이해했다


## 📖 공식 문서

- [시크릿 사용(조직/리포/환경)](https://docs.github.com/en/actions/concepts/security/secrets)
- [보안 강화(마스킹, 인젝션, SHA 고정, pull_request_target)](https://docs.github.com/en/actions/reference/security/secure-use)
- [워크플로 실행 건너뛰기 (`[skip ci]`)](https://docs.github.com/en/actions/managing-workflow-runs-and-deployments/managing-workflow-runs/skipping-workflow-runs)
- [GITHUB_TOKEN 권한(permissions) 문법](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)

<!-- NAV -->

---

[← Lab 3 · 파이프라인 설계와 산출물](lab3-pipeline-artifacts.html)  ·  [🏠 랩 목록](../)  ·  [Lab 5 · 표준화와 재사용 →](lab5-reuse-standardize.html)

<!-- {% endraw %} -->
