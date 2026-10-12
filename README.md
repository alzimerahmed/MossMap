<div align="center">
  <img src="qt/res/logo.png" height="100"/>
</div>
<h1 align="center">MossMap</h1>

**MossMap** is a privacy-first offline maps & GPS app for hiking, cycling, and driving — a community-driven fork of [Organic Maps](https://github.com/organicmaps/organicmaps). Absolutely free. No ads. No tracking. Powered by [OpenStreetMap](https://www.openstreetmap.org) data.

[![Android CI](https://github.com/alzimerahmed/MossMap/actions/workflows/android-check.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/android-check.yaml)
[![iOS CI](https://github.com/alzimerahmed/MossMap/actions/workflows/ios-check.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/ios-check.yaml)
[![Linux CMake CI](https://github.com/alzimerahmed/MossMap/actions/workflows/build-cmake.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/build-cmake.yaml)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg?logo=apache)](LICENSE)

*The whole world, in your pocket — offline.*

[Features](#features) • [Tech Stack](#tech-stack) • [Building](#building)

---

## Features

Inherited from Organic Maps:

- Detailed offline maps with places that don't exist on other maps, thanks to [OpenStreetMap](https://openstreetmap.org)
- Cycling routes, hiking trails, and walking paths
- Contour lines, elevation profiles, peaks, and slopes
- Turn-by-turn walking, cycling, and car navigation with voice guidance
- Subway maps and public transport routes
- Fast offline search on the map
- Bookmarks and tracks import/export in KML, KMZ, GPX, GeoJSON formats
- Dark Mode
- Wikipedia articles for places of interest
- Free and open-source (Apache-2.0)

## Screenshots

See the [upstream Organic Maps README](https://github.com/organicmaps/organicmaps#screenshots) for screenshots.

## Tech Stack

| Layer | Technology |
|---|---|
| Core engine | C++17, CMake (`libs/`) |
| Renderer | OpenGL/Vulkan via `drape`/`drape_frontend` |
| Android | Kotlin/Java, Gradle, JNI |
| iOS | Swift/ObjC, Xcode |
| Desktop | Qt, CMake |
| Data pipeline | `generator/`, Python tooling (`tools/python`) |
| Map data | OpenStreetMap |

```
libs/         C++ core (indexer, routing, search, storage, map, drape, ...)
android/      Android client (Gradle + JNI)
iphone/       iOS client (Maps app + CoreApi framework)
generator/    Map data generation toolchain
tools/        Python tooling, skin/style generators, mwm viewer
data/         Styles, categories, resources
3party/       Vendored third-party libraries
```

## Building

1. Clone the repo:
   ```bash
   git clone --recursive https://github.com/alzimerahmed/MossMap.git
   ```
2. Android: open `android/` in Android Studio, or `./gradlew assembleDebug` (see `android/HOW_TO_USE_GRADLE.txt`).
3. iOS: open `iphone/Maps/Maps.xcodeproj` in Xcode.
4. Desktop/generator: CMake presets — see `CMakePresets.json`.

<details>
<summary>Full build prerequisites</summary>
Per-platform toolchain requirements are documented in the upstream Organic Maps wiki.
</details>

## Usage

Launch the app, download a country/region pack over Wi-Fi once, and navigate, search, and browse fully offline afterwards.

## Contributing

Fork → branch → PR. Conventional Commits (`type(scope): description`) for changes on this fork.

## Roadmap

- [ ] Fork CI hardening (check-only workflows green on the fork)
- [ ] Feature phases from the internal roadmap (offline UX, routing, search improvements)
- [ ] Quality Gate audit (security, performance, a11y)
- [ ] Signed release builds

## Changelog

See [upstream releases](https://github.com/organicmaps/organicmaps/releases) for inherited changes; fork-specific changes will be documented in GitHub Releases.

## License & Attribution

Apache-2.0 — see [LICENSE](LICENSE), [NOTICE](NOTICE), and [CONTRIBUTORS](CONTRIBUTORS). MossMap is a fork of [Organic Maps](https://github.com/organicmaps/organicmaps) by MapsWithMe founders and the open-source community; upstream attribution is preserved.
