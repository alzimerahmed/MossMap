# MossMap — Privacy-first offline maps for hiking, cycling, and driving

<div align="center">

[![Android CI](https://github.com/alzimerahmed/MossMap/actions/workflows/android-check.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/android-check.yaml)
[![iOS CI](https://github.com/alzimerahmed/MossMap/actions/workflows/ios-check.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/ios-check.yaml)
[![Linux CMake CI](https://github.com/alzimerahmed/MossMap/actions/workflows/build-cmake.yaml/badge.svg)](https://github.com/alzimerahmed/MossMap/actions/workflows/build-cmake.yaml)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg?logo=apache)](LICENSE)
[![Release](https://img.shields.io/github/v/release/alzimerahmed/MossMap?logo=github)](https://github.com/alzimerahmed/MossMap/releases)

*The whole world, in your pocket — offline. Free, no ads, no tracking, powered by OpenStreetMap.*

[Download](https://github.com/alzimerahmed/MossMap/releases) • [Features](#features) • [Tech Stack](#tech-stack) • [Building](#quick-start--building)

</div>

---

MossMap is a community-driven fork of [Organic Maps](https://github.com/organicmaps/organicmaps). It inherits the full upstream feature set — offline vector maps, turn-by-turn routing, offline search, bookmarks, OSM editing — and adds its own feature batches on top.

## Features

Inherited from Organic Maps:

- Detailed offline maps with places that don't exist on other maps, thanks to [OpenStreetMap](https://openstreetmap.org)
- Cycling routes, hiking trails, and walking paths with contour lines and elevation profiles
- Turn-by-turn walking, cycling, and car navigation with voice guidance
- Subway maps and public transport routes
- Fast offline search on the map
- Bookmarks and tracks import/export in KML, KMZ, GPX, GeoJSON formats
- Dark mode, offline Wikipedia articles for places of interest
- Free and open-source (Apache-2.0)

Added by MossMap:

- Satellite/aerial imagery overlay and user-added XYZ/WMS tile sources with per-source opacity
- Weather radar overlay and point forecast (keyless, privacy-first)
- GPX track recording with configurable interval and accuracy
- Lane guidance display and sport/terrain-aware route planning (trail difficulty weighting)
- Cross-street address search ("Main St & 5th Ave")
- Mapillary street-level imagery links and Wikivoyage travel-guide links on the place page
- Bookmark media notes and settings backup/restore to file (no cloud)
- Speed camera and hazard warnings in navigation
- Opt-in live location sharing to a user-chosen/self-hosted endpoint
- On-device voice search (offline speech input, audio never leaves the phone)
- Screen-reader navigation announcements when voice guidance is off

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

## Project Structure

```
libs/         C++ core (indexer, routing, search, storage, map, drape, ...)
android/      Android client (Gradle + JNI)
iphone/       iOS client (Maps app + CoreApi framework)
generator/    Map data generation toolchain
tools/        Python tooling, skin/style generators, mwm viewer
data/         Styles, categories, resources
3party/       Vendored third-party libraries
```

## Quick Start / Building

1. Clone the repo:
   ```bash
   git clone --recursive https://github.com/alzimerahmed/MossMap.git
   ```
2. Android: open `android/` in Android Studio, or run `./gradlew assembleDebug`.
3. iOS: open `iphone/Maps/Maps.xcodeproj` in Xcode. Desktop/generator: use the CMake presets in `CMakePresets.json`.

<details>
<summary>Full build prerequisites</summary>
Per-platform toolchain requirements are documented in the upstream Organic Maps wiki.
</details>

## Usage

Launch the app, download a country/region pack over Wi-Fi once, and navigate, search, and browse fully offline afterwards.

## Contributing

Fork → branch → PR. Conventional Commits (`type(scope): description`) for changes on this fork.

## Roadmap

- [x] Fork CI hardening (check-only workflows green on the fork)
- [x] Feature batches: layers & imagery, routing & outdoors, content & data, platform & privacy
- [x] Quality Gate audits (security, performance, a11y, anti-slop)
- [x] Signed release builds (v1.0.0)
- [ ] Transit routing, delta map updates, live traffic overlay, 3D terrain relief, route alternatives

## Changelog

See [GitHub Releases](https://github.com/alzimerahmed/MossMap/releases).

## License & Attribution

Apache-2.0 — see [LICENSE](LICENSE), [NOTICE](NOTICE), and [CONTRIBUTORS](CONTRIBUTORS). MossMap is a fork of [Organic Maps](https://github.com/organicmaps/organicmaps) by MapsWithMe founders and the open-source community; upstream attribution is preserved.
