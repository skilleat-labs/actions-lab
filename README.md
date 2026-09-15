<!-- {% raw %} -->
# GitHub Actions 실습 랩 (actions-lab)

GitHub Actions **실습 전용** 가이드입니다.
시연을 보는 대신, **각자 자기 레포를 하나 만들어** 1교시부터 7교시까지
워크플로를 직접 만들고 눈으로 결과를 확인합니다.

> 대상: 기존 CI/CD(Jenkins 등) 경험자 · GitHub Actions 처음
> 환경: **github.com 개인 레포 + 무료 러너**로 전부 실행됩니다.
> 실무 환경은 보통 GHES + self-hosted 러너지만, 학습은 github.com이 가장 빠릅니다.
> 각 랩 끝의 **실무 대응** 절에서 "우리 환경에서는 이 자리에 무엇이 들어가는가"를 짚습니다.

## 랩 목록

| 랩 | 교시 | 무엇을 눈으로 확인하나 |
|---|---|---|
| [Lab 1](labs/lab1-first-workflow.html) | 1교시 | 러너는 매번 새 머신 — checkout 전엔 소스도 없다 / job은 서로 격리 |
| [Lab 2](labs/lab2-structure-ui.html) | 2교시 | 필터로 안 도는 것 vs 실패한 것 / 빨간불·로그·재실행 |
| [Lab 3](labs/lab3-pipeline-artifacts.html) | 3교시 | needs·매트릭스·아티팩트·승인 게이트·Release |
| [Lab 4](labs/lab4-secrets-security.html) | 4교시 | 시크릿 마스킹의 한계 / 외부 API 호출 / 권한 최소화 / (선택) 외부 저장소 업로드 |
| [Lab 5](labs/lab5-reuse-standardize.html) | 5교시 | reusable workflow·composite action·cache |
| [Lab 6](labs/lab6-runners-migration.html) | 6교시 | self-hosted 러너(선택, 클라우드 VM 포함)·Actions Importer로 Jenkins 변환·종합 실습(VM 러너→승인→Blob) |
| [Lab 7](labs/lab7-migration-workshop.html) | 7교시 | 우리 파이프라인을 Actions로 — 전환 워크시트 |

## 시작하기

먼저 [00-setup.md](00-setup.html)를 보고 **자기 실습 레포**를 하나 만드세요.
그다음 Lab 1부터 순서대로 진행합니다. 각 랩은 앞 랩 위에 쌓입니다.

## 폴더 구조

```
actions-lab/
├─ README.md              이 파일
├─ 00-setup.md            실습 레포 만들기
├─ labs/                  랩 1~7
└─ starter/               랩 3부터 쓰는 예제 소스 (자기 레포로 복사)
   ├─ Makefile
   ├─ include/logic.h
   ├─ src/{main.c,logic.c}
   └─ tests/test_logic.c
```

<!-- {% endraw %} -->
