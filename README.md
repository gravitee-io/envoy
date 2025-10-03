# Ambassador Envoy Fork

This repo contains the Ambassador fork of [Envoy Proxy][]. It is used to maintain a handfull of patches on top of stock Envoy for some features of Edge Stack described below. At the time all of these were written, it was not possible to do these things with stock Envoy, but if there comes a point where all of these features below can be implemented using config for stock envoy, then this fork should be removed and Emissary / Edge Stack should return to using stock Envoy.

## Note to future Devs

if you find any information in this readme is no longer accurate, please do your best to help keep it updated :)

## Updating the Fork

### 1. Don't use the GitHub Sync Fork button

  For this repo, we don't actually care about Envoy Proxy's `main` branch, or any other branches other than release branches. Don't worry about GitHub telling you that this branch is out of date with the fork.
  Everything that we care about for this repo happens in the `rebase/release/x.y.z` branches. Each of those branches represents a released version of Envoy. We make a new `rebase/release/x.y.z` branch for
  each specific Envoy release rather than upstream which uses `release/x.y` and continues to update that for patch releases. The reason for this is that with [Emissary][] and [Edge Stack][], we use a similar approach to upstream
  and have `release/x.y` branches. If we were to need to ship a patch release for an older minor version of Edge Stack / Emissary, we would want the commit in this repo being referenced there to still exist, and not be missing due to a rebase to keep it updated with the upstream `release/x.y` branch and continually replaying our commits on top of that. It's a minor difference, but it makes managing previously released verisons a little easier.

### 2. Create a new `rebase/release/x.y.z` branch

Later on, we will setup a development VM for building and testing Envoy. The following steps will assume that you are updating the code using your personal computer, but if you would rather start with the VM and then do your development there using an editor like vim or using VSCode over ssh, then you can skip to [step 4][], get the VM setup, then return here and perform [step 2][] and [step 3][] from the virtual machine. In most cases, the commits are be able to be added without need to write or change much.

Typically using a terminal editor such as vim is more than sufficient for any editing you will need to do while connected to the VM, but [for a guide on setting up VSCode over SSH, refer to this doc][]

For the following example, we are going to assume that Envoy 1.31.1 was just released and you need to upgrade Edge Stack / Emissary to use Envoy 1.31.1.

1. Set some variables to make copy/pasting easier

```bash
export GOVERSION="1.24.5" # use whatever the most recent version of go supported by emissary is
```

```bash
export ENVOY_VERSION="1.31.10" # The version of Envoy you are upgrading to
```

```bash
export SHORT_ENVOY_VERSION="1.31" # Same as above, but without the patch verison at the end
```

1. Clone this repo

   ```bash
   mkdir envoy-upgrade && 
   cd ./envoy-upgrade && 
   git clone git@github.com:datawire/envoy.git .
   ```

2. Add the mainline Envoy repo as a new remote

   ```bash
   git remote add upstream git@github.com:envoyproxy/envoy.git && 
   git fetch upstream
   ```

3. Start a new `rebase/release/x.y.z` branch from the matching upstream minor release branch

   ```bash
   git checkout -b rebase/release/v${ENVOY_VERSION} upstream/release/v${SHORT_ENVOY_VERSION}
   ```

### 2. Add our custom commits to your new branch

Cherry-pick our custom commits from the next most recent `rebase/release/x.y.z` branch in this repo into your new branch.
   If you need to make any edits to make sure that the commits go in cleanly, try to separate your fixup commits from the orginal commits that add the custom features so that it is easier for those in the future to tell what the original logic and what the workarounds and fixes from upstream changes/refactors are. The following custom commits you should be looking for are:

