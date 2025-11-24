// 檔案：public/main.js | 作者：JimmyChang | 日期：2025/11/24
// 說明：負責載入設定、與 WebAssembly 溝通、繪製棋盤並處理玩家輸入

const COLORS = [
  '#020617',
  '#38bdf8',
  '#2563eb',
  '#f97316',
  '#facc15',
  '#22c55e',
  '#a855f7',
  '#ef4444'
];

// state 保存 WASM 模組與畫面相關資訊，方便跨函式使用
const state = {
  module: null,
  boardPtr: 0,
  rows: 0,
  cols: 0,
  running: false,
  tickHandle: null,
  tickRate: 700,
  initialized: false,
  activeBufferPtr: 0,
  activeHeapIndex: 0
};

// 快速取得所有會用到的 DOM 節點
const dom = {
  form: document.getElementById('settings-form'),
  rows: document.getElementById('rows-input'),
  cols: document.getElementById('cols-input'),
  drops: document.getElementById('drops-input'),
  resetBtn: document.getElementById('reset-btn'),
  pauseBtn: document.getElementById('pause-btn'),
  score: document.getElementById('score'),
  lines: document.getElementById('lines'),
  dropsLeft: document.getElementById('drops-left'),
  status: document.getElementById('status'),
  canvas: document.getElementById('board'),
  ctx: document.getElementById('board').getContext('2d')
};

// 從 public/config 讀取 JSON 設定，若失敗則套用預設值
async function loadConfig() {
  try {
    const response = await fetch('./config/game-settings.json');
    if (!response.ok) {
      throw new Error('設定檔讀取失敗');
    }
    return await response.json();
  } catch (err) {
    console.warn(err);
    return {
      defaultRows: 20,
      defaultCols: 10,
      defaultMaxDrops: 150,
      minSize: 5,
      maxRows: 40,
      maxCols: 20
    };
  }
}

// clamp 幫助我們把輸入值限制在合法區間
function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

// 預先填入設定表單，可從 localStorage 回復玩家上次的選擇
function hydrateForm(config) {
  const saved = JSON.parse(localStorage.getItem('tetris-settings') || 'null');
  const rows = saved?.rows ?? config.defaultRows;
  const cols = saved?.cols ?? config.defaultCols;
  const drops = saved?.drops ?? config.defaultMaxDrops;
  dom.rows.value = rows;
  dom.cols.value = cols;
  dom.drops.value = drops;
}

// 將表單輸入轉為數字並進行最小/最大值驗證
function readSettings(config) {
  const rows = clamp(parseInt(dom.rows.value, 10) || config.minSize, config.minSize, config.maxRows);
  const cols = clamp(parseInt(dom.cols.value, 10) || config.minSize, config.minSize, config.maxCols);
  const drops = Math.max(1, parseInt(dom.drops.value, 10) || config.defaultMaxDrops);
  return { rows, cols, drops };
}

function saveSettings(settings) {
  localStorage.setItem('tetris-settings', JSON.stringify(settings));
}

// 建立傳回作用中方塊座標的緩衝區，只需配置一次
function ensureActiveBuffer() {
  if (!state.module || state.activeBufferPtr) {
    return;
  }
  const bytes = Int32Array.BYTES_PER_ELEMENT * 8;
  state.activeBufferPtr = state.module._malloc(bytes);
  state.activeHeapIndex = state.activeBufferPtr >> 2;
}

// 每次重新初始化/重設後都要更新棋盤指標與尺寸
function updateBoardPointer() {
  state.boardPtr = state.module._wasm_get_board_ptr();
  state.rows = state.module._wasm_get_rows();
  state.cols = state.module._wasm_get_cols();
}

// 從 WASM 記憶體讀取棋盤資料並以 Canvas 繪製
function drawBoard() {
  if (!state.module || !state.boardPtr) {
    return;
  }
  const ctx = dom.ctx;
  const rows = state.rows;
  const cols = state.cols;
  const cellSize = Math.floor(Math.min(28, 520 / Math.max(rows, cols)));
  dom.canvas.width = cols * cellSize;
  dom.canvas.height = rows * cellSize;
  ctx.fillStyle = '#020617';
  ctx.fillRect(0, 0, dom.canvas.width, dom.canvas.height);
  const heap = state.module.HEAP32;
  const start = state.boardPtr >> 2;
  for (let row = 0; row < rows; row += 1) {
    for (let col = 0; col < cols; col += 1) {
      const colorId = heap[start + row * cols + col] ?? 0;
      const color = COLORS[colorId] || COLORS[0];
      if (colorId > 0) {
        ctx.fillStyle = color;
        ctx.fillRect(col * cellSize, row * cellSize, cellSize - 1, cellSize - 1);
      } else {
        ctx.strokeStyle = '#1e293b';
        ctx.strokeRect(col * cellSize, row * cellSize, cellSize, cellSize);
      }
    }
  }
  if (state.activeBufferPtr) { // 再畫上目前可操作的方塊
    state.module._wasm_write_active_cells(state.activeBufferPtr);
    const colorId = state.module._wasm_get_active_color();
    const base = state.activeHeapIndex;
    ctx.fillStyle = COLORS[colorId] || '#e11d48';
    for (let i = 0; i < 4; i += 1) {
      const x = heap[base + i * 2];
      const y = heap[base + i * 2 + 1];
      ctx.fillRect(x * cellSize, y * cellSize, cellSize - 1, cellSize - 1);
    }
  }
}

