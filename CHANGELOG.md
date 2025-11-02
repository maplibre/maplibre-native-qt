# Changelog

## v4.0.0 (unreleased)

### ✨ New features

- Full renderer backend support for Vulkan, Metal and OpenGL.

## v3.0.0

### ✨ New features

- Completely reorganized the project structure into the `QMapLibre` namespace
  and three libraries: `QMapLibre`, `QMapLibreLocation` and `QMapLibreWidgets`.
- Reference documentation now available at
  https://maplibre.org/maplibre-native-qt/docs/.
- Built with Qt 6.5, 6.6 and 6.7 for all platforms and also Qt 5.15.2 for
  macOS, Linux and Windows.
- QML configuration cleaned up, styles are now set with `maplibre.map.styles`.
- QML style parameters are also made available in C++.
  Now imported using `import MapLibre 3.0`.
- QML plugins can be installed using a CMake helper function
  `qmaplibre_location_setup_plugins`.
- Add ability to build as static libraries (#98).
- Add CMake presets for easier usage (#112).
- Support image coordinate change (#139).
- Add mouse events with coordinate to GLWidget (#141).
- Improve GeoJSON and image source handling (#164).

### 🐞 Bug fixes

- Improve handling of system ICU on Linux (#56).
- Allow to use as a CMake included project (#100).
- Make creation of Style with empty URL possible (#107).
- Set proper soversion (#117).
- Use less generic target names to allow usage as subproject (#127).
- Fix style filters setting (#163).

## v2.1.0

### ✨ New features

- Based on Qt 6.4.3

### 🐞 Bug fixes

- Reset GL state before rendering MapLibre (#19)

## v2.0.2

### 🐞 Bug fixes

- Fixed issues with iOS binaries.

## v2.0.1

### 🐞 Bug fixes

- Fixed issues with release tarballs.

## v2.0

### ✨ New features

- Full Qt5 and Qt6 support for macOS, Linux, Windows, iOS and Android.
