#!/bin/bash

# Script to comment out the qt.cmake include line in maplibre-native CMakeLists.txt
# This is used in CI to prevent Qt-specific configuration from being included

CMAKE_FILE="vendor/maplibre-native/CMakeLists.txt"

# Check if the file exists
if [ ! -f "$CMAKE_FILE" ]; then
    echo "Error: $CMAKE_FILE not found"
    exit 1
fi

# Use sed to comment out the line if it's not already commented
# This matches lines that contain include(...platform/qt/qt.cmake) and are not already commented
sed -i.bak -E 's/^([[:space:]]*)include\(.*platform\/qt\/qt\.cmake\)/\1# include(${PROJECT_SOURCE_DIR}\/platform\/qt\/qt.cmake)/' "$CMAKE_FILE"

# Check if the replacement was made
if grep -q "^[[:space:]]*include.*platform/qt/qt.cmake" "$CMAKE_FILE"; then
    echo "Error: Failed to comment out the qt.cmake include line"
    exit 1
else
    echo "Successfully commented out qt.cmake include line (or it was already commented)"
fi

# Remove the backup file created by sed
rm -f "${CMAKE_FILE}.bak"