# Ambassador Envoy Fork

This repo contains the Ambassador fork of [Envoy Proxy][]. It is used to maintain a handful of patches on top of stock Envoy for some features of Edge Stack described below. At the time all of these were written, it was not possible to do these things with stock Envoy, but if there comes a point where all of these features below can be implemented using config for stock envoy, then this fork should be removed and Emissary / Edge Stack should return to using stock Envoy.

## Note to future Devs

if you find any information in this readme is no longer accurate, please do your best to help keep it updated :)

## Updating the Fork

### 1. Don't use the GitHub Sync Fork button

  For this repo, we don't actually care about Envoy Proxy's `main` branch, or any other branches other than release branches. Don't worry about GitHub telling you that this branch is out of date with the fork.
  Everything that we care about for this repo happens in the `rebase/release/x.y.z` branches. Each of those branches represents a released version of Envoy. We make a new `rebase/release/x.y.z` branch for
  each specific Envoy release rather than upstream which uses `release/x.y` and continues to update that for patch releases. The reason for this is that with [Emissary][] and [Edge Stack][], we use a similar approach to upstream
  and have `release/x.y` branches. If we were to need to ship a patch release for an older minor version of Edge Stack / Emissary, we would want the commit in this repo being referenced there to still exist, and not be missing due to a rebase to keep it updated with the upstream `release/x.y` branch and continually replaying our commits on top of that. It's a minor difference, but it makes managing previously released versions a little easier.

### 2. Create a new `rebase/release/x.y.z` branch

Later on, we will setup a development VM for building and testing Envoy. The following steps will assume that you are updating the code using your personal computer, but if you would rather start with the VM and then do your development there using an editor like vim or using VSCode over ssh, then you can skip to [step 5][], get the VM setup, then return here and perform [step 3][] and [step 4][] from the virtual machine. In most cases, the commits can be added without needing to write or change much.

Typically using a terminal editor such as vim is more than sufficient for any editing you will need to do while connected to the VM, but [for a guide on setting up VSCode over SSH, refer to this doc][]

For the following example, we are going to assume that Envoy 1.38.4 was just released and you need to upgrade Edge Stack / Emissary to use Envoy 1.38.4. Whenever you do an upgrade, please bump the example values below to the versions you used so the next person can copy/paste them.

1. Set some variables to make copy/pasting easier

```bash
export GOVERSION="1.25.10" # use the `toolchain` version from go.mod in datawire/emissary
```

```bash
export ENVOY_VERSION="1.38.4" # The version of Envoy you are upgrading to
```

```bash
export SHORT_ENVOY_VERSION="1.38" # Same as above, but without the patch version at the end
```

2. Clone this repo

   ```bash
   mkdir envoy-upgrade && 
   cd ./envoy-upgrade && 
   git clone git@github.com:gravitee-io/envoy.git .
   ```

3. Add the mainline Envoy repo as a new remote

   ```bash
   git remote add upstream git@github.com:envoyproxy/envoy.git && 
   git fetch upstream
   ```

4. Start a new `rebase/release/x.y.z` branch from the matching upstream minor release branch
   - Note: this branches from the head of `upstream/release/vX.Y`, which may be a commit or two past the `vX.Y.Z` tag. That is fine and is what previous upgrades have done.

   ```bash
   git checkout -b rebase/release/v${ENVOY_VERSION} upstream/release/v${SHORT_ENVOY_VERSION}
   ```

### 3. Add our custom commits to your new branch

Cherry-pick our custom commits from the next most recent `rebase/release/x.y.z` branch in this repo into your new branch. You can find that branch and list its custom commits with:

```bash
git branch -r | grep 'origin/rebase/release/' | sort -V | tail -n 3
```

```bash
# example for finding the custom commits on the 1.37.5 branch
git log --reverse --oneline upstream/release/v1.37..origin/rebase/release/v1.37.5
```

Keep the number of commits small: one commit per feature, with any fixups needed for a new Envoy version squashed into the feature commit they belong to. Fewer commits means fewer cherry-picks next time. Record what was folded in (and why) in the commit body so the history of workarounds is still discoverable. As of the 1.38.4 branch there are five commits:

