#!/usr/bin/env bash
# remove-dsstore.sh — delete all .DS_Store files in repo

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
find "$repo_root" -name '.DS_Store' -type f -print -delete