- `feat(http1): adds Custom header rewrite rules`: Allows us to support certain header case override functionality not (currently) supported by stock envoy. This is used for the [header_case_overrides][] feature in Edge Stack.
- `feat(response_map): add custom http filter for modifying responses`: This supports the [custom error responses][] feature in Edge Stack that allows you to send custom static responses back to clients when we receive. This can likely be supported with the local reply filters now available in mainline Envoy and drop this commit.
- `feat(ext_authz): allow appending non-existent headers`: This supports the amb-sidecar ext_authz filter that supports Edge Stack's `Filter`/`FilterPolicy` features getting the ability to create new headers on requests instead of being limited to appending to or changing existing headers only. This feature can likely now be supported with the append actions on the ext_authz response now available in mainline Envoy and drop this commit.
- `udpa naming workaround`: fixes a change in package dependencies
- `fix response map from context refactor`: This fixes the above response map commit which needed large changes to be compatabile with upstream refactors around contexts
- `fix busted ext_authz test`: Fixes an ext_authz test broken by our custom commit for the headers

When you are done, push your new branch to this repo and set this variable to the commit at the head of your branch

```bash
export COMMIT_HASH=$(git rev-parse HEAD)
```

### 3. Create new tags

We don't need any PRs in this repo. Once your new branch is up, you will cut two tags from the head of the branch and push them.

1. Create a version tag
   - You will likely need to first delete the local tag from the upsteam envoy remote we added. We don't want to push that.

   ```bash
   git tag -d v${ENVOY_VERSION} && 
   git tag v${ENVOY_VERSION} && 
   git push origin v${ENVOY_VERSION}
   ```

2. Create a custom tag
   - Next, we push a custom tag with the format `datawire-1.31.1-<full commit hash>`. The build/release pipelines in Emissary and Edge Stack are very janky and will be looking for a tag following this format. Use the commit hash of the most recent commit on your new branch.

   ```bash
   git tag datawire-${ENVOY_VERSION}-${COMMIT_HASH} && 
   git push origin datawire-${ENVOY_VERSION}-${COMMIT_HASH}
   ```

### 4. VM Creation Guide (for building + testing Envoy)

Unfortunately the following process will not work if the above steps have not been done. If you get to the build/test step and find that there are issues such as code failing to compile or legitimate test failures, then you may have to go back to the above steps, edit the branch, delete the existing tags, and push new ones.

The following sections will walk you through setting up a virtual machine in GCP to build/test Envoy and then update Emissary with those changes once we have validated that everything builds and passes tests.

You can try using your local PC since Envoy uses Bazel and it should be hermetic; however, when you need to run the Bazel test suite, it will use a ridiculous amount of cpu, memory and disk space. We have done this once or twice in the past, but it typicaly involved letting a laptop with at least 1TB of disk space run at it for a day or two without having anything else open. By using a VM in GCP we can reduce the build and test time down to something more reasonable (somewhere between 1-4 hours depending on how many test flakes there are). It also allows you to work on other tasks while the VM is churning rather than bogging down your personal computer.

If someone at Ambassador wants automate the following via VM templates/scripts then that would be more efficient. We haven't prioritized that since this is a process that does not need to be done very often.

The steps here will assume you have some familiarity with Google Cloud Platform (GCP) and creating VM’s. If not be sure to spend some time familiarizing yourself with GCP. The Cloud Console (web ui) or gcloud cli can be used.

1. Create a new Virtual Machine in GCP Compute enginer
   - Make sure to create it in the `datawire-dev` project if you are from Amabssador
   - We will setup a ram disk to help with the speed, but know that the process of building/testing consumes a massive amount of CPU,  memory, and disk space.
   - The following settings are recommended. The preset machine types change from time to time, so just try to find the cheapest one with the settings closest to the below values or create a custom one.
   - **IMPORTANT: these VMs are very costly to run, so make sure you turn it off as soon as it is no longer needed**

   | Setting            | Value            |
   |--------------------|------------------|
   | Provisioning Model | Standard         |
   | vCPUs              | 112 (or more)    |
   | Memory (GB)        | 896              |
   | OS                 | Ubuntu 20.04 LTS |
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

### 5. VM Configuration Guide

