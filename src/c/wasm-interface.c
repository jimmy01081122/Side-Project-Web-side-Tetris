/* 檔案：src/c/wasm-interface.c | 作者：JimmyChang | 日期：2025/11/24
 * 說明：將 C 端遊戲邏輯透過 EMSCRIPTEN_KEEPALIVE 導出，供 JavaScript 呼叫
 */

#include <stdint.h>

#include <emscripten/emscripten.h>

#include "tetris.h"

#define WASM_MIN_SIZE 5  /* 再次保護傳入值，避免 JS 偶然送入過小棋盤 */

static GameState g_state;

/* 初始化遊戲，若行列過小則自動調整到最小值 */
EMSCRIPTEN_KEEPALIVE int wasm_init_game(int rows, int cols, int max_drops) {
    if (rows < WASM_MIN_SIZE) {
        rows = WASM_MIN_SIZE;
    }
    if (cols < WASM_MIN_SIZE) {
        cols = WASM_MIN_SIZE;
    }
    return tetris_init(&g_state, rows, cols, max_drops);
}

/* 重新套用設定（表單送出時使用） */
EMSCRIPTEN_KEEPALIVE int wasm_reset_game(int rows, int cols, int max_drops) {
    return tetris_reset(&g_state, rows, cols, max_drops);
}

/* 讓前端可設定隨機種子，以利除錯或固定關卡順序 */
EMSCRIPTEN_KEEPALIVE void wasm_set_seed(uint32_t seed) {
    tetris_set_seed(&g_state, seed);
}

/* 定時呼叫，推進一格重力 */
EMSCRIPTEN_KEEPALIVE int wasm_tick(void) {
    return tetris_tick(&g_state);
}

/* 泛用移動封裝，方便左右移與軟降 */
EMSCRIPTEN_KEEPALIVE int wasm_move(int dx, int dy) {
    return tetris_move(&g_state, dx, dy);
}

EMSCRIPTEN_KEEPALIVE int wasm_move_left(void) {
    return wasm_move(-1, 0);
}

EMSCRIPTEN_KEEPALIVE int wasm_move_right(void) {
    return wasm_move(1, 0);
}

EMSCRIPTEN_KEEPALIVE int wasm_soft_drop(void) {
    return tetris_soft_drop(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_hard_drop(void) {
    return tetris_hard_drop(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_rotate_cw(void) {
    return tetris_rotate(&g_state, 1);
}

EMSCRIPTEN_KEEPALIVE int wasm_rotate_ccw(void) {
    return tetris_rotate(&g_state, 0);
}

/* 以下 getter 供前端讀取遊戲狀態 */
EMSCRIPTEN_KEEPALIVE int wasm_get_board_ptr(void) {
    return (int)(intptr_t)tetris_board(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_rows(void) {
    return tetris_rows(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_cols(void) {
    return tetris_cols(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_score(void) {
    return tetris_score(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_lines(void) {
    return tetris_lines(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_drop_cap(void) {
    return tetris_drop_cap(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_get_drop_count(void) {
    return tetris_drop_count(&g_state);
}

EMSCRIPTEN_KEEPALIVE int wasm_is_game_over(void) {
    return tetris_is_over(&g_state);
}

/* 將目前方塊的四個格座標寫入指定緩衝區（JS 透過 _malloc 取得） */
EMSCRIPTEN_KEEPALIVE void wasm_write_active_cells(int ptr) {
    if (ptr == 0) {
        return;
    }
    Cell cells[CELLS_PER_PIECE];
    if (!tetris_active_cells(&g_state, cells)) {
        return;
    }
    int *buffer = (int *)ptr;
    for (int i = 0; i < CELLS_PER_PIECE; ++i) {
        buffer[i * 2] = cells[i].x;
        buffer[i * 2 + 1] = cells[i].y;
    }
}

/* 回傳目前方塊顏色，用於 Canvas 著色 */
EMSCRIPTEN_KEEPALIVE int wasm_get_active_color(void) {
    return tetris_active_color(&g_state);
}
