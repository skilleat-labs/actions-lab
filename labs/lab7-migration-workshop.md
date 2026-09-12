# Lab 7 — 전환 워크숍 (7교시)

[🏠 랩 목록으로](../)


## 목표
지금까지 만든 것을 **우리 실제 환경**에 대입합니다. 이 랩은 실행이 아니라 **작성(워크시트)**입니다.

---

## 7-A. 우리 파이프라인을 대입하기

아래 표의 오른쪽 칸을 **본인 조직 기준**으로 채웁니다.

| 우리가 지금 쓰는 것 | GitHub Actions 대응 | 우리 경우 (직접 작성) |
|---|---|---|
| 기존 CI 빌드 VM (Jenkins 등) | self-hosted 러너 (기존 VM 재활용) | |
| Jenkinsfile (Declarative) | 워크플로 .yml | |
| stage | job (needs로 순서) | |
| shared library | reusable workflow + composite action | |
| credentials() | secrets / OIDC | |
| Manual Approval | environment + Required reviewers | |
| archiveArtifacts | upload-artifact | |
| 아티팩트 저장소 업로드 | (그대로 유지 — CI만 교체) | |
| 배포·형상 시스템 전송 | 마지막 job의 API 호출 | |
| 기존 소스 저장소 (예: Bitbucket, GitLab) | GHES로 이관 (**별도 과제**) | |

---

## 7-B. 도입 장애물 — 세 축으로 정리

각 축에 **우리 조직의 구체적 장애물**을 적습니다.

### 보안
- 소스 반출 제한 → GHES 필요:
- 프록시(HTTPS 프록시 여부):
- 협력사 계정 / 포크 PR:
- 시크릿·SHA 고정 정책:

### 인프라
- GHES 사양(8 vCPU / 64 GB) + 외부 스토리지:
- 러너 = 기존 빌드 VM 재활용 가능 여부:
- 특수 툴체인 라이선스 서버 러너 도달성:
- actions-sync 운영 담당:

### 조직
- 기존 소스 저장소 → GHES 이관 계획(별도 과제):
- 현재 Jenkins 플러그인 목록 조사:
- 파일럿 팀·레포 선정:
- Jenkins 병행 운영 기간:

---

## 7-C. 전환 로드맵 — 한 번에 하나씩

우리 상황에 맞춰 각 단계의 **담당/기한**을 적습니다.

1. **평가** — 플러그인 대응 조사 · GHES 사양 산정 →
2. **PoC** — 파일럿 레포 1개, 러너 1~2대 →
3. **표준화** — 검증된 파이프라인을 reusable workflow로 →
4. **확산** — 팀별 순차, Jenkins 병행, 산출물 해시 동일성 검증 →
5. **정리** — Jenkins 읽기 전용 후 폐기 →

> 원칙: **CI 전환과 소스 저장소 이관을 동시에 하지 않는다.**
> 문제가 생기면 원인을 특정할 수 없습니다.

---

## 7-D. 지금 바로 할 일 / 더 확인할 일

### 지금 바로
- [ ] 현재 CI(Jenkins 등) 플러그인 목록 뽑기
- [ ] 파일럿 레포 하나 선정
- [ ] GHES 사양을 인프라팀에 전달

### 더 확인
- [ ] 사내 IdP·시스템의 OIDC(JWT) 지원 여부
- [ ] 사내 프록시가 HTTPS 프록시인지 (GHES는 HTTP 프록시만 지원)
- [ ] 라이선스 서버에 러너가 접근 가능한지

---

## 마무리
기술적으로는 대부분 가능합니다. 다만 **GHES + self-hosted가 전제**이고, 이는 인프라 프로젝트입니다.
한 번에 전부 옮기지 않고 **파일럿 하나부터** 시작합니다.

여기까지가 실습입니다. Lab 1~6에서 손으로 만든 것들이, 위 표의 왼쪽을 오른쪽으로 옮기는 실제 작업이 됩니다.


## 📖 공식 문서

- [Jenkins에서 이전(Actions Importer)](https://docs.github.com/en/actions/migrating-to-github-actions/automated-migrations/migrating-from-jenkins-with-github-actions-importer)
- [재사용 워크플로로 표준화](https://docs.github.com/en/actions/reference/workflows-and-actions/reusable-workflows)
- [엔터프라이즈(GHES)에서 Actions](https://docs.github.com/en/enterprise-server@latest/admin/github-actions/getting-started-with-github-actions-for-your-enterprise/getting-started-with-github-actions-for-github-enterprise-server)

<!-- NAV -->

---

[← Lab 6 · 온프레미스 러너와 마이그레이션](lab6-runners-migration.html)  ·  [🏠 랩 목록](../)
