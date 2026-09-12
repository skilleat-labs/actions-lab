# Lab 6 — 온프레미스 러너와 마이그레이션 (6교시)

## 목표
self-hosted 러너의 동작을 (선택적으로) 직접 등록해 확인하고, **Actions Importer**로
Jenkins 파이프라인 변환을 직접 돌려 **무엇이 자동 변환되고 무엇이 안 되는지** 눈으로 봅니다.

> 이 랩은 일부가 실제 인프라(러너 머신, Jenkins, GHES)를 필요로 합니다.
> 각 절 머리에 **[무료 가능]** / **[머신 필요]** / **[읽기]** 로 표시합니다.

---

## 6-A. self-hosted 러너 등록  [머신 필요 · 선택]

본인 노트북/VM 하나를 러너로 붙여 "러너는 아웃바운드만 쓴다"를 확인합니다.

### 해보기
1. 레포 **Settings → Actions → Runners → New self-hosted runner**
2. 화면에 나오는 명령을 그대로 복붙 (다운로드 → `./config.sh ...` → `./run.sh`)
3. 워크플로에서 `runs-on`을 바꿔 실행:

```yaml
jobs:
  on_prem:
    runs-on: self-hosted
    steps:
      - run: |
          echo "여기는 내 머신에서 돈다"
          hostname
```

### 눈으로 확인
- 내 머신의 hostname이 로그에 찍힘
- 러너 등록·실행 내내 **들어오는 포트를 열지 않았다** — 러너가 GitHub으로 **나가는** 연결만 씀
- 끝나면 `Ctrl+C`로 러너를 내리고, Settings에서 러너를 제거

### 왜
Jenkins는 controller ↔ agent 양방향이라 방화벽 협의가 복잡했습니다.
Actions 러너는 **아웃바운드 HTTPS 443**만 필요해 협의가 단순합니다.

### 실무 대응
- label(`self-hosted,linux,ghs-toolchain`)과 **runner group**으로 라우팅·라이선스 통제.
- 대규모는 **ARC**(Kubernetes 위 러너) — Helm 차트 두 개, Pod 하나 = Job 하나.

---

## 6-B. Actions Importer로 Jenkins 변환  [무료 가능]

Docker와 gh CLI만 있으면 로컬에서 돌릴 수 있습니다.

### 준비
```bash
# gh CLI 설치되어 있어야 함 (https://cli.github.com)
gh extension install github/gh-actions-importer
gh actions-importer version
```

### 해보기 — 샘플 Jenkinsfile 변환(dry-run)
실제 Jenkins 서버 없이도, 로컬 Jenkinsfile을 변환해볼 수 있습니다.
간단한 Declarative Jenkinsfile을 하나 만듭니다. (예: `sample/Jenkinsfile`)

```groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps { sh 'make' }
    }
    stage('Test') {
      steps { sh 'make test' }
    }
  }
  post {
    always { echo 'done' }
  }
}
```

변환:
```bash
gh actions-importer dry-run jenkins --source-file-path sample/Jenkinsfile --output-dir out/
cat out/*.yml
```

### 눈으로 확인
- `stage('Build')` → `jobs.build`, `sh 'make'` → `run: make` 로 **자동 변환**됨
- 변환된 YAML을 읽어보면 구조가 그대로 옮겨진 것을 확인

### 안 바뀌는 것 (솔직히)
공식 문서가 밝힌 **자동 변환 안 되는 것**:
- Scripted pipeline (Groovy 스크립트)
- 플러그인(unknown), shared library
- 시크릿, self-hosted 러너 설정
- 파라미터, 조건 로직 일부

Groovy로 `if/for/함수`를 쓴 부분은 변환기가 손대지 못하고 **수동 재작성**이 필요합니다.
그 이유는 Lab(2교시)에서 본 것처럼 **YAML에는 로직이 없기 때문**입니다.

### 실무 대응
- 실제 도입 시엔 `audit`로 전체 Jenkins 현황을 먼저 분석 → 변환 가능 비율 파악.
- "80%는 자동, 20%는 손"이 정직한 기대치입니다.

---

## 6-C. 폐쇄망에서 공식 액션 가져오기 (actions-sync)  [읽기]

github.com 개인 실습에서는 필요 없지만, 실무(폐쇄망 GHES)에서 반드시 만납니다.

- GHES는 github.com Marketplace에 직접 접근하지 못합니다.
- `actions/checkout` **조차** 미리 동기화해야 `uses:`가 동작합니다.
- 인터넷 머신에서 `actions-sync pull` → 반입 → GHES에 `actions-sync push`.
- 액션 버전을 올릴 때마다 반복 → **운영 절차**로 만들어야 합니다.
- 제한적 아웃바운드가 허용되면 **GitHub Connect**가 더 편한 대안입니다.

---

## 실무에서 만나는 사용량 제한 (참고)
| 항목 | 값 |
|---|---|
| Job 시간 (self-hosted) | 5일 |
| 큐 대기 | 24시간 초과 시 자동 취소 |
| 매트릭스 job | 256개 / 실행 |
| 캐시 | 10GB · 미사용 7일 삭제 |
| 워크플로 파일 | 500 KB |

## 체크리스트
- [ ] (선택) 내 머신을 러너로 붙여 hostname을 봤다
- [ ] Actions Importer로 Jenkinsfile을 변환해봤다
- [ ] Groovy 로직이 자동 변환 안 되는 것을 확인했다
- [ ] actions-sync가 왜 필요한지 이해했다