- `feat(http1): adds Custom header rewrite rules`: Allows us to support certain header case override functionality not (currently) supported by stock envoy. This is used for the [header_case_overrides][] feature in Edge Stack.
- `feat(response_map): add custom http filter for modifying responses`: This supports the [custom error responses][] feature in Edge Stack that allows you to send custom static responses back to clients. This can likely be supported with the local reply filters now available in mainline Envoy and drop this commit. Its commit body lists the upstream refactors it has been patched around over the years (udpa/xds repo renames, context refactor, access log changes, `route()` returning `OptRef`).
- `feat(ext_authz): allow appending non-existent headers`: This supports the amb-sidecar ext_authz filter that supports Edge Stack's `Filter`/`FilterPolicy` features getting the ability to create new headers on requests instead of being limited to appending to or changing existing headers only. This feature can likely now be supported with the append actions on the ext_authz response now available in mainline Envoy and drop this commit.
- `fix golang filter end_stream=true injected before trailers`: Fixes a crash in the contrib golang HTTP filter (used by Edge Stack's `filter.so`) when data is injected while trailers are pending. Kept separate from the feature commits so it can be dropped once upstream fixes `contrib/golang/filters/http/source/processor_state.h`.
- `golang: always post continueStatus/sendLocalReply to the dispatcher (#44974)`: Backport of an upstream fix (envoyproxy/envoy#44974, fixing #44704) that never reached `release/v1.38`. Without it, `make check-envoy` (which runs in debug mode) fails `golang_filter_test` and `golang_integration_test` with `assert failure: filterState() == FilterState::ProcessingHeader`, and opt builds silently corrupt the golang filter state machine. Drop this commit on 1.39+.

Branches older than 1.38.4 carry these as many more small commits (fixups such as `udpa naming workaround`, `fix response map from context refactor`, `fix busted ext_authz test`, `Fixing custom commits`) plus commits that regenerate expired test certificates under `test/config/integration/certs`. Upstream regenerates those certs periodically, so before cherry-picking cert commits, check whether the certs on your new branch are still valid:

```bash
for f in test/config/integration/certs/*cert.pem; do echo "$f $(openssl x509 -in $f -noout -enddate)"; done
```

If they expire more than a year out, skip the cert commits (`expired_cert.pem` is intentionally expired and should be ignored). Also double check that none of the commits you cherry-pick contain leftover `<<<<<<<` conflict markers; it has happened before.

Typical things that break between Envoy minors, all of which showed up in the 1.38 upgrade:

- Bazel external repositories get renamed (for example `@com_github_cncf_udpa` became `@com_github_cncf_xds`, which became `@xds`). Bazel analysis fails within a few minutes with `Repository '@@...' is not defined`; grep our BUILD files for the old name.
- Filter callback signatures change (for example `route()` now returns `OptRef<const Router::Route>`). These show up as ordinary C++ compile errors near the end of the build.
- Upstream bugs that only trip `ASSERT`s. Upstream CI runs contrib tests only in its release job with `-c opt`, where asserts are compiled out, but `make check-envoy` runs `-c dbg`. If a contrib test fails consistently with an `assert failure`, check whether upstream `main` already has a fix (search the commit log for the file) and backport it as a separate commit rather than skipping the test.
- Runtime races that no test suite catches. Envoy 1.37 had a race between worker threads creating the V8 wasm VM at startup, so Edge Stack 3.13.2-3.14.3 crashed on boot for anyone with a wasm `EnvoyFilter` (ES-140), while `make check-envoy` and a single KAT run were green. The build also silently drops the V8 runtime if the bazel `--define wasm=` selection changes. Emissary now has two checks for this class of problem, both of which are part of the flow in [step 7][]: `make check-envoy-contract` (runs inside `make update-base`) and the wasm restart soak `make pytest-wasm-soak`.

When you are done, push your new branch to this repo and set this variable to the commit at the head of your branch

```bash
export COMMIT_HASH=$(git rev-parse HEAD)
```

### 4. Create new tags

We don't need any PRs in this repo. Once your new branch is up, you will cut two tags from the head of the branch and push them.

1. Create a version tag
   - You will likely need to first delete the local tag from the upstream envoy remote we added. We don't want to push that.

   ```bash
   git tag -d v${ENVOY_VERSION} && 
   git tag v${ENVOY_VERSION} && 
   git push origin v${ENVOY_VERSION}
   ```

2. Create a custom tag
   - Next, we push a custom tag with the format `datawire-1.38.4-<full commit hash>`. The build/release pipelines in Emissary and Edge Stack are very janky and will be looking for a tag following this format. Use the commit hash of the most recent commit on your new branch.

   ```bash
   git tag datawire-${ENVOY_VERSION}-${COMMIT_HASH} && 
   git push origin datawire-${ENVOY_VERSION}-${COMMIT_HASH}
   ```

### 5. VM Creation Guide (for building + testing Envoy)

Unfortunately the following process will not work if the above steps have not been done. If you get to the build/test step and find that there are issues such as code failing to compile or legitimate test failures, then you may have to go back to the above steps, edit the branch, delete the existing tags, and push new ones.

The following sections will walk you through setting up a virtual machine in GCP to build/test Envoy and then update Emissary with those changes once we have validated that everything builds and passes tests.

You can try using your local PC since Envoy uses Bazel and it should be hermetic; however, when you need to run the Bazel test suite, it will use a ridiculous amount of cpu, memory and disk space. We have done this once or twice in the past, but it typically involved letting a laptop with at least 1TB of disk space run at it for a day or two without having anything else open. By using a VM in GCP we can reduce the build and test time down to something more reasonable (somewhere between 1-4 hours depending on how many test flakes there are). It also allows you to work on other tasks while the VM is churning rather than bogging down your personal computer.

If someone at Ambassador wants automate the following via VM templates/scripts then that would be more efficient. We haven't prioritized that since this is a process that does not need to be done very often.

The steps here will assume you have some familiarity with Google Cloud Platform (GCP) and creating VM’s. If not be sure to spend some time familiarizing yourself with GCP. The Cloud Console (web ui) or gcloud cli can be used.

1. Create a new Virtual Machine in GCP Compute Engine
   - Make sure to create it in the `datawire-dev` project if you are from Ambassador
   - We will setup a ram disk to help with the speed, but know that the process of building/testing consumes a massive amount of CPU,  memory, and disk space.
   - The following settings are recommended. The preset machine types change from time to time, so just try to find the cheapest one with the settings closest to the below values or create a custom one.
   - **IMPORTANT: these VMs are very costly to run, so make sure you turn it off as soon as it is no longer needed**

   | Setting            | Value            |
   |--------------------|------------------|
   | Provisioning Model | Standard         |
   | vCPUs              | 112 (or more)    |
   | Memory (GB)        | 896              |
   | OS                 | Ubuntu 22.04 LTS (e.g. image `ubuntu-2204-jammy-v20260731`) |
   | Disk (GB)          | 1,500            |

2. Start the VM and connect to it
   - You can do this using `ssh` directly on your system if you want to add an ssh key to the VM, otherwise, it is very easy to connect to the instance over ssh with the following gcloud cli command.
   - Manual ssh keys:
     - [gcloud create ssh key][]
     - [gcloud add ssh key][]
   - gcloud CLI:

     ```bash
      gcloud compute ssh --zone <your vm zone> <your vm name> --project "datawire-dev"
     ```

### 6. VM Configuration Guide

1. Set the same variables you did earlier on the VM

   ```bash
   export COMMIT_HASH=<your head commit hash>
   ```

   ```bash
   export GOVERSION="1.25.10" # use the `toolchain` version from go.mod in datawire/emissary
   ```

   ```bash
   export ARCH="amd64"
   ```

   ```bash
   export ENVOY_VERSION="1.38.4" # The version of Envoy you are upgrading to
   ```

   ```bash
   export SHORT_ENVOY_VERSION="1.38" # Same as above, but without the patch version at the end
   ```

   ```bash
   export GH_EMAIL="your-email@example.com" # Your GitHub email address
   ```

   ```bash
   export GH_NAME="John Doe" # Your Name for commits
   ```

   These variables only live in the current shell. If you reconnect (or log out and back in for the Docker group step below), export them again before continuing.

   **The following steps only need to be done once for the virtual machine**

2. Update packages
   - make sure to upgrade ubuntu and packages

   ```bash
   sudo apt update && sudo apt upgrade
   ```

3. Install Dependencies

   - Various dependencies needed by Envoy/Emissary. `rsync` is required by Emissary's `compile-envoy-protos` step and is not preinstalled on Ubuntu 22.04 images; without it the step deletes `api/envoy` and `pkg/api/envoy` and then fails to copy the regenerated files back (restore with `git checkout -- api pkg/api`, install rsync, rerun `make compile-envoy-protos`).

   ```bash
   sudo apt -y install build-essential libarchive-tools software-properties-common make jq zstd git vim rsync
   ```

4. Setup Git

   - Configure git:

   ```bash
   git config --global user.name "${GH_NAME}" && 
   git config --global user.email "${GH_EMAIL}" && 
   git config --global core.editor vim
   ```

   - Generate a new SSH key to add to your GitHub account so you can push/pull the private repos. Save it to a file such as `~/.ssh/github_envoy_vm_ed25519`:

   ```bash
   ssh-keygen -t ed25519 -C "${GH_EMAIL}"
   ```

   - Add your SSH key to the ssh agent:

   ```bash
   eval "$(ssh-agent -s)" && 
   ssh-add ~/.ssh/github_envoy_vm_ed25519
   ```

   - Print out the public key and add it to your GitHub account:

   ```bash
   cat ~/.ssh/github_envoy_vm_ed25519.pub
   ```

   - Go To GitHub.com
   - `Settings` > `SSH and GPG keys` > `New SSH key`
   - Paste the public key and give it a title so that you can remember what the key is for
   - Make sure that your GitHub and git are setup to do request signing since Emissary requires all commits to be signed.
     - You can follow the pages in [GitHub's Verifying commit signatures docs][] to make sure you are setup properly.
     - If you already have commit signing setup on your personal laptop, then you can simply open `~/.gitconfig` and make sure to copy the `signingkey` entry under `[user]` to the `~/.gitconfig` of the virtual machine.

   - Run the following commands to update your git settings for use with the private repos

   ```bash
   git config --global url."git@github.com:datawire/emissary".insteadOf "https://github.com/datawire/emissary" && 
   git config --global url."ssh://git@github.com/".insteadOf "https://github.com/" && 
   git config --global hub.protocol ssh
   ```

   - When you're done, your `~/.gitconfig` should look like the following

   ```ini
   [user]
           email = <your email>
           name = <your name>
           signingkey = <your signing key>
   
   [url "git@github.com:datawire/emissary"]
           insteadOf = https://github.com/datawire/emissary
   
   [url "ssh://git@github.com/"]
           insteadOf = https://github.com/
   [core]
           editor = vim
   [hub]
           protocol = ssh
   ```

5. Setup and configure Python

   - The Envoy build, tests, and proto generation all run inside the `envoyproxy/envoy-build-ubuntu` container (via `ci/run_envoy_docker.sh`), and Bazel brings its own hermetic Python toolchain, so the host Python version does not matter for anything in this guide. You just need a working `python3`, `pip`, and `venv`, plus a `python` alias for scripts that expect it.
   - Ubuntu 22.04 ships Python 3.10 as the default `python3`, so there is no need for the `deadsnakes` PPA (it does not provide 3.10 for 22.04 anyway). Do **not** delete or re-link `/usr/bin/python3`; that is what older versions of this guide did on 20.04 and it breaks `apt` tooling.

   ```bash
   sudo apt -y install python3 python3-pip python3-venv python-is-python3 && 
   python3 --version && 
   python --version
   ```

   - If you ever need to run Emissary's Python lint or chart tests on the VM (not part of this guide), those create a venv with the Python version from `docker/base-python/Dockerfile` in Emissary (3.12 at the time of writing), which on 22.04 is available from `ppa:deadsnakes/ppa` as `python3.12 python3.12-venv`.

6. Setup Docker

   - If the following commands are out of date, refer to:
     - <https://docs.docker.com/engine/install/ubuntu/>
     - <https://docs.docker.com/engine/install/linux-postinstall/#configure-docker-to-start-on-boot>

   - Remove existing packages

   ```bash
   for pkg in docker.io docker-doc docker-compose docker-compose-v2 podman-docker containerd runc; do sudo apt-get remove $pkg; done
   ```

   - Add apt packages

   ```bash
   sudo apt-get update
   sudo apt-get install ca-certificates curl
   sudo install -m 0755 -d /etc/apt/keyrings
   sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
   sudo chmod a+r /etc/apt/keyrings/docker.asc

   echo \
     "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
     $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
     sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
   sudo apt-get update
   ```

   - Install packages

   ```bash
   sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
   ```

   - Let your user talk to the Docker daemon (don't worry if you get a `groupadd: group 'docker' already exists`)

   ```bash
   sudo groupadd docker; \
   sudo usermod -aG docker $USER
   ```

   - Then **log out and back in** (close the SSH session and reconnect) so the group membership takes effect. Note that a VS Code Remote terminal is not a new login: the VS Code server keeps running on the VM across reconnects and its terminals keep the old groups. Start a fresh `gcloud compute ssh` session, or run the "Remote-SSH: Kill VS Code Server on Host" command and reconnect. Do **not** use `newgrp docker` for the shell you run the build from: it makes `docker` your primary group, and Envoy's build container runs `groupmod -g $(id -g) envoybuild` on startup, which fails with `groupmod: GID '998' already exists` when that GID is already taken inside the build image. The build then silently does nothing and `make update-base` fails a moment later with `release.tar.zst: Cannot open: No such file or directory`. If you must stay in a `newgrp` shell, run the build with `export USER_GID=$(id -u)` set.
   - Verify with `docker run --rm hello-world` before moving on.
   - If `docker` commands fail with `permission denied while trying to connect to the docker API` and `id` no longer lists the `docker` group after you reconnect, the GCP guest agent has reset your supplementary groups. Add `docker` to the guest agent's group list so the membership survives, then log out and back in again:

   ```bash
   sudo usermod -aG docker $USER && 
   sudo sed -i 's/^groups = .*/&,docker/' /etc/default/instance_configs.cfg && 
   sudo systemctl restart google-guest-agent
   ```

   - Login to docker with the bot account (use the content of the d6edevautomaton.envoyvms.token file in Keybase as the password)

   ```bash
   docker login -u d6edevautomaton
   ```

7. Setup Go

   - Replace the following version of Go with whatever the latest being used by Emissary is

   ```bash
   curl -O -L "https://golang.org/dl/go${GOVERSION}.linux-${ARCH}.tar.gz" && 
   sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go${GOVERSION}.linux-${ARCH}.tar.gz && 
   echo 'export PATH=$PATH:/usr/local/go/bin' >> ~/.bashrc && 
   source $HOME/.bashrc &&
   go version
   ```

   - `~/.bashrc` is used rather than `~/.profile` so that `go` is on the PATH in non-login shells too (VS Code remote terminals, `tmux`, etc). If `go` disappears after reconnecting, run `source ~/.bashrc`.

8. Setup Helm

   ```bash
   curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash
   helm version
   ```

9. Create a ram disk mount

   ```bash
   echo 'tmpfs   /var/lib/docker   tmpfs   rw,size=700G   0 0' | sudo tee -a /etc/fstab && 
   sudo mount -a && 
   sudo systemctl restart docker.service; 
   df -h | grep /var/lib/docker
   ```

   - After running the above command, you should see the following output

   ```bash
   tmpfs           700G  160K  700G   1% /var/lib/docker
   ```

### 7. Building, testing, and updating Envoy for Emissary / Edge Stack

1. Within the VM, clone Emissary (the private fork, not the public repo)

   ```bash
   mkdir ~/emissary && 
   cd ~/emissary && 
   git clone git@github.com:datawire/emissary.git .
   ```

2. Start a new branch in Emissary

   ```bash
   git checkout -b dev/upgrade-envoy/v${ENVOY_VERSION}
   ```

3. Generate files

   - Run the following command to make sure that all necessary files have been generated

   ```bash
   make generate
   ```

4. Update `ENVOY_COMMIT` in `./_cxx/envoy.mk`

   - Set the new Envoy commit to the commit hash from the head of your `rebase/release/x.y.z` branch. The head commit of that branch should always be the commit that is tagged for your `vx.y.z` and `datawire-x.y.z-<commit hash>` tags.

   ```bash
   vim ./_cxx/envoy.mk
   ```

5. Compile and build Envoy

   - This will build and compile a ton of files and also push a bunch of docker images. It will take a very long time to run. Just hang out, let it run, and check back in every now and then until it is finished.
   - If any files fail to compile, you probably have an error in the code that needs to be sorted out. If so, return to the initial steps, update the branch and tags, then update `ENVOY_COMMIT` in `./_cxx/envoy.mk` again. You can work on the Envoy code from the VM or from your personal machine if you prefer.

   ```bash
   make update-base
   ```

   - After building the `base-envoy` image and before pushing it, `make update-base` runs `make check-envoy-contract`: it starts the new `envoy-static-stripped` with `--mode validate` on `_cxx/tools/envoy-contract.yaml`, a bootstrap that instantiates every Envoy extension Emissary / Edge Stack depends on (the wasm filter on `envoy.wasm.runtime.v8` loading `test/wasm-fixture/filter.wasm`, the Go filter, `ext_authz`, `ratelimit`, `lua`, our `response_map` filter, `router`). If it fails with `Failed to create Wasm VM using envoy.wasm.runtime.v8 runtime. Envoy was compiled without support for it`, `no factory found for a required type URL ...` or `Didn't find a registered implementation for 'envoy.filters.http.X'`, the binary was built without something we need (typically a bazel `--define` or a dropped custom commit, see [step 3][]). Fix the branch and tags first; the image must not be pushed in that state. You can rerun the check on its own with `make check-envoy-contract`; it validates the locally built binary when it matches `ENVOY_COMMIT`, otherwise the published image.

6. Run Envoy Tests

   - The following command will run a very large number of tests. It can take many hours to complete. A few tests might fail/flake throughout this process. This is expected. If any tests do flake, simply run the command again after it has finished and it will re-run the failed tests and skip over the tests that passed.

   ```bash
   make check-envoy
   ```

   - `make check-envoy` runs `make check-envoy-contract` first, so a binary missing an extension fails here as well, within a minute, before Bazel starts compiling tests.
   - take a screenshot of the output saying that the tests passed so that you can include it on the PR for upgrading Emissary.

7. Run the wasm restart soak

   - This is the regression check for ES-140 (see the list of things that break between minors in [step 3][]). The Envoy unit tests and a single KAT run do not exercise it, and the GitHub-hosted CI runners only have 4 vCPUs, which is not enough parallelism to reproduce the race, so it is run by hand on the build VM as part of every Envoy bump. The test installs Emissary as a Deployment with `ENVOY_CONCURRENCY=8`, a wasm `EnvoyFilter` running `test/wasm-fixture/filter.wasm` and ~300 Mappings, restarts it 15 times, and fails on any container restart, on `SIGSEGV` / `Segmentation fault` / `Failed to clone Base Wasm` in the Envoy log, or on a routed response without the filter's `x-wasm-filter: processed` header. The full description is in `DevDocumentation/DEVELOPING.md` under "Making changes to Envoy".
   - It needs a Kubernetes cluster and a registry the cluster can pull from, exactly like the KAT tests. On the VM, the simplest option is the same k3d cluster CI uses plus your Docker Hub namespace (you need to be logged in with push permission; CI uses the same `DEV_REGISTRY` mechanism):

   ```bash
   make ci/setup-k3d && 
   export DEV_KUBECONFIG=~/.kube/config && 
   export DEV_REGISTRY=docker.io/<your dockerhub namespace> && 
   export DEV_KUBE_NO_PVC=yes
   ```

   - Then run the soak. It builds and pushes the Emissary test images first, so the first run takes a while; the soak itself is 15 rollouts, roughly 10-20 minutes.

   ```bash
   make pytest-wasm-soak
   ```

   - A failure here means the new Envoy is not safe to ship with wasm filters, even if `make check-envoy` passed. Keep the pytest output (it names the iteration and the crash signature) for the Emissary PR, and look at the `--previous` container log of the ambassador pod for the Envoy stack trace.

8. Update the Envoy Go Control Plane

   ```bash
   make guess-envoy-go-control-plane-commit
   ```

   - Take the commit hash from the output of the above command, and update `ENVOY_GO_CONTROL_PLANE_COMMIT` in `./_cxx/envoy.mk`.

   ```bash
   vim ./_cxx/envoy.mk
   ```

   - Run the following command to re-generate various go controlplane files. If there are no changes, then don't worry, the go control plane does not always have updates, even if the commit hash changes.

   ```bash
   make compile-envoy-protos &&
   make generate
   ```

9. Mirroring the base envoy images

   - First, you will need to setup gcloud auth for docker so that you can mirror the dockerhub image for base envoy over to gcr. This step is required for your PR to upgrade Envoy in Emissary Ingress to pass CI.

   - Copy the contents of the `googlecloud.gcr-ci-robot` JSON key file from Keybase to a file on the VM such as ~/gcp-svc-acct.key.json
     - TODO: this process should move to creating the VM in the `datawire` project where the service account can be bound to the VM at the time of creation, but currently Ambassador devs are not given permissions that broad.

   ```bash
   gcloud auth activate-service-account --key-file=$HOME/gcp-svc-acct.key.json
   ```

   - **IMPORTANT: delete that key file after you've authed in, we don't want to leave credentials anywhere else**

   ```bash
   gcloud auth configure-docker
   ```

   ```bash
   docker pull datawire/base-envoy:envoy-0.${COMMIT_HASH}.opt && \
   docker tag datawire/base-envoy:envoy-0.${COMMIT_HASH}.opt gcr.io/datawire/ambassador-base:envoy-0.${COMMIT_HASH}.opt && \
   docker push gcr.io/datawire/ambassador-base:envoy-0.${COMMIT_HASH}.opt
   ```

10. Push your changes

   - Once all of the above are finished, you're almost done. Just commit your changes, push the branch, and then open a PR in <https://github.com/datawire/emissary>. Once that PR merges, you can open a PR in <https://github.com/datawire/apro> to upgrade Edge Stack's Emissary dependency to the version that has the updated Envoy image.

   ```bash
   git add . && git commit -m "update Envoy to version ${ENVOY_VERSION}" --signoff && git push origin dev/upgrade-envoy/v${ENVOY_VERSION}
   ```

[step 3]: #3-add-our-custom-commits-to-your-new-branch
[step 4]: #4-create-new-tags
[step 5]: #5-vm-creation-guide-for-building--testing-envoy
[step 7]: #7-building-testing-and-updating-envoy-for-emissary--edge-stack

[for a guide on setting up VSCode over SSH, refer to this doc]: ./VSCODE_WITH_SSH.md

[Envoy Proxy]: https://github.com/envoyproxy/envoy
[Emissary]: https://github.com/datawire/emissary/
[Edge Stack]: https://github.com/datawire/apro/
[gcloud add ssh key]: https://cloud.google.com/compute/docs/connect/add-ssh-keys#console_2
[gcloud create ssh key]: https://cloud.google.com/compute/docs/connect/create-ssh-keys
[header_case_overrides]: https://www.getambassador.io/docs/edge-stack/latest/topics/running/ambassador#header-behavior
[custom error responses]: https://www.getambassador.io/docs/edge-stack/latest/topics/running/custom-error-responses#custom-error-responses
[GitHub's Verifying commit signatures docs]: https://docs.github.com/en/authentication/managing-commit-signature-verification/about-commit-signature-verification
