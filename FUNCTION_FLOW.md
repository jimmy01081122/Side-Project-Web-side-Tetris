<!-- 檔案：FUNCTION_FLOW.md | 作者：JimmyChang | 日期：2025/11/24 -->
# 函式調度關係與工作內容視覺化說明

> 目標：讓剛接觸專案的人也能理解「按下按鍵後」系統內部誰叫誰、每個函式要做什麼。

## 1. 模組分層圖

```
┌────────────┐     ┌──────────────────────┐     ┌────────────────┐
│  HTML/CSS  │ --> │ public/main.js       │ --> │ WebAssembly    │
│  (UI)      │     │  (瀏覽器邏輯)        │     │ 模組 (C 語言)   │
└────────────┘     └──────────────────────┘     └────────────────┘
                                           │
                                           ▼
                                      玩家可見畫面
```
- HTML/CSS：提供設定表單、按鈕、Canvas 容器。
- `public/main.js`：負責載入設定、處理鍵盤/按鈕、畫面刷新。
- C + WASM：計算方塊位置、判斷碰撞、更新分數。

## 2. 組件互動流程

### 2.1 初始化（載入頁面）

```
bootstrap()
  ├─ loadConfig()             # 讀取 JSON 預設值
  ├─ hydrateForm()            # 將預設值填入表單
  ├─ 綁定 form/pause/reset/keydown 事件
  └─ createTetrisModule()     # 產生 WASM 模組
      └─ wasm_init_game()     # C 端：tetris_init()
          ├─ ensure_board()
          ├─ shuffle_bag()
          └─ spawn_next_piece()
```

### 2.2 每次遊戲迴圈（startLoop → setInterval）

```
startLoop()
  └─ setInterval(700ms)
       ├─ wasm_tick()            # C：tetris_tick()
       │    ├─ tetris_move() 或 lock_piece()
       │    └─ clear_lines() / spawn_next_piece()
       ├─ drawBoard()
       │    ├─ 讀取 HEAP32 棋盤資料
       │    └─ wasm_write_active_cells() + wasm_get_active_color()
       └─ updateHud()
            ├─ wasm_get_score()
            ├─ wasm_get_lines()
            └─ wasm_get_drop_cap()/wasm_get_drop_count()
```

### 2.3 鍵盤/按鈕事件

| 使用者操作 | JavaScript 函式 | 對應 WASM API | C 端邏輯 |
| --- | --- | --- | --- |
| ← → | `handleKey` → `_wasm_move_left/right` | `wasm_move()` | `tetris_move()` 檢查碰撞後更新位置 |
| ↓   | `_wasm_soft_drop` | `tetris_soft_drop()` | 能下移則加 1 分，否則鎖定 |
| ↑ / X | `_wasm_rotate_cw` | `tetris_rotate()` | 嘗試旋轉並套用踢牆 |
| Z   | `_wasm_rotate_ccw` | `tetris_rotate(...clockwise=0)` | 逆時針旋轉 |
| Space | `_wasm_hard_drop` | `tetris_hard_drop()` | 直落到底並依距離加分 |
| 暫停鍵 | `stopLoop()` | — | 僅停止 JS 計時器 |
| 套用設定 | `applySettings()` → `_wasm_reset_game` | `tetris_reset()` | 重建棋盤與隨機袋 |

## 3. C 函式呼叫圖（依功能分群）

```
初始化/重設：
  tetris_init()
    ├─ tetris_free()
    ├─ ensure_board()
    ├─ shuffle_bag()
    └─ spawn_next_piece()

落子流程：
  tetris_tick()
    └─ tetris_move(0,1)
         └─ 若失敗 → lock_piece()
              ├─ clear_lines()
              └─ spawn_next_piece()

玩家操作：
  tetris_move(dx,dy)
  tetris_soft_drop()
  tetris_hard_drop()
  tetris_rotate(clockwise)
    └─ collides() / project_cells()

查詢資料：
  tetris_board()/rows()/cols()/score()/lines()/drop_cap()/drop_count()
  tetris_active_cells()
  tetris_active_color()
```

## 4. JavaScript 函式地圖

```
bootstrap()
  ├─ loadConfig()
  ├─ hydrateForm()
  ├─ applySettings()
        ├─ readSettings()
        ├─ saveSettings()
        ├─ wasm_init/reset
        ├─ updateBoardPointer()
        ├─ ensureActiveBuffer()
        ├─ drawBoard()
        └─ updateHud()
  ├─ event listeners (form / buttons / keydown)
  └─ startLoop()
```

- `drawBoard()` 與 `updateHud()` 只讀取 WASM，沒有寫入。
- `stopLoop()` 與 `resumeIfPossible()` 讓玩家控制遊戲速度。
- `ensureActiveBuffer()` 只會在需要時向 WASM 要記憶體一次，避免重複配置。

## 5. 常見情境解說

1. **玩家按下左鍵**
   - `handleKey()` 捕捉事件 → 呼叫 `_wasm_move_left()` → C 端 `tetris_move(-1,0)` 檢查碰撞 → 回傳成功 → JS 重新繪製 → 玩家看到方塊左移。
2. **方塊落到底部**
   - `tetris_move(0,1)` 回傳 0 → `lock_piece()` 將格子寫入棋盤，並增加 `drop_count` → `clear_lines()` 如有清除則加分 → `spawn_next_piece()` 產生新方塊或宣告結束。
3. **玩家修改設定並重新開始**
   - 表單送出 → `applySettings()` 讀取數值 → `_wasm_reset_game(rows, cols, drops)` → `tetris_reset()` 重新配置棋盤 → JS 更新 Canvas 尺寸並自動開始新的計時迴圈。

---
透過以上圖表與說明，即使是初學者也能沿著流程理解整體函式如何串接，並知道要在哪個層級修改或除錯。
