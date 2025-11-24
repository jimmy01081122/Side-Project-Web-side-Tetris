<!-- 檔案：README.md | 作者：JimmyChang | 日期：2025/11/24 -->
# WebAssembly 俄羅斯方塊專案使用說明

本專案示範如何以 C 語言撰寫俄羅斯方塊核心邏輯，透過 Emscripten 編譯為 WebAssembly，再由純 HTML/CSS/JavaScript 呈現於網頁。以下文件以「從零開始」為目標，列出所有環境設定與操作指令。

## 1. 目錄結構

```bash
config/                  # 遊戲預設設定 (JSON)
public/                  # GitHub Pages 發佈用靜態資源
 -- /index.html             # 主網頁 (載入 WASM 與繪製 Canvas)
 -- /style.css              # 版面與主題樣式
 -- /main.js                # 與 WASM 溝通、繪製棋盤
 -- /config/game-settings.json (由 sync 腳本複製)
scripts/
 --/ build-wasm.sh          # 使用 emcc 編譯 C → WASM
 --/ sync-config.sh         # 將設定檔複製到 public/config
src/c/
 -- /tetris.h               # 資料結構與函式宣告
 -- /tetris.c               # 遊戲邏輯實作
 -- /wasm-interface.c       # 導出給 JavaScript 使用的 API
DESIGN.md                # 逐步設計流程
README.md                # 本文件
```

## 2. 環境安裝與啟動

1. **取得 Emscripten SDK**
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
   cd ~/emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh    # 每次開新終端機都要重新 source
   ```
   > `source ./emsdk_env.sh` 會將 `emcc` 等指令加入目前終端機的 PATH。

2. **回到專案目錄**
   ```bash
   cd /home/a/Side-Project-Web-side-Tetris
   ```

## 3. 建置流程

1. **編譯 C → WebAssembly**
   ```bash
   ./scripts/build-wasm.sh
   ```
   - 產出檔案：`public/tetris.js` 與 `public/tetris.wasm`。

2. **同步設定檔到公開資料夾**
   ```bash
   ./scripts/sync-config.sh
   ```
   - 會建立/更新 `public/config/game-settings.json`，供前端載入。

## 4. 本地測試

```bash
cd public
python3 -m http.server 4173
```

接著以瀏覽器開啟 `http://localhost:4173`：
- 表單可調整列數、行數（最少 5）與最大掉落次數。
- WASM 模組在背後處理碰撞、旋轉、計分等邏輯。
- HUD 顯示分數、消除列、剩餘掉落次數，並可暫停/繼續。

### 4.1 網頁操作說明

| 區域 | 操作方式 | 說明 |
| --- | --- | --- |
| 設定表單 | 於列數、行數（>=5）與最大掉落輸入值後按「套用設定」 | 立即重新初始化並套用新的棋盤尺寸與掉落上限。 |
| 重新開始 | 點擊「重新開始」 | 使用目前設定重置遊戲狀態。 |
| 暫停/繼續 | 點擊「暫停」或「繼續」 | 僅停止/恢復自動掉落，棋盤資料不變。 |
| 鍵盤 ←/→ | 左右移动 | 呼叫 `_wasm_move_left/right`，對應 `tetris_move()`。 |
| 鍵盤 ↓ | 軟降 | 加速下降並小幅加分；若無法下降會鎖定方塊。 |
| 鍵盤 ↑ 或 X | 順時針旋轉 | 呼叫 `_wasm_rotate_cw`。 |
| 鍵盤 Z | 逆時針旋轉 | 呼叫 `_wasm_rotate_ccw`。 |
| 鍵盤 Space | 硬掉 | 直落到底並依距離加分。 |
| Canvas | 無需直接操作 | 依 WASM 回傳的棋盤內容自動顯示不同顏色。 |

## 5. 部署到 GitHub Pages

1. 重複執行建置與同步指令，確認 `public/` 內容最新：
   ```bash
   ./scripts/build-wasm.sh
   ./scripts/sync-config.sh
   ```
2. 將 `public/`（或複製為 `dist/`）推送至 `gh-pages` 分支。
3. 在 GitHub 的 Repository Settings → Pages 中，選擇 `gh-pages` / `/` 作為來源。
4. 網站即會於數分鐘後上線。

## 6. 常見問題

| 問題 | 解法 |
| --- | --- |
| `emcc: command not found` | 確認已在目前終端機執行 `source ~/emsdk/emsdk_env.sh`。 |
| `python: command not found` | 使用 `python3 -m http.server`。 |
| 設定檔變更後網頁無更新 | 重新執行 `./scripts/sync-config.sh`，再刷新瀏覽器。 |

## 7. 進一步開發建議

- 請參考 `DESIGN.md` 了解整體架構與實作步驟。
- 若要新增功能（例如儲存下一個方塊預覽、加入音效），建議先在 `src/c/` 擴充資料結構，再調整 `public/main.js` 與 UI。

> 完整流程：啟動 emsdk → `./scripts/build-wasm.sh` → `./scripts/sync-config.sh` → `python3 -m http.server` → 確認功能 → 推送 GitHub Pages。
