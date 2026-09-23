# Lab 6 — 온프레미스 러너와 마이그레이션 (6교시)

[🏠 랩 목록으로](../)


## 목표
self-hosted 러너의 동작을 (선택적으로) 직접 등록해 확인하고, **Actions Importer**로
Jenkins 파이프라인 변환을 직접 돌려 **무엇이 자동 변환되고 무엇이 안 되는지** 눈으로 봅니다.

> 이 랩은 일부가 실제 인프라(러너 머신, Jenkins, GHES)를 필요로 합니다.
> 각 절 머리에 **[무료 가능]** / **[머신 필요]** / **[읽기]** 로 표시합니다.

---

## 6-A. self-hosted 러너 등록  [머신 필요 · 선택]

본인 노트북/VM 하나를 러너로 붙여 "러너는 아웃바운드만 쓴다"를 확인합니다.

### 해보기 — ① 러너 등록 (OS 마다 다릅니다)

레포 **Settings → Actions → Runners → New self-hosted runner** 로 가면
**운영체제와 아키텍처를 고르는 화면**이 먼저 나옵니다. 고르면 그 OS 에 맞는 명령이 화면에 생성되고,
**그 명령을 그대로 복사해 붙여넣는 것**이 설치입니다. 아래는 OS 별로 무엇이 다른지와 미리 준비할 것입니다.

=== "macOS (노트북)"
    **화면에서 고를 것**: `macOS` + 칩에 맞는 아키텍처
    (Apple Silicon = **ARM64**, 인텔 맥 = **x64**. 모르면 터미널에서 `uname -m` → `arm64` / `x86_64`)

    **미리 준비**

    ```bash
    xcode-select --install     # git, make, cc (이미 있으면 그냥 넘어감)
    cc --version && make --version
    ```

    **설치·실행** — 화면의 명령을 순서대로 (예시)

    ```bash
    mkdir actions-runner && cd actions-runner
    # curl -o ... tar.gz      ← 화면에 나오는 다운로드 명령 그대로
    # tar xzf ./actions-runner-osx-*.tar.gz
    ./config.sh --url https://github.com/<계정>/<레포> --token <화면의 토큰>
    ./run.sh                   # 이 터미널을 열어둔 동안만 러너가 살아 있음
    ```

    - `./config.sh` 질문은 전부 **Enter**(기본값). 라벨을 물으면 그냥 Enter.
    - 터미널을 닫으면 러너가 내려갑니다. 계속 띄워두려면 `./svc.sh install && ./svc.sh start` (launchd 서비스 등록, sudo 불필요).
    - 첫 실행 때 macOS 가 "확인되지 않은 개발자" 경고를 내면 **시스템 설정 → 개인정보 보호 및 보안**에서 허용.