1. Set the same variables you did earlier on the VM

   ```bash
   export COMMIT_HASH=<your head commit hash>
   ```

   ```bash
   export GOVERSION="1.24.5" # use whatever the most recent version of go supported by emissary is
   ```

   ```bash
   export ARCH="amd64"
   ```

   ```bash
   export ENVOY_VERSION="1.31.10" # The version of Envoy you are upgrading to
   ```

   ```bash
   export SHORT_ENVOY_VERSION="1.31" # Same as above, but without the patch verison at the end
   ```

   ```bash
   export GH_EMAIL="your-email@example.com" # Your GitHub email address
   ```

   ```bash
   export GH_NAME="John Doe" # Your Name for commits
   ```

   **The following steps only need to be done once for the virtual machine**

2. Update packages
   - make sure to upgrade ubuntu and packages

   ```bash
   sudo apt update && sudo apt upgrade
   ```

3. Install Dependencies

   - Various dependencies needed by Envoy/Emissary

   ```bash
   sudo apt -y install build-essential libarchive-tools software-properties-common make jq zstd git
   ```

4. Setup Git

   - Configure git:

   ```bash
   git config --global user.name "${GH_NAME}" && 
   git config --global user.email "${GH_EMAIL}"
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

   - We will setup and install python 3.10 as required for building/testing, and then setup a link so that both `python` and `python3` use python 3.10. Note: If issues arise, Running these commands individually may help 

   ```bash
   sudo add-apt-repository ppa:deadsnakes/ppa && 
   sudo apt install python3.10 python3-pip python3.10-venv && 
   sudo rm /usr/bin/python3 && 
   sudo ln -s /usr/bin/python3.10 /usr/bin/python3 && 
   sudo ln -s /usr/bin/python3.10 /usr/bin/python && 
   sudo apt remove python3-apt &&
   sudo apt install python3-apt &&
   python3 --version && 
   python --version
   ```

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

   - Configure Docker to start on VM startup (don't worry if you get a `groupadd: group 'docker' already exists`)

   ```bash
   sudo groupadd docker; \
   sudo usermod -aG docker $USER; \
   newgrp docker
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
   echo 'export PATH=$PATH:/usr/local/go/bin' >> ~/.profile && 
   source $HOME/.profile &&
   go version
   ```

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

### 6. Building, testing, and updating Envoy for Emissary / Edge Stack

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

6. Run Envoy Tests

   - The following command will run a very large number of tests. It can take many hours to complete. A few tests might fail/flake throughout this process. This is expected. If any tests do flake, simply run the command again after it has finished and it will re-run the failed tests and skip over the tests that passed.

   ```bash
   make check-envoy
   ```

   - take a screenshot of the output saying that the tests passed so that you can include it on the PR for upgrading Emissary.

7. Update the Envoy Go Control Plane

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

8. Mirroring the base envoy images

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

9. Push your changes

   - Once all of the above are finished, you're almost done. Just commit your changes, push the branch, and then open a PR in <https://github.com/datawire/emissary>. Once that PR merges, you can open a PR in <https://github.com/datawire/apro> to upgrade Edge Stack's Emissary dependency to the version that has the updated Envoy image.

   ```bash
   git add . && git commit -m "update Envoy to version ${ENVOY_VERSION}" --signoff && git push origin dev/upgrade-envoy/v${ENVOY_VERSION}
   ```

[step 2]: #2-add-our-custom-commits-to-your-new-branch
[step 3]: #3-create-new-tags
[step 4]: #4-vm-creation-guide-for-building--testing-envoy

[for a guide on setting up VSCode over SSH, refer to this doc]: ./VSCODE_WITH_SSH.md

[Envoy Proxy]: https://github.com/envoyproxy/envoy
[Emissary]: https://github.com/datawire/emissary/
[Edge Stack]: https://github.com/datawire/apro/
[gcloud add ssh key]: https://cloud.google.com/compute/docs/connect/add-ssh-keys#console_2
[gcloud create ssh key]: https://cloud.google.com/compute/docs/connect/create-ssh-keys
[header_case_overrides]: https://www.getambassador.io/docs/edge-stack/latest/topics/running/ambassador#header-behavior
[custom error responses]: https://www.getambassador.io/docs/edge-stack/latest/topics/running/custom-error-responses#custom-error-responses
[GitHub's Verifying commit signatures docs]: https://docs.github.com/en/authentication/managing-commit-signature-verification/about-commit-signature-verification
