# Automated Multi-Registry Publishing Guide

RT-MPBC is equipped with an automated CI/CD publishing pipeline defined in [`.github/workflows/publish.yml`](../.github/workflows/publish.yml).

Whenever you push a git version tag (e.g., `v1.2.0`) or trigger the workflow manually from the GitHub Actions tab, the pipeline automatically builds, verifies, and distributes packages to all configured registries.

---

## 1. Registry Distribution Matrix

| Target Registry | Packages Published | Authentication / Secret Required |
| :--- | :--- | :--- |
| **Crates.io** (Rust) | `rtmpbc-runtime`, `rtmpbc-compiler` | `CARGO_REGISTRY_TOKEN` |
| **PyPI** (Python) | `pyrtmpbc` | `PYPI_API_TOKEN` |
| **GitHub Container Registry (GHCR)** | `ghcr.io/shahmdmahi15/rt-mpbc` | *None (uses automatic `GITHUB_TOKEN`)* |
| **GitHub Releases** | Binaries, C Headers, Static/Shared Libs | *None (uses automatic `GITHUB_TOKEN`)* |
| **Docs.rs** | Rust API documentation | *Automatic upon Crates.io publish* |

---

## 2. Generating Required Tokens

### A. Crates.io Token (Cargo)
1. Log in to [crates.io](https://crates.io).
2. Go to **Account Settings** -> [API Tokens](https://crates.io/settings/tokens).
3. Click **New Token**:
   - Name: `github-actions-rtmpbc`
   - Scopes: Select `publish-new` and `publish-update`.
4. Copy the generated token (`cio...`).

### B. PyPI Token (Python)
1. Log in to [pypi.org](https://pypi.org).
2. Go to **Account Settings** -> [API Tokens](https://pypi.org/manage/account/token/).
3. Click **Add API Token**:
   - Token name: `github-actions-rtmpbc`
   - Scope: Select **Entire account** (for initial creation of `pyrtmpbc`) or project-specific if already registered.
4. Copy the generated token (`pypi-...`).

---

## 3. Configuring Secrets in GitHub

Run these two commands in your terminal (using GitHub CLI):

```bash
# Set Crates.io token
gh secret set CARGO_REGISTRY_TOKEN --repo shahmdmahi15/rt-mpbc

# Set PyPI token
gh secret set PYPI_API_TOKEN --repo shahmdmahi15/rt-mpbc
```

*(Alternatively, go to `https://github.com/shahmdmahi15/rt-mpbc/settings/secrets/actions` and click **New repository secret**).*

---

## 4. Triggering Automated Release & Publication

Once the two secrets are added, any new release can be triggered with one git tag command:

```bash
git tag v1.2.0
git push origin v1.2.0
```

Or trigger it directly from the terminal without creating a tag:
```bash
gh workflow run publish.yml --repo shahmdmahi15/rt-mpbc
```

The CI/CD pipeline will:
1. Run full test suites (C99, Rust `#![no_std]`, and Python).
2. Publish `rtmpbc-runtime` and `rtmpbc-compiler` to **Crates.io**.
3. Build wheels/sdist and publish `pyrtmpbc` to **PyPI**.
4. Build and push the multi-stage Docker image to **GHCR**.
5. Package and attach Linux x86_64 binaries and C dev headers to the **GitHub Release**.
