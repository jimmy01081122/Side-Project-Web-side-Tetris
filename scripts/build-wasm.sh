#!/usr/bin/env bash
# 檔案：scripts/build-wasm.sh | 作者：JimmyChang | 日期：2025/11/24
# 功能：以 emcc 將 C 程式編譯成模組化 WebAssembly，輸出到 public/ 下供網頁載入

set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/public"
mkdir -p "$OUT_DIR"
EMCC=${EMCC:-emcc}
cd "$ROOT_DIR"
# - MODULARIZE=1 / EXPORT_NAME：產生 createTetrisModule 工廠函式
# - ALLOW_MEMORY_GROWTH：讓 WASM 記憶體可以動態擴充，避免棋盤加大時溢位
# - EXPORTED_RUNTIME_METHODS：讓 JS 能用 cwrap/getValue/HEAP32 讀取記憶體
# - EXPORTED_FUNCTIONS：列出 C 端導出的控制函式與記憶體配置函式
$EMCC src/c/tetris.c src/c/wasm-interface.c \
  -Isrc/c -s WASM=1 -O2 -s MODULARIZE=1 -s EXPORT_NAME="createTetrisModule" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["cwrap","getValue","HEAP32"]' \
  -s EXPORTED_FUNCTIONS='["_wasm_init_game","_wasm_tick","_wasm_move","_wasm_move_left","_wasm_move_right","_wasm_soft_drop","_wasm_hard_drop","_wasm_rotate_cw","_wasm_rotate_ccw","_wasm_get_board_ptr","_wasm_get_rows","_wasm_get_cols","_wasm_get_score","_wasm_get_lines","_wasm_get_drop_cap","_wasm_get_drop_count","_wasm_is_game_over","_wasm_set_seed","_wasm_reset_game","_wasm_write_active_cells","_wasm_get_active_color","_malloc","_free"]' \
  -o "$OUT_DIR/tetris.js"