=== "Windows (노트북)"
    **화면에서 고를 것**: `Windows` + `x64`

    **미리 준비**

    - [Git for Windows](https://git-scm.com/download/win) 설치 (없으면 checkout 실패)
    - **PowerShell 을 관리자 권한으로** 열기
    - ⚠️ 이 랩의 C 빌드(`make test`, `make`)는 Windows 에 gcc/make 가 없어 **실패합니다**.
      Windows 러너로는 아래 `hostname` 출력 job 까지만 확인하세요.
      (C 빌드까지 하려면 MSYS2 나 WSL 로 툴체인을 깔아야 합니다 — 실무의 "러너에 도구는 우리가 설치"가 이 얘기)

    **설치·실행** — 화면의 명령을 순서대로 (예시)

    ```powershell
    mkdir actions-runner; cd actions-runner
    # Invoke-WebRequest -Uri ... -OutFile actions-runner-win-x64-*.zip   ← 화면 명령 그대로
    # Expand-Archive -Path actions-runner-win-x64-*.zip -DestinationPath .
    ./config.cmd --url https://github.com/<계정>/<레포> --token <화면의 토큰>
    ./run.cmd
    ```

    - `config.cmd` 가 **"Would you like to run the runner as service?"** 를 물으면 `Y` 를 고르면 서비스로 상주합니다(창을 닫아도 유지). 실습만 할 거면 `N` + `./run.cmd`.
    - 회사 노트북은 보안 정책(스크립트 실행 차단, 방화벽)으로 막힐 수 있습니다. 막히면 6-A+ 의 클라우드 VM 으로 하세요.

=== "Linux (VM · 서버)"
    **화면에서 고를 것**: `Linux` + `x64` (ARM 서버면 `ARM64`)

    **미리 준비**

    ```bash
    sudo apt-get update && sudo apt-get install -y build-essential git curl
    ```

    **설치·실행** — 화면의 명령을 순서대로 (예시)

    ```bash
    mkdir actions-runner && cd actions-runner
    # curl -o ... tar.gz      ← 화면 명령 그대로
    # tar xzf ./actions-runner-linux-x64-*.tar.gz
    ./config.sh --url https://github.com/<계정>/<레포> --token <화면의 토큰>
    sudo ./svc.sh install && sudo ./svc.sh start && sudo ./svc.sh status
    ```

    - 서비스로 등록하면 SSH 를 끊거나 재부팅해도 러너가 살아 있습니다. 실습만 할 거면 `./run.sh`.
    - 클라우드 VM 으로 하는 전체 절차는 **6-A+** 에 있습니다.

!!! note "공통"
    - 화면의 **등록 토큰은 1시간짜리**입니다. 만료되면 같은 화면에서 새로 발급받으세요.
    - 등록이 끝나면 **Settings → Runners** 목록에 러너가 **Idle**(초록)로 보입니다. 이게 보여야 다음 단계로.
    - 어떤 OS 든 **들어오는 포트는 열지 않습니다.** 설치 중에도 방화벽에 아무것도 추가하지 않은 것을 확인하세요.

### 해보기 — ② 워크플로에서 이 러너 쓰기

워크플로의 `runs-on` 을 바꿔 커밋·올린 뒤 실행합니다.

!!! tip "수정 후 커밋하고 올리기"
    - **웹 UI**: 기존 워크플로 ✏️ → `runs-on: self-hosted` 로 변경 → **Commit changes**
    - **CLI**: `git add .github/workflows/ && git commit -m "lab6-A: self-hosted runner 테스트" && git push`

```yaml
jobs:
  on_prem:
    runs-on: self-hosted
    steps:
      - run: |
          echo "여기는 내 머신에서 돈다"
          echo "러너 이름: ${{ runner.name }}"
          hostname
```

!!! note "`runs-on` 에 쓰는 건 **이름이 아니라 라벨**입니다"
    Settings → Runners 목록에서 굵게 보이는 것(예: `Candoit`)은 러너 **이름**이고, 그 옆 회색 알약이 **라벨**입니다.
    self-hosted 러너를 등록하면 라벨 세 개가 자동으로 붙습니다 — `self-hosted`, OS(`macOS`/`Linux`/`Windows`), 아키텍처(`X64`/`ARM64`).
    그래서 `runs-on: self-hosted` 로 쓰면 됩니다. 이름은 `runs-on` 에 쓰지 않습니다.

    **러너가 두 대 이상이면** `self-hosted` 만 적었을 때 **먼저 비어 있는 쪽**으로 갑니다. 특정 머신을 지정하려면 배열(= AND 조건)로:

    ```yaml
    runs-on: [self-hosted, Linux]      # Azure VM (6-A+)
    runs-on: [self-hosted, macOS]      # 내 노트북
    ```

    더 명확하게 하려면 커스텀 라벨을 붙입니다.

    - 등록할 때: `./config.sh --labels azure-vm …`
    - 이미 등록했으면: **Settings → Runners → 러너 클릭 → Labels** 에서 추가

    그다음 `runs-on: [self-hosted, azure-vm]`. 없는 라벨을 적으면 에러 없이 **큐에서 계속 대기**합니다(도전 과제 1).

### 눈으로 확인
- 내 머신의 hostname과 러너 이름(`${{ runner.name }}`)이 로그에 찍힘
- 러너 등록·실행 내내 **들어오는 포트를 열지 않았다** — 러너가 GitHub으로 **나가는** 연결만 씀
- Settings → Runners 에서 러너가 job 실행 중에는 **Active**, 끝나면 다시 **Idle**

### 정리 (끝나고 꼭)

=== "macOS · Linux"
    ```bash
    # ./run.sh 로 띄웠으면 그 터미널에서 Ctrl+C
    # 서비스로 등록했으면
    sudo ./svc.sh stop && sudo ./svc.sh uninstall     # macOS 는 sudo 없이

    # 등록 해제 (Settings → Runners → 해당 러너 → Remove 화면의 토큰 사용)
    ./config.sh remove --token <제거 토큰>
    ```

=== "Windows"
    ```powershell
    # run.cmd 로 띄웠으면 Ctrl+C
    ./config.cmd remove --token <제거 토큰>
    ```

지우지 않고 두면 Settings 목록에 **Offline** 으로 남고, 14일 뒤 자동 삭제됩니다.

### 왜
러너 앱이 GitHub에 붙어 "일감 있나요?"를 계속 묻고(long poll), 일감을 받으면 실행하고, 결과를 다시 GitHub으로 올립니다.
모든 연결이 **러너 → GitHub 방향**이라 러너 쪽에 들어오는 포트를 열 필요가 없고, 방화벽엔 **아웃바운드 HTTPS 443** 하나만 요청하면 됩니다.
(Jenkins 아시는 분: controller ↔ agent 는 SSH 22 또는 JNLP 50000 인바운드가 필요했던 것과 대비)

### 실무 대응
- label(`self-hosted,linux,ghs-toolchain`)과 **runner group**으로 라우팅·라이선스 통제.
- 대규모는 **ARC**(Kubernetes 위 러너) — Helm 차트 두 개, Pod 하나 = Job 하나.

---

## 6-A+. 클라우드 VM을 러너로 — 사내 빌드 VM과 같은 모양  [클라우드 계정 필요, 선택]

노트북 대신 **Linux VM**을 러너로 붙이면 "온프레미스 빌드 VM에 러너 설치"와 같은 그림이 됩니다.
노트북을 닫아도 러너가 살아 있어서 Lab 4-D, Lab 5를 self-hosted에서 이어서 돌려볼 수 있습니다.
여기서는 Azure를 예로 듭니다 (AWS EC2, 사내 VM도 절차는 같습니다).

### 흐름 그림

```
  Azure Cloud Shell               Azure                                  GitHub
  ┌────────────┐  SSH 22          ┌──────────────────────────────┐          ┌──────────┐
  │ 브라우저    │ ───────────────▶ │ runner-01 (Ubuntu 24.04)     │          │          │
  │ az / ssh    │                  │  actions-runner/ svc 상주     │ ───────▶ │ Job 큐   │
  └────────────┘                   │  gcc, make                   │ 443 나감 │          │
                                   └──────────────────────────────┘          └──────────┘
                                   NSG 인바운드: 22번 하나뿐 ─ GitHub에서 들어오는 규칙 없음
```

!!! tip "전부 브라우저 안에서 — Azure Cloud Shell"
    아래 명령은 **[portal.azure.com](https://portal.azure.com) 오른쪽 위 `>_` (Cloud Shell)** 에서 실행합니다.
    본인 PC 에 `az` 나 `ssh` 를 설치할 필요가 없고, 로그인도 이미 되어 있습니다.
    처음 열면 **Bash** 를 고르고, 스토리지 만들라고 하면 만들면 됩니다.

### 해보기 0 — 내 리소스 그룹 확인

Cloud Shell 은 포털에 로그인한 계정(`user0N@nrkim0615outlook.onmicrosoft.com`)으로 **이미 로그인된 상태**입니다.

```bash
az account show --query user.name -o tsv    # 내 계정 확인
az group list --query '[].name' -o tsv      # 내 리소스 그룹 (예: user02-rg)
```

!!! warning "본인 리소스 그룹만 사용"
    각 계정은 자기 리소스 그룹에만 권한이 있고, 새 그룹을 만들 수는 없습니다.
    다른 번호의 그룹을 지정하면 `AuthorizationFailed` 로 실패합니다.

### 해보기 1 — VM 만들기 (Cloud Shell)

```bash
RG=user02-rg        # 본인 계정 번호에 맞게 (해보기 0 에서 확인한 이름)

az vm create -g $RG -n runner-01 --image Ubuntu2404 --size Standard_B2s \
  --admin-username azureuser --generate-ssh-keys --nsg-rule SSH --public-ip-sku Standard \
  --query '{ip:publicIpAddress}' -o table
```

출력된 **공용 IP를 메모**합니다. 다음 단계에서 계속 쓰니 변수에 담아두면 편합니다.

```bash
IP=$(az vm show -d -g $RG -n runner-01 --query publicIps -o tsv); echo $IP
```

- `--generate-ssh-keys` 로 만든 키는 Cloud Shell 홈(`~/.ssh`)에 저장되어 **다음 세션에도 남습니다.**
- `--nsg-rule SSH` 는 인바운드에 **22번 하나만** 엽니다. GitHub 쪽에서 들어오는 규칙은 끝까지 만들지 않습니다.

??? tip "인바운드를 더 좁히고 싶다면"
    특정 IP 로만 SSH 를 허용할 수 있습니다. 단 **Cloud Shell 의 출발 IP 는 세션마다 바뀌므로**, 좁히면 다음 세션에서 접속이 안 될 수 있습니다. 그때는 이 명령을 다시 실행하면 됩니다.

    ```bash
    az network nsg rule update -g $RG --nsg-name runner-01NSG -n default-allow-ssh \
      --source-address-prefixes $(curl -s ifconfig.me)
    ```

??? tip "SSH 를 아예 안 열고 하기 (인바운드 0개)"
    VM 을 `--nsg-rule NONE` 으로 만들면 **인바운드 규칙이 하나도 없습니다.** 그래도 `az vm run-command` 로 명령을 넣을 수 있어 실습이 가능합니다. "러너는 나가는 연결만 쓴다"를 가장 강하게 보여주는 구성입니다.

    ```bash
    az vm create -g $RG -n runner-01 --image Ubuntu2404 --size Standard_B2s \
      --admin-username azureuser --generate-ssh-keys --nsg-rule NONE --public-ip-sku Standard
    ```

    이 경우 아래 단계의 `ssh ...` 대신 이렇게 실행합니다.

    ```bash
    az vm run-command invoke -g $RG -n runner-01 --command-id RunShellScript \
      --scripts "여기에 실행할 셸 명령" --query 'value[0].message' -o tsv
    ```

### 해보기 2 — 빌드 도구 설치 (Cloud Shell → VM)

GitHub 호스티드 러너에는 gcc, make, az 등이 **미리 깔려** 있지만, 내 VM은 **내가 깔아야** 합니다.
"self-hosted 러너 = 내가 관리하는 머신"의 첫 체감입니다.

```bash
ssh -o StrictHostKeyChecking=accept-new azureuser@$IP \
  'sudo apt-get update -q && sudo apt-get install -y -q build-essential && gcc --version | head -1'
```

### 해보기 3 — 러너 등록 (VM 안에서)

#### 3-1. GitHub 에서 등록 명령 받기

실습 레포 → **Settings** → 왼쪽 **Actions → Runners** → 오른쪽 위 **New self-hosted runner**

- **Runner image**: `Linux`
- **Architecture**: `x64`

그러면 아래처럼 **Download** 와 **Configure** 두 묶음의 명령이 생성됩니다. 이 창을 **그대로 열어둡니다**(토큰이 여기 있습니다).

!!! danger "아래 코드는 '모양' 예시입니다 — 절대 그대로 복사하지 마세요"
    버전 번호와 해시가 실제 값이 아니라 `…` 로 적혀 있습니다.
    **반드시 GitHub 화면에 생성된 명령을 복사**하세요. 화면의 각 줄 오른쪽 📋 버튼을 누르면 됩니다.

```
# Download  (화면에는 실제 버전과 해시가 들어 있습니다)
mkdir actions-runner && cd actions-runner
curl -o actions-runner-linux-x64-<버전>.tar.gz -L https://github.com/actions/runner/releases/download/v<버전>/actions-runner-linux-x64-<버전>.tar.gz
echo "<64자리 해시>  actions-runner-linux-x64-<버전>.tar.gz" | shasum -a 256 -c
tar xzf ./actions-runner-linux-x64-<버전>.tar.gz

# Configure
./config.sh --url https://github.com/<계정>/<레포> --token <등록 토큰>
```

!!! warning "토큰은 1시간짜리"
    `--token` 뒤의 값은 **등록 전용 토큰**이고 1시간 뒤 만료됩니다. 만료되면 같은 화면을 새로고침해 새 명령을 받으세요.
    이 토큰은 시크릿이 아니라 등록용이지만, 남에게 공유하지는 마세요.

!!! failure "root 로 하지 마세요"
    프롬프트가 `root@runner-01` 이면 `sudo -i` 등으로 root 가 된 상태입니다. 러너는 **일반 계정(`azureuser`)으로 설정**해야 하고,
    root 로 `./config.sh` 를 실행하면 `Must not run with sudo` 로 거부됩니다.

    ```bash
    exit                                                # azureuser 로 돌아오기
    sudo chown -R azureuser:azureuser ~/actions-runner  # root 로 받은 파일 소유권 정리
    ```

    `sudo` 를 쓰는 건 마지막 서비스 등록(`svc.sh`) 뿐입니다.

#### 3-2. Cloud Shell 에서 VM 에 접속

```bash
ssh azureuser@$IP
```

- 처음 접속하면 `Are you sure you want to continue connecting (yes/no/[fingerprint])?` → **`yes`** 입력
- `$IP` 가 비어 있으면 다시 잡습니다: `IP=$(az vm show -d -g $RG -n runner-01 --query publicIps -o tsv); echo $IP`
- 접속되면 프롬프트가 `azureuser@runner-01:~$` 로 바뀝니다. **여기서부터는 VM 안**입니다.

#### 3-3. 다운로드 → 압축 해제 (VM 안에서)

3-1 의 **Download** 묶음을 **한 줄씩 순서대로** 붙여넣습니다.

| 명령 | 하는 일 |
|------|---------|
| `mkdir actions-runner && cd actions-runner` | 러너 앱을 풀어 둘 폴더를 만들고 들어감 (홈 디렉터리 아래) |
| `curl -o … -L https://github.com/actions/runner/releases/…` | 러너 앱 압축 파일을 내려받음 (약 200 MB, 몇 초~1분) |
| `echo "<해시>  …tar.gz" \| shasum -a 256 -c` | 내려받은 파일이 손상/변조되지 않았는지 검사. **화면의 줄을 그대로** 써야 합니다(해시가 들어 있음). 건너뛰어도 설치는 됩니다. `no properly formatted SHA checksum lines found` 가 나오면 자리표시자를 붙여넣은 것 |
| `tar xzf ./actions-runner-linux-x64-*.tar.gz` | 압축 해제. `config.sh`, `run.sh`, `svc.sh` 가 생깁니다 |

확인:

```bash
ls
# bin  config.sh  env.sh  externals  run.sh  safe_sql.json  svc.sh  ...
```

#### 3-4. 등록 (`config.sh`)

3-1 의 **Configure** 줄을 붙여넣습니다.

```bash
./config.sh --url https://github.com/<계정>/<레포> --token AXXXXXXXXXXXXXXXXXXXXXXXXX
```

질문이 네 개 나옵니다. **대부분 그냥 Enter** 입니다.

| 질문 | 뜻 | 이번 실습에서는 |
|------|-----|----------------|
| `Enter the name of the runner group:` | 러너를 묶는 그룹 (조직 레벨 기능) | **Enter** (Default) |
| `Enter the name of runner:` | 이 러너의 이름. Settings 목록에 표시됨 | **Enter** (`runner-01`, VM 호스트명) |
| `Enter any additional labels (ex. label-1,label-2):` | 추가 라벨 — `runs-on` 에서 이 러너를 콕 집을 때 씀 | `azure-vm` 입력 후 Enter (권장) |
| `Enter name of work folder:` | 작업 폴더 이름 | **Enter** (`_work`) |

성공하면 이렇게 나옵니다.

```
√ Connected to GitHub
√ Runner successfully added
√ Runner connection is good
√ Settings Saved.
```

!!! failure "여기서 막히면"
    - `Http response code: NotFound` → URL 오타 또는 토큰 만료. 3-1 화면을 새로고침해 다시 복사.
    - `Must not run with sudo` → `sudo ./config.sh` 로 실행한 경우. `sudo` 없이 실행하세요.
    - `libicu` 관련 에러 → `sudo apt-get install -y libicu-dev` 후 재시도.

#### 3-5. 서비스로 상주시키기

화면 마지막에 안내되는 `./run.sh` 는 **터미널을 닫으면 같이 죽습니다.** 대신 서비스로 등록합니다.

```bash
sudo ./svc.sh install     # systemd 서비스 등록
sudo ./svc.sh start       # 시작
sudo ./svc.sh status      # active (running) 확인
```

```
● actions.runner.<계정>-<레포>.runner-01.service - GitHub Actions Runner
     Active: active (running) since ...
```

이제 SSH 를 끊어도, VM 을 재부팅해도 러너가 살아 있습니다.

```bash
exit        # VM 에서 나오기 (러너는 계속 돕니다)
```

#### 3-6. 등록 확인

GitHub 레포 → **Settings → Actions → Runners** 에 `runner-01` 이 **Idle**(초록 점)로 보이면 성공입니다.
라벨에 `self-hosted` `Linux` `X64` `azure-vm` 이 붙어 있습니다.

### 눈으로 확인
- Settings → Runners에 `runner-01` 이 **Idle**(초록)
- 6-A의 `runs-on: self-hosted` 워크플로 실행 → 로그에 `runner-01`
- **Azure 포털 → runner-01 → 네트워킹**: 인바운드 규칙에 22번(내 IP)뿐인데 job이 배정됨
  → 러너가 GitHub으로 **나가서** 일감을 받아오는 구조라는 증거

### 🤔 생각해보기
- 이 VM에서 Lab 3 `pipeline.yml`의 `build` job을 돌리면 어떻게 될까요? `runs-on`만 바꿔서 해보세요.
  (힌트: 호스티드 러너엔 있고 내 VM엔 없는 것, 그리고 "매번 새 머신"이 아니라는 것)
- Lab 4-D의 `publish` job을 여기서 돌리려면? (`az` CLI가 없음 → `curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash`)

### 왜
- 사내 빌드 VM에 러너를 설치하는 절차가 **이것과 똑같습니다**. 다른 점은 방화벽 협의뿐인데, 그마저 "443 나가는 것"만 열면 됩니다.
- `svc.sh` 로 서비스 등록을 안 하면 SSH 세션이 끊길 때 러너가 죽고, 재부팅 후 사라집니다.

### 실무 대응
- 러너를 **runner group** 으로 묶어 특정 레포/팀만 쓰게 통제합니다.
- 툴체인(컴파일러, 라이선스 도구)이 깔린 VM은 label(`ghs-toolchain` 등)로 구분해 `runs-on: [self-hosted, ghs-toolchain]` 으로 라우팅합니다.
- "매번 새 머신"이 아니므로 이전 빌드 찌꺼기가 남습니다. `ephemeral` 러너나 ARC로 해결합니다.

### 정리 (비용)
Cloud Shell 에서 실행합니다. 쓰지 않을 때는 끄고(할당 해제 → 과금 0), 다 끝나면 지웁니다. 지우기 전에 Settings → Runners에서 러너를 **Remove** 합니다.

```bash
az vm deallocate -g $RG -n runner-01        # 끄기 (다시 켜기: az vm start)
```
```bash
az vm delete -g $RG -n runner-01 --yes      # 완전 삭제
```
> VM만 지우면 디스크, NIC, 공용 IP, NSG가 남을 수 있습니다. 포털에서 `runner-01` 로 시작하는 리소스를 함께 삭제하세요.

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

## 6-D. 종합 실습 — VM 러너에서 빌드하고, 승인 후 Blob에 올리기  [클라우드 계정 필요, 선택]

Lab 6-A+(VM 러너)와 Lab 4-D(Blob 업로드), Lab 3-D(승인 게이트)를 **하나의 파이프라인**으로 잇습니다.
새로 배우는 건 없습니다. 지금까지 만든 것을 사내 환경과 같은 모양으로 조립하는 실습입니다.



세 덩어리, 화살표 셋:
- **Azure Cloud Shell(브라우저)** — `az`로 VM/저장소를 만들고 `ssh`로 러너를 설치. GitHub은 브라우저에서 Run workflow와 승인
- **Azure** — `runner-01` VM(러너 앱 + 우리가 깐 gcc/make/az CLI) + Blob Storage. NSG 인바운드는 SSH 22(내 IP)뿐
- **GitHub** — 실습 레포, Job 큐, Settings → Runners, `production` 환경(승인)
- 화살표는 전부 **나가는 방향**: Cloud Shell→VM(SSH, 관리용), VM→GitHub(① 443 일감 요청/배정), VM→Blob(② 443 업로드)

### 준비 (이미 했으면 건너뜀)
- [ ] Lab 6-A+ 의 `runner-01` 이 Settings → Runners 에 **Idle**
- [ ] Lab 4-D 의 저장소 계정, `firmware` 컨테이너, 시크릿 `AZ_SAS`, 변수 `AZ_STORAGE_ACCOUNT`
- [ ] Lab 3-D 의 `production` 환경 + Required reviewers
- [ ] **VM에 az CLI 설치** (호스팅 러너엔 있었지만 내 VM엔 없음):

Cloud Shell 에서 (IP 를 다시 잡으려면 `IP=$(az vm show -d -g $RG -n runner-01 --query publicIps -o tsv)`):

```bash
ssh azureuser@$IP 'curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash && az version'
```

### 해보기
`.github/workflows/publish-onprem.yml` 을 새로 만듭니다.

!!! tip "파일 만들고 올리기"
    - **웹 UI**: **Add file** → **Create new file** → `.github/workflows/publish-onprem.yml` → **Commit changes**
    - **CLI**: `git add .github/workflows/publish-onprem.yml && git commit -m "lab6-C: VM 러너 종합 실습 [skip ci]" && git push`

    > `[skip ci]` 를 붙여 push 후, **Run workflow** 로 수동 실행합니다.

Lab 4-D `publish.yml` 에서 바뀐 곳은 주석 표시한 세 줄뿐입니다.

```yaml
name: Lab6 종합 — VM 러너에서 빌드, 승인 후 Blob
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: self-hosted                 # ← 호스팅 러너 대신 내 VM
    steps:
      - uses: actions/checkout@v7
      - run: hostname                    # ← 어느 머신인지 로그에 남김
      - run: make test
      - run: make
      - uses: actions/upload-artifact@v7
        with:
          name: app
          path: build/app

  publish:
    needs: build
    runs-on: self-hosted                 # ← 업로드도 내 VM에서 (az CLI 설치 필요)
    environment: production              # ← 승인 후에만 시작
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
          echo "## firmware 컨테이너 (러너: $(hostname))" >> "$GITHUB_STEP_SUMMARY"
          az storage blob list --container-name firmware \
            --account-name "$ACCT" --sas-token "$SAS" \
            --query '[].name' -o tsv | sort >> "$GITHUB_STEP_SUMMARY"
```

커밋 메시지에 `[skip ci]` 붙여 push → **Run workflow**.

### 눈으로 확인 (순서대로)
1. `build` job 로그의 `hostname` = `runner-01`. Settings → Runners 에서 러너가 **Active** 로 바뀌었다가 Idle 로 돌아옴
2. `publish` 가 노란 **승인 대기** — 이 시점엔 VM에서 아무것도 안 돎(러너 Idle)
3. **Review deployments → Approve** → 그제야 `publish` 가 runner-01 에서 시작
4. Summary 에 `v0.<번호>/app` 목록. Cloud Shell 에서도: `az storage blob list -c firmware --account-name $ACCT --sas-token "$SAS" -o table`
5. Azure 포털 → runner-01 → 네트워킹: 인바운드 규칙은 여전히 SSH 하나

### 🤔 생각해보기
- `build` 를 두 번 실행하고 VM에서 `ls ~/actions-runner/_work/*/*/build` 를 보세요. 이전 빌드가 남아 있나요? 호스팅 러너와 무엇이 다른가요?
- `publish` 만 호스팅 러너(`ubuntu-latest`)로 바꾸면 무엇이 달라지나요? (힌트: az CLI, 아티팩트는 어디를 거치나)
- SAS 대신 OIDC(`azure/login`)로 바꾸면 시크릿이 몇 개 남을까요?

### 왜
- 이 파이프라인이 **사내 온프레미스 구성의 축소판**입니다: 빌드 VM(=러너) → 승인(=결재) → 외부 저장소(=Artifactory). 다른 건 Azure 자리에 사내 VM, Blob 자리에 Artifactory가 들어가는 것뿐.
- 방화벽 요청서는 한 줄입니다: "빌드 VM에서 GitHub(GHES)과 저장소로 **아웃바운드 443**". 인바운드 없음.
- self-hosted 러너는 도구(az CLI)도, 찌꺼기(_work)도 우리 책임 — 그래서 6교시의 ephemeral/ARC 얘기로 이어집니다.

### 실무 대응

| 실습 | 사내 |
|---|---|
| Azure VM `runner-01` | 기존 빌드 VM에 러너 앱 설치 (Lab 7) |
| `runs-on: self-hosted` | `runs-on: [self-hosted, linux, ghs-toolchain]` + runner group |
| Blob + SAS | Artifactory + `jf` CLI (+ OIDC) |
| `production` 환경 승인 | 결재 담당 팀을 Required reviewers 로 |
| GitHub.com | GHES (러너 등록 URL만 GHES 주소로) |

### 정리 (비용)
Lab 6-A+, 4-D 의 정리 절 참고. VM은 `az vm deallocate`, 저장소는 `az storage account delete`.

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
- [ ] (선택) 클라우드 VM을 러너로 붙이고, 인바운드 규칙 없이 job이 배정되는 것을 봤다
- [ ] (선택) VM 러너 → 승인 → Blob 업로드를 한 파이프라인으로 돌려봤다 (6-D)
- [ ] Actions Importer로 Jenkinsfile을 변환해봤다
- [ ] Groovy 로직이 자동 변환 안 되는 것을 확인했다
- [ ] actions-sync가 왜 필요한지 이해했다


## 🔧 도전 과제

> 기본 실습을 마쳤으면 아래를 **답 코드 없이** 해봅니다.
> 이 랩에서 배운 것에 **📖 공식 문서**(또는 검색)를 조금 더하면 풀 수 있는 실무형 과제입니다.
>
> ★ 5분 · ★★ 10분 · ★★★ 15분 — 다 못 해도 됩니다. 시간이 남는 분이 하는 과제입니다.

---

### 도전 1 · ★★ — 라벨로 라우팅

러너에 라벨을 붙이고 `runs-on` 배열로 **특정 러너에만** job을 보냅니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | 러너에 `toolchain-a` 라벨 추가, `runs-on: [self-hosted, toolchain-a]` 로 지정. 없는 라벨(`toolchain-b`)로 바꿨을 때 어떻게 되는지도 확인 |
| **완료 조건** | 맞는 라벨 → 실행 / 없는 라벨 → job이 **큐에서 계속 대기**(노란색) → 취소 후 원복 |

??? tip "힌트"
    - 라벨 편집: Settings → Runners → 러너 클릭 → 라벨 편집
    - `runs-on` 에 배열을 주면 **AND 조건** (모든 라벨을 가진 러너에만 배정)
    - 큐 대기가 24시간 넘으면 어떻게 되는지는 "사용량 제한" 절 확인

---

### 도전 2 · ★★★ — 변환 안 되는 Jenkinsfile 손으로 옮기기

Importer가 **TODO로 남긴 부분**을 Lab 3에서 배운 것으로 직접 채웁니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | 아래 stage를 6-B Jenkinsfile에 추가 후 `dry-run`, 빠진 부분을 직접 완성 |
| **완료 조건** | `when { branch 'main' }` 과 `input` 이 Actions의 무엇으로 바뀌었는지 설명 가능, 완성본이 실습 레포에서 실제로 승인 대기까지 감 |

추가할 stage:

```groovy
stage('Deploy') {
  when { branch 'main' }
  steps {
    input message: '배포할까요?'
    sh 'make deploy'
  }
}
```

??? tip "힌트"
    - `when { branch 'main' }` → job `if:` + `github.ref`
    - `input` → Lab 3-D (환경 승인 게이트)
    - 이 과제가 **7교시 워크시트의 "Manual Approval" 줄**과 이어집니다

---

### 도전 3 · ★ — 러너 정리 자동화

VM 러너를 **쓰지 않는 시간에 자동으로 할당 해제**하는 방법을 조사합니다.

| 항목 | 내용 |
|------|------|
| **요구사항** | 6-A+ VM 러너를 매일 20시에 자동 할당 해제하는 방법 조사 (Actions 또는 Azure 어느 쪽이든) |
| **완료 조건** | 방법 하나를 골라 이유와 함께 한 줄로 적기 (실행까지는 선택) |

??? tip "힌트"
    - Azure VM에는 **"자동 종료"** 설정이 기본 제공됩니다
    - Actions 쪽이라면 `schedule` + `az vm deallocate` + OIDC 로그인(4교시) 조합
    - 어느 쪽이 더 단순한지 비교해보세요

## 📖 공식 문서

- [self-hosted 러너](https://docs.github.com/en/actions/reference/runners/self-hosted-runners)
- [ARC(Actions Runner Controller)](https://github.com/actions/actions-runner-controller)
- [Jenkins에서 이전(Actions Importer)](https://docs.github.com/en/actions/migrating-to-github-actions/automated-migrations/migrating-from-jenkins-with-github-actions-importer)
- [사용량 제한](https://docs.github.com/en/actions/reference/limits)

<!-- NAV -->

---

[← Lab 5 · 표준화와 재사용](lab5-reuse-standardize.md)  ·  [🏠 랩 목록](../)  ·  [Lab 7 · 전환 워크숍 →](lab7-migration-workshop.md)

