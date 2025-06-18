cat > run_qml.sh <<'SH'
#!/usr/bin/env bash
APP="$1"; shift
exec codesign -f -s - --entitlements /dev/null --timestamp=none "$APP" && "$APP" "$@"
SH
chmod +x run_qml.sh

./run_qml.sh $(which qml)  ../quickmetal.qml