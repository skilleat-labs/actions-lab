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
      - uses: actions/checkout@v5
      - run: echo "빌드 실행됨 (src가 바뀜)"
```

### 이 트리거가 무슨 뜻일까 (한 줄씩)

1교시에서는 `workflow_dispatch`(수동)만 배웠죠. 여기서는 트리거가 **두 개**입니다.

| 키 | 뜻 |
|---|---|
| `on:` | 트리거는 **여러 개**를 나열할 수 있습니다. 여기서는 `push` 와 `workflow_dispatch` 둘 다. |
| `push:` | 코드를 **push할 때마다 자동 실행**. (Jenkins의 SCM 트리거에 해당) |
| `paths:` | push 중에서도 **여기 지정한 파일이 바뀐 경우에만** 실행. |
| `- 'src/**'` | `src/` 폴더 아래(하위 폴더 포함)가 바뀌면 실행. `README.md`만 바뀌면 안 돎. |
| `workflow_dispatch:` | 1교시에서 배운 **수동 실행 버튼**. |

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
- **Re-run failed jobs**: 실패한 job만 다시 돎 (성공한 job은 그대로)
- 워크플로 파일을 고쳐서 다시 돌리려면 **새 커밋을 push**해야 함
  (재실행은 그 시점의 스냅샷을 다시 돌리는 것이라, 파일 수정은 재실행에 반영되지 않음)

---

## 2-D. workflow_dispatch 입력값

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
고른 값이 로그에 찍힘. Jenkins의 파라미터 빌드에 해당합니다.

---

## 2-E. Actions 탭 둘러보기

한 번씩 눌러보며 어디에 무엇이 있는지 익힙니다.
- **Summary** — job 그래프, 소요 시간, 아티팩트
- **각 step 로그** — 접기/펼치기
- **Workflow file** 탭 — 그 실행에 쓰인 워크플로 원본 ("분명 고쳤는데 옛날대로 돈다"일 때 확인)
- 우측 **…** → **Disable workflow** — 워크플로 일시 정지

---

## 실무 대응
- 필터(`paths`, `branches`)는 여러 모듈을 한 레포에 담을 때 "바뀐 모듈만 빌드"에 그대로 씁니다.
- 실패 지점이 코드 줄에 표시되는 것(Annotations)은 Lab 4의 정적분석 출력과 연결됩니다.

## 체크리스트
- [ ] README만 고치면 워크플로가 안 뜨는 것을 봤다
- [ ] 빨간불과 skipped step을 봤다
- [ ] Re-run failed jobs를 써봤다
- [ ] workflow_dispatch 입력값을 넣어봤다


<!-- NAV -->

---

[← Lab 1 · 첫 워크플로와 러너 관찰](lab1-first-workflow.html)  ·  [🏠 랩 목록](../)  ·  [Lab 3 · 파이프라인 설계와 산출물 →](lab3-pipeline-artifacts.html)
