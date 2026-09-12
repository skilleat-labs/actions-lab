# Lab 1 — 첫 워크플로와 러너 관찰 (1교시)

## 목표
러너가 **매번 새 머신**이라는 것과, **job은 서로 격리**된다는 것을 눈으로 확인합니다.

---

## 1-A. 첫 워크플로 만들기

### 해보기
레포에 `.github/workflows/hello.yml` 파일을 만들고 아래를 넣습니다.

```yaml
name: Lab1 첫 워크플로
on:
  workflow_dispatch:        # Actions 탭에서 수동 실행

jobs:
  inspect:
    runs-on: ubuntu-latest
    steps:
      - name: checkout 전에 작업 폴더 보기
        run: ls -la

      - name: 소스 가져오기
        uses: actions/checkout@v5

      - name: checkout 후에 작업 폴더 보기
        run: ls -la
```

커밋/푸시한 뒤, **Actions 탭 → Lab1 첫 워크플로 → Run workflow** 를 눌러 실행합니다.

### 눈으로 확인
`inspect` job 로그를 펼쳐서 두 개의 `ls -la`를 비교합니다.
- **checkout 전**: 작업 폴더가 거의 비어 있음
- **checkout 후**: 소스 파일들이 나타남

### 왜
러너는 job이 시작될 때마다 **깨끗한 새 환경**입니다.
Jenkins는 SCM 체크아웃이 암묵적이었지만, Actions는 `actions/checkout`을 **명시**하지 않으면 소스가 없습니다.

---

## 1-B. job은 서로 격리된다

### 해보기
`hello.yml`의 `jobs:` 아래에 job 두 개를 추가합니다. (기존 `inspect`는 그대로 두거나 지워도 됩니다.)

```yaml
jobs:
  job_a:
    runs-on: ubuntu-latest
    steps:
      - run: echo "hello from A" > /tmp/probe.txt
      - run: cat /tmp/probe.txt        # 같은 job → 읽힘

  job_b:
    runs-on: ubuntu-latest
    steps:
      - name: 다른 job이 만든 파일 읽어보기
        run: |
          if [ -f /tmp/probe.txt ]; then
            echo "읽힘 — 같은 러너";
          else
            echo "없음 — job이 다르면 러너도 다르다";
          fi
```

다시 **Run workflow**.

### 눈으로 확인
- `job_a`: 두 번째 step에서 파일이 **읽힘**
- `job_b`: 같은 경로 파일이 **없음**

### 왜
**Job = 러너 1대 = 파일시스템 1대.** job이 다르면 파일이 공유되지 않습니다.
그래서 job 사이로 파일을 넘기려면 아티팩트가 필요합니다 (Lab 3).

---

## 1-C. 순서 만들기 (needs)

### 해보기
`job_b`에 `needs: job_a`를 추가하고 다시 실행합니다.

```yaml
  job_b:
    needs: job_a
    runs-on: ubuntu-latest
    steps:
      - run: echo "job_a가 끝난 뒤에 시작"
```

### 눈으로 확인
`needs`가 없을 때는 두 job이 **동시에** 시작했지만, `needs`를 걸면 `job_a` 성공 후에만 `job_b`가 시작합니다. Actions 탭의 job 그래프로 순서가 보입니다.

### 왜
Actions의 job은 **병렬이 기본**입니다. Jenkins의 stage(순차 기본)와 정반대입니다.
Jenkinsfile의 stage를 job으로 1:1 옮기면 순서가 깨지는 이유가 이것입니다.

---

## 실무 대응
- `ubuntu-latest` → 실무에서는 **self-hosted 러너**(기존 빌드 VM)로 바뀝니다 (Lab 6).
- 러너가 매번 새 환경이므로, 툴체인 설치 시간을 줄이려면 **cache**가 필요합니다 (Lab 5).

## 체크리스트
- [ ] checkout 전/후 `ls -la` 차이를 봤다
- [ ] job_a가 만든 파일을 job_b가 못 읽는 것을 봤다
- [ ] needs로 순서가 바뀌는 것을 봤다
