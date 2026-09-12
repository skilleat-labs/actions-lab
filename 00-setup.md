# Lab 00 — 실습 레포 만들기

## 1. 새 레포 생성

1. github.com 로그인 → 우상단 **+** → **New repository**
2. 이름: 예) `actions-lab-<본인이름>` · **Private**로 만들어도 됩니다
3. **Add a README file** 체크 → **Create repository**

> 실무 참고: 소스가 사내에 머물러야 하는 조직은 GHES를 씁니다.
> 학습은 github.com 개인 레포가 가장 빠르고, 배우는 문법·구조는 100% 동일합니다.

## 2. 로컬에 clone (선택)

웹에서 파일을 직접 만들어도 되지만, 로컬이 편하면:

```bash
git clone https://github.com/<본인계정>/actions-lab-<본인이름>.git
cd actions-lab-<본인이름>
```

## 3. Actions 활성화 확인

- 레포 상단 **Actions** 탭 클릭 → 사용 가능한 상태인지 확인
- (조직 레포라면 Settings → Actions → General에서 Allow가 켜져 있어야 합니다)

## 4. 워크플로 파일 위치 규칙

- 워크플로는 반드시 **`.github/workflows/`** 폴더 안의 `.yml` 파일이어야 합니다
- 이 경로가 아니면 GitHub이 인식하지 못합니다

## ⚠️ 코드 복사할 때 주의 (중요)

랩의 YAML을 옮길 때는 **문서(GitHub 레포 또는 Pages 사이트)의 코드 블록**에서 복사하세요.
코드 블록 오른쪽 위의 **복사 버튼(📋)** 을 쓰면 정확하게 복사됩니다.

- ❌ **강의 화면(채팅/발표 슬라이드)에서 눈으로 보고 따라 치지 마세요.**
  `${{ ... }}` 같은 표현식이 화면에서 `$` 로 짧게 보여서, 그대로 옮기면 깨집니다.
- ✅ 예: `name: app-${{ matrix.build_type }}` 를 `name: app-$` 로 잘못 옮기면
  값이 자동으로 안 채워지고, 산출물 이름이 글자 그대로 `app-$` 가 됩니다.

> `${{ }}` 는 GitHub이 실행 직전에 실제 값으로 바꿔주는 자리표시자입니다. **통째로** 들어가야 동작합니다.

> 액션 버전(`@v7` 등)은 시간이 지나면 올라갈 수 있습니다. 실행 로그에 "Node deprecated" 같은
> **경고(warning)** 가 떠도 동작에는 지장 없지만, 깔끔하게 쓰려면 각 액션의 **Releases 페이지**에서
> 최신 메이저 버전을 확인해 맞추면 됩니다. (예: upload-artifact, download-artifact는 버전 번호가 서로 다를 수 있음)

## 준비 완료

이제 [Lab 1](labs/lab1-first-workflow.html)로 갑니다.

각 랩은 이 표기를 씁니다:
- **해보기** — 직접 만들고 실행하는 단계
- **눈으로 확인** — Actions 탭에서 무엇이 보여야 하는지
- **왜** — 그 동작의 원리 (Jenkins와의 차이)
- **실무 대응** — 실제 사내 환경에서는 이 자리에 무엇이 들어가는가

<!-- NAV -->

---

[🏠 랩 목록](./)  ·  [Lab 1 · 첫 워크플로와 러너 관찰 →](labs/lab1-first-workflow.html)
