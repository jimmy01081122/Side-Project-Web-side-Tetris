<!-- 檔案：DESIGN.md | 作者：JimmyChang | 日期：2025/11/24 -->
# WebAssembly 俄羅斯方塊專案設計流程（圖文並茂版）

以下流程假設讀者完全沒有此專案背景，只要逐步跟著做即可完成環境建置、程式開發與最終部署。

## 1. 規劃階段

1. **確認需求**：
   - 遊戲邏輯需以 C 語言撰寫並轉為 WebAssembly（WASM）。
   - 網頁需提供設定表單，可調整列數、行數（最小 5）、以及可掉落方塊的最大次數。
   - 每種 Tetromino 需有獨特顏色，並顯示分數、消除行數與剩餘掉落次數。
   - 專案需能部署到 GitHub Pages。
2. **決定技術組合**：
   - C17 + Emscripten：將遊戲邏輯編譯成 WASM。
   - 原生 HTML/CSS/JavaScript：建立 UI、載入 WASM 並處理鍵盤事件。
   - JSON 設定檔：儲存預設列數、行數與最大掉落次數。
   - Bash 腳本：自動化建置與設定同步。
3. **資料夾結構**：
   - `src/c/`：C 語言原始碼（`tetris.c`, `tetris.h`, `wasm-interface.c`）。
   - `public/`：靜態前端資源（HTML/CSS/JS 與編譯後的 WASM）。
   - `config/`：設定檔 JSON。
   - `scripts/`：建置與同步腳本。
   - `DESIGN.md`、`README.md`：文件。

## 2. 開發階段

### 2.1 C 邏輯

1. **資料結構**：
   - `TetrominoDef`：描述每種方塊的旋轉狀態與顏色 ID。
   - `ActivePiece`：目前玩家控制中的方塊。
   - `GameState`：包含棋盤、列數行數、分數、剩餘掉落次數、亂數袋（bag of 7）等。
2. **核心函式**：
   - `tetris_init/reset`：初始化遊戲，確保列/行 >= 5，並建立隨機袋。
   - `tetris_tick`：時間前進（重力）。
   - `tetris_move/rotate/soft_drop/hard_drop`：處理玩家輸入。
   - `clear_lines`：檢查並清除完成的列，累加分數。
   - `lock_piece`：將當前方塊固定到棋盤，更新掉落計數並生成下一個方塊。
3. **WASM 介面**：
   - 在 `wasm-interface.c` 內使用 `EMSCRIPTEN_KEEPALIVE` 導出 C 函式，例如 `wasm_init_game`、`wasm_tick`、`wasm_move_left` 等，並暴露 `_malloc/_free` 以便 JS 取得記憶體緩衝區。

### 2.2 前端邏輯

1. **HTML (`public/index.html`)**：
   - 設定表單（列、行、掉落次數）。
   - HUD 顯示分數、消除行數、剩餘掉落次數與遊戲狀態。
   - Canvas 畫布繪製棋盤與方塊。
2. **CSS (`public/style.css`)**：
   - 深色太空感配色，使用 flex / grid 排版。
   - 響應式設計確保窄螢幕仍好用。
3. **JavaScript (`public/main.js`)**：
   - 載入 JSON 設定並套用表單預設值。
   - 透過 `createTetrisModule()` 載入 WASM，取得 C 函式。
   - `drawBoard()`：讀取 WASM 內的棋盤記憶體並繪圖。
   - `handleKey()`：綁定鍵盤事件（← → ↓ ↑ 、Space、Z、X）。
   - `startLoop()/stopLoop()`：控制自動掉落計時器。

## 3. 建置與測試階段

1. **安裝並啟動 Emscripten**：
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
   cd ~/emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```
2. **編譯 C → WASM**：
   ```bash
   cd /home/a/Side-Project-Web-side-Tetris
   ./scripts/build-wasm.sh
   ```
3. **同步設定檔至公開目錄**：
   ```bash
   ./scripts/sync-config.sh
   ```
4. **在本地端測試**：
   ```bash
   cd public
   python3 -m http.server 4173
   ```
   於瀏覽器開啟 `http://localhost:4173`。

## 4. 部署階段

1. 將 `public/`（或複製後的 `dist/`）推送至 GitHub Pages 專用分支，如 `gh-pages`。
2. 在 GitHub 設定 Pages 指向該分支的根目錄。
3. 更新 `README.md` 及 `DESIGN.md`，說明最新變更與流程。

## 5. 維護建議

- 變更 `config/game-settings.json` 後記得重新執行 `./scripts/sync-config.sh`。
- 若擴充功能（例如加入幽靈線、暫存方塊），請在 C 層先定義資料結構再調整前端顯示。
- 可於未來導入 Playwright 或 Cypress 進行端到端測試，確保設定表單與棋盤互動正確。

> 以上流程確保任何程度的開發者都能重建環境並了解系統組成。
