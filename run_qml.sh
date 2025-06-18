#!/usr/bin/env bash
QML_BIN="$(command -v qml)"
TMP_QML="$(mktemp -t qml-XXXXX)"
cp "$QML_BIN" "$TMP_QML"
chmod +x "$TMP_QML"                 # <- add this
codesign --force --sign - --timestamp=none "$TMP_QML"
exec "$TMP_QML" "$@"