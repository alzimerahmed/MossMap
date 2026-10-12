# MossMap — Rules for AI Agents

## Project

MossMap = open-source offline maps app (fork of Organic Maps, Apache-2.0; upstream: https://github.com/organicmaps/organicmaps; our repo: https://github.com/alzimerahmed/MossMap, owner: Alzimer Ahmed <alzimerahmed84@gmail.com>). Inherits the full Organic Maps feature set: offline vector maps, routing (car/pedestrian/bike/transit), search, bookmarks/KML, OSM data editing, subway overlays, tracking, desktop (Qt) + Android + iOS clients. Differentiation: clean branding, privacy-first (no telemetry beyond upstream's anonymous stats), community-driven.

**License:** Apache-2.0. Upstream `LICENSE`, `NOTICE`, `LICENSES/`, `DATA_LICENSE.txt`, `CONTRIBUTORS` must be preserved. Our additions carry our copyright notice; upstream attribution stays in README.

**Scope:** Native apps only — Android (Gradle), iOS (Xcode), desktop (Qt/CMake), plus the C++ core and map generator toolchain. No web client. All verification is CI-only (see Build/Verify).

## How It Works (domain constraints — read before touching the engine)

- **C++ core is the architecture** (`libs/` — base, coding, geometry, indexer, storage, routing, search, map, drape/drape_frontend (renderer), kml, editor, platform, traffic, transit, tracking, opening_hours, ge0). Shared by all three clients. New cross-platform logic goes into `libs/`, not into a client.
- **Android** (`android/`): Gradle + Kotlin/Java, JNI bridge to the core (`android/app/src/main/cpp/`), detekt + ktlint configured (`android/detekt.yml`).
- **iOS** (`iphone/`): Swift/ObjC, Maps.app target + CoreApi/Chart frameworks.
- **Desktop** (`qt_tstfrm`, `tools/mwm_viewer`, CMake): Qt-based desktop viewer; `generator/` + `tools/` build the map data pipeline (Python + C++).
- **Data**: `data/` (styles, categories, sounds, resources); map tiles (.mwm) are generated, not committed — never hand-edit generated data.
- **Styles**: map style definitions live in `data/styles` — changes must pass the style-consistency checks.
- **Strings**: all user-facing strings via `strings.txt` (tools/python) — regenerated files under `android/` and `iphone/` are generated; never hand-edit generated string files, edit `strings.txt`.

## Tech Stack & Conventions (do not fight it)

- **Core**: C++17, CMake (presets in `CMakePresets.json`), precompiled headers (`precompiled_headers.hpp`).
- **Android**: Kotlin preferred for new code, min/target SDK per `android/build.gradle`; code style via detekt + ktlint (CI: `code-style-check-kotlin.yaml`, `detekt-check.yaml`).
- **iOS**: Swift with swift-format config (CI: `code-style-check-swift.yaml`).
- **Python tooling**: `pyproject.toml` + ruff for `tools/python` and generator scripts (CI: `code-style-check.yaml`).
- **Commits**: Conventional Commits for our changes (`type(scope): description`); upstream history uses its own style — do not rewrite it.
- **Bundle identity**: keep upstream Android `applicationId` (`app.omaps` derivatives) and iOS bundle IDs for now — renaming is a planned, CI-verified phase, never ad hoc.

## Build / Verify

```bash
# CI (GitHub Actions, .github/workflows/ — push/PR):
#   android-check.yaml        — Android build + unit tests (Ubuntu, Gradle)
#   ios-check.yaml            — iOS build + tests (macOS, Xcode)
#   build-cmake.yaml          — Linux CMake core build + tests
#   build-cmake-mac.yaml      — macOS CMake core build
#   code-style-check*.yaml    — clang-format / ktlint / detekt / swift-format / ruff
#   strings-check.yaml        — strings.txt consistency
```

**Remote-first verification (mandatory):** we do NOT build or test locally — this workspace is Windows and all builds/tests run in GitHub Actions. Local work is edit-only: static review while iterating. Push and let CI verify; a CI green check counts as the gate.

**Signing:** release/beta workflows require upstream secrets we do not have — release workflows stay disabled on the fork; check/build workflows are the active gate. Never hardcode signing material.

## Removed on Purpose — Do NOT Re-add

Repo was trimmed for size and fork-scope. These are intentionally absent — never restore them into the repo:

- `data/borders/`, `data/test_data/`, `tools/shaders_compiler/` — heavy regenerable/test-only data; **CI sparse-restores them from upstream** in `build-cmake.yaml`. Tests needing them work only in CI.
- `CLAUDE.md` (×3), `.claude/`, `.agents/`, `.gemini/`, `.github/copilot-instructions.md`, `.github/instructions/`, `.github/skills/` — non-Devin agent files.
- `INSTALL.md`, `CONTRIBUTING.md`, `docs/CONTRIBUTING.md`, `GOVERNANCE`, `DCO`, `PR_GUIDE`, `RELEASE_MANAGEMENT`, `TEAMS`, `WINDOWS_RELEASE`, `ROADMAP`, `COMMIT_MESSAGES`, `DEBUG_COMMANDS`, `ANDROID_LOCATION_TEST`, `CODE_OF_CONDUCT`, `CREDENTIALS`, `docs/badges`, `docs/sponsors` — upstream community docs we don't maintain.
- `package.json`, `.prettierrc.yaml`, `.git-blame-ignore-revs` — upstream-only tooling, dead on this fork.
- Secret-dependent workflows: `android-release`, `android-release-metadata`, `android-sdk`, `android-beta`, `ios-beta`, `ios-release`, `android-monkey` — re-adding them makes pushes fail on missing secrets.
- `LICENSE`, `NOTICE`, `CONTRIBUTORS`, `LICENSES/`, `DATA_LICENSE.txt` — **must stay** (license/attribution); never delete.

## Gotchas

- **Shallow clone** (`--depth 1`) — no upstream history locally; don't attempt history-dependent git operations.
- **No local build safety net** — grep all references (CMakeLists, gradle, pbxproj, strings.txt, styles) before deleting or renaming anything.
- **Generated files**: strings, style dumps, and `data/` regenerables are produced by `tools/` — regenerate, don't hand-edit.
- **JNI boundary**: Android changes that touch native signatures must update both Kotlin and C++ sides in the same commit.
- **3party/**: vendored third-party libs — do not upgrade casually; license compatibility must stay Apache-2.0-compatible.
- **Windows checkout**: symlinks and case-sensitive paths can be hostile; avoid `git clean -fdx`.

## Agent Guidelines & Constraints

### Do's
- **Write tests** for new core behavior (C++ unit tests in `libs/*/testing` style) and Android JVM tests; run in CI only.
- **Conventional Commits** for our changes.
- **Respect module boundaries** — new reusable logic goes into `libs/`.
- **Plan discipline** — `docs/plan.md` must contain a Quality Gate phase and a Release phase. Every `idea.md` Feature Gap entry maps to a phase or is explicitly deferred. When a phase answers an open question in `docs/research.md`, close it there in the same change.

### Don'ts
- **NO local builds or test runs** — verification is CI-only (user directive; Windows host).
- **NO secrets in code** — signing keys via CI secrets only; none exist on the fork yet.
- **NO new heavy dependencies** without a decision record in `docs/research.md` (license must stay Apache-2.0-compatible).
- **NO deleting resources** without grepping all reference types (CMakeLists, gradle, pbxproj, strings.txt, styles, workflows).
- **NO renaming app IDs/bundle IDs/targets ad hoc** — that is a planned phase, CI-verified.

## Agent Guidelines & Workflow (this repo's .devin system)

### Resource Discipline (mandatory, non-trivial tasks)
Before any non-trivial task:
1. Read `docs/toolset.md` intent-map (task type → resources)
2. Invoke every skill + sub-agent in that row
3. Read every rule for that task type (`.devin/rules/`)
4. At task end: `code-reviewer` sub-agent on final diff (non-negotiable)
5. Append learnings via `/ce-compound` if durable lesson

Phase implementations (task completes a docs/plan.md row): follow `.devin/prompt/phase.md`.

Skip all this for single-line edits, pure Q&A, reading files.

### Project-Type Filter (cross-platform offline maps app)
Per `docs/toolset.md` intent-map:
- **Skip web-only:** pwa-engineer, seo-specialist, css-architect, playwright-design-clone, payment-integrator, email-engineer, monorepo-manager, web-scraper, search-optimization (web), backend-architect (no server).
- **Keep universal:** code-reviewer, debugger, test-engineer, security-auditor, performance-engineer, git-master, migration-specialist, docs-writer, build-optimizer, devops-engineer, dx-optimizer, caveman-compressor, vibe-coding-auditor, type-safety-engineer, state-manager, i18n-specialist (strings.txt/RTL), media-optimizer, animation-engineer, frontend-designer (native UI taste), content-writer, database-engineer (map storage/search), search-architect (in-app search).
- **Quality gates:** CI-only — Android/iOS/CMake builds + tests + style checks on GitHub Actions. No local builds.

## Communication Style

Default **caveman-lite** (lightly compressed, readable, technically accurate). `/caveman` skill for full/ultra/wenyan modes.

## Quick Task Flow

Quick tasks: `.devin/prompt/quick.md` (commandments) + `.devin/prompt/rules.md` (scoping, verification, escalation). Phased work: `.devin/prompt/phase.md`.

## Key References

> Note: `docs/*.md` files below are the private knowledge layer (gitignored) — they exist only in the local workspace, not for external cloners.

- `docs/toolset.md` — intent map (task type → skills, sub-agents, rules)
- `docs/plan.md` — phased plan + status
- `docs/project.md` — project state/structure
- `docs/tools-log.md` — .devin resources invoked per session
- `docs/CONCEPTS.md` — project vocabulary
- `docs/research.md` — research, ADRs, gotchas, open questions
- `docs/idea.md` — competitive analysis + Feature Gap List
- `docs/design/design-system.md` — UI tokens + rules
