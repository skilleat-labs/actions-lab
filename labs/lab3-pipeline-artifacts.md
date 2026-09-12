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

> 왜 clone 대신 curl? `git clone`은 README, labs 등 **레포 전체**를 받아옵니다.
> 실습 레포에는 예제 소스만 있으면 되니, 필요한 파일만 `curl`로 받는 게 깔끔합니다.

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
      - uses: actions/checkout@v5
      - name: 테스트
        run: make test
      - name: 빌드
        run: make
      - name: 산출물 업로드
        uses: actions/upload-artifact@v4
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
      - uses: actions/checkout@v5
      - run: make test
      - run: make BUILD_TYPE=${{ matrix.build_type }}
      - uses: actions/upload-artifact@v4
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
        uses: actions/download-artifact@v4
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

## 3-D. environment 승인 게이트

### 준비 (레포 설정)
1. 레포 **Settings → Environments → New environment** → 이름 `production`
2. **Required reviewers** 에 **본인**을 추가 → Save

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
`deploy` job이 **노란색 대기 상태**로 멈춤 → 화면에 **Review deployments** 버튼 → 승인하면 그제서야 `deploy`가 시작.

### 왜
Jenkins의 input과 달리, **승인 전에는 job 자체가 시작되지 않습니다.**
환경 시크릿에 접근조차 못 합니다 (Lab 4와 연결).

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
      - uses: actions/download-artifact@v4
        with:
          pattern: app-*
          path: dist
          merge-multiple: true
      - name: Release 생성
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          gh release create "v0.${{ github.run_number }}" \
            --title "빌드 v0.${{ github.run_number }}" \
            --generate-notes \
            dist/*
```

### 눈으로 확인
승인 후, 레포 **Releases** 에 새 릴리스가 생기고 산출물이 첨부됨. 릴리스 노트는 자동 생성.

### 왜
Jenkins에는 Release라는 1급 개념이 없었습니다. 태그 + 노트 + 첨부가 한 곳에 모입니다.

---

## 실무 대응
- `build/app` → 실무에서는 실제 빌드 산출물(바이너리, 컨테이너 이미지 등).
- `gh release create` → 사내 **아티팩트 저장소 업로드 + 배포·형상 시스템 전송**으로 바뀝니다 (Lab 4).
- environment 승인 → 사내 **Manual Approval / 결재**에 그대로 대응됩니다.

## 체크리스트
- [ ] 아티팩트를 업로드/다운로드했다
- [ ] 매트릭스로 job이 갈라지는 것을 봤다
- [ ] deploy가 승인 대기에서 멈추는 것을 봤다
- [ ] Release가 생성되고 파일이 첨부된 것을 봤다

## 📖 공식 문서

- [needs, strategy.matrix 문법](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)
- [아티팩트 업로드/다운로드](https://github.com/actions/upload-artifact)
- [환경(environment)과 승인 게이트](https://docs.github.com/en/actions/how-tos/deploy/configure-and-manage-deployments/manage-environments)

<!-- NAV -->

---

[← Lab 2 · 워크플로 구조와 UI](lab2-structure-ui.html)  ·  [🏠 랩 목록](../)  ·  [Lab 4 · 외부 연동과 보안 →](lab4-secrets-security.html)
