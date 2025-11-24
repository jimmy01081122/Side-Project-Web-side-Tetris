#!/usr/bin/env bash
# 檔案：scripts/sync-config.sh | 作者：JimmyChang | 日期：2025/11/24
# 功能：確保公開資料夾擁有最新設定檔，以便前端讀取

set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cp "$ROOT_DIR/config/game-settings.json" "$ROOT_DIR/public/config/game-settings.json"
echo "Copied config/game-settings.json -> public/config/game-settings.json"
