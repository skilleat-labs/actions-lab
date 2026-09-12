# Lab 00 — 실습 레포 만들기

## 1. 새 레포 생성

1. github.com 로그인 → 우상단 **+** → **New repository**
2. 이름: 예) `actions-lab-<본인이름>` · **Private**로 만들어도 됩니다
3. **Add a README file** 체크 → **Create repository**

> 실무 참고: 현대모비스는 소스가 사내에 머물러야 하므로 GHES를 씁니다.
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

## 준비 완료

이제 [Lab 1](labs/lab1-first-workflow.html)로 갑니다.

각 랩은 이 표기를 씁니다:
- **해보기** — 직접 만들고 실행하는 단계
- **눈으로 확인** — Actions 탭에서 무엇이 보여야 하는지
- **왜** — 그 동작의 원리 (Jenkins와의 차이)
- **실무 대응** — 현대모비스 환경에서는 이 자리에 무엇이 들어가는가