// HUD 顯示分數、消除列與剩餘掉落
function updateHud() {
  dom.score.textContent = state.module._wasm_get_score();
  dom.lines.textContent = state.module._wasm_get_lines();
  const cap = state.module._wasm_get_drop_cap();
  const used = state.module._wasm_get_drop_count();
  dom.dropsLeft.textContent = String(Math.max(cap - used, 0));
}

// 變更狀態文字，可選擇高亮（紅色）顯示錯誤
function setStatus(text, highlight = false) {
  dom.status.textContent = text;
  dom.status.style.color = highlight ? '#f87171' : '#facc15';
}

// 停止計時器與更新狀態
function stopLoop(message) {
  if (state.tickHandle) {
    clearInterval(state.tickHandle);
    state.tickHandle = null;
  }
  state.running = false;
  dom.pauseBtn.textContent = '繼續';
  if (message) {
    setStatus(message, true);
  }
}

// 啟動遊戲重力迴圈，每 700ms 觸發一次 tick
function startLoop() {
  if (state.tickHandle || !state.module) {
    return;
  }
  state.tickHandle = setInterval(() => {
    state.module._wasm_tick();
    drawBoard();
    updateHud();
    if (state.module._wasm_is_game_over()) {
      stopLoop('遊戲結束');
    }
  }, state.tickRate);
  state.running = true;
  dom.pauseBtn.textContent = '暫停';
  setStatus('進行中');
}

// 只有在非 Game Over 且尚未運行時才允許繼續
function resumeIfPossible() {
  if (!state.running && !state.module._wasm_is_game_over()) {
    startLoop();
  }
}

// 鍵盤控制：箭頭移動、Z/X 旋轉、Space 硬降
function handleKey(event) {
  if (!state.module) {
    return;
  }
  const code = event.code;
  let handled = false;
  switch (code) {
    case 'ArrowLeft':
      state.module._wasm_move_left();
      handled = true;
      break;
    case 'ArrowRight':
      state.module._wasm_move_right();
      handled = true;
      break;
    case 'ArrowDown':
      state.module._wasm_soft_drop();
      handled = true;
      break;
    case 'ArrowUp':
      state.module._wasm_rotate_cw();
      handled = true;
      break;
    case 'Space':
      state.module._wasm_hard_drop();
      handled = true;
      break;
    case 'KeyZ':
      state.module._wasm_rotate_ccw();
      handled = true;
      break;
    case 'KeyX':
      state.module._wasm_rotate_cw();
      handled = true;
      break;
    default:
      break;
  }
  if (handled) {
    event.preventDefault();
    drawBoard();
    updateHud();
  }
}

// 表單送出或按下重新開始時呼叫，重新套用設定
function applySettings(config) {
  const settings = readSettings(config);
  saveSettings(settings);
  const success = state.initialized
    ? state.module._wasm_reset_game(settings.rows, settings.cols, settings.drops)
    : state.module._wasm_init_game(settings.rows, settings.cols, settings.drops);
  if (!success) {
    setStatus('初始化失敗，請檢查設定', true);
    return;
  }
  state.initialized = true;
  updateBoardPointer();
  ensureActiveBuffer();
  drawBoard();
  updateHud();
  stopLoop();
  startLoop();
}

// 初始化流程：讀取設定、綁定事件、載入 WASM
async function bootstrap() {
  const config = await loadConfig();
  hydrateForm(config);
  dom.form.addEventListener('submit', (event) => {
    event.preventDefault();
    applySettings(config);
  });
  dom.resetBtn.addEventListener('click', () => applySettings(config));
  dom.pauseBtn.addEventListener('click', () => {
    if (!state.initialized) {
      return;
    }
    if (state.running) {
      stopLoop('已暫停');
    } else {
      resumeIfPossible();
    }
  });
  window.addEventListener('keydown', handleKey);
  if (typeof createTetrisModule !== 'function') {
    setStatus('尚未編譯 WebAssembly 模組', true);
    return;
  }
  state.module = await createTetrisModule();
  applySettings(config);
}

bootstrap();
