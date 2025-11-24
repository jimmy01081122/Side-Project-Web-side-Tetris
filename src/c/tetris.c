/* 檔案：src/c/tetris.c | 作者：JimmyChang | 日期：2025/11/24
 * 說明：俄羅斯方塊核心邏輯，處理方塊生成、碰撞檢測、旋轉、消除與計分，並保障棋盤最小尺寸
 */

#include "tetris.h"

#include <stdlib.h>
#include <string.h>

#define MIN_SIZE 5

/* 所有 7 種方塊的旋轉定義與顏色 ID，供 runtime 查表 */
static const TetrominoDef kPieces[7] = {
    /* I */ {4, {{{0, 1}, {1, 1}, {2, 1}, {3, 1}},
                 {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
                 {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
                 {{1, 0}, {1, 1}, {1, 2}, {1, 3}}}, 1},
    /* J */ {4, {{{0, 0}, {0, 1}, {1, 1}, {2, 1}},
                 {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
                 {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
                 {{1, 0}, {1, 1}, {0, 2}, {1, 2}}}, 2},
    /* L */ {4, {{{2, 0}, {0, 1}, {1, 1}, {2, 1}},
                 {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
                 {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
                 {{0, 0}, {1, 0}, {1, 1}, {1, 2}}}, 3},
    /* O */ {1, {{{1, 0}, {2, 0}, {1, 1}, {2, 1}},
                 {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
                 {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
                 {{1, 0}, {2, 0}, {1, 1}, {2, 1}}}, 4},
    /* S */ {2, {{{1, 0}, {2, 0}, {0, 1}, {1, 1}},
                 {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
                 {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
                 {{0, 0}, {0, 1}, {1, 1}, {1, 2}}}, 5},
    /* T */ {4, {{{1, 0}, {0, 1}, {1, 1}, {2, 1}},
                 {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
                 {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
                 {{1, 0}, {0, 1}, {1, 1}, {1, 2}}}, 6},
    /* Z */ {2, {{{0, 0}, {1, 0}, {1, 1}, {2, 1}},
                 {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
                 {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
                 {{1, 0}, {0, 1}, {1, 1}, {0, 2}}}, 7}
};

/* LCG 亂數，維持與舊遊戲類似的隨機性即可 */
static uint32_t next_random(GameState *state) {
    state->rng_state = state->rng_state * 1664525u + 1013904223u;
    return state->rng_state;
}

/* bag-of-7 洗牌，避免長時間未出現某種方塊 */
static void shuffle_bag(GameState *state) {
    for (int i = 0; i < 7; ++i) {
        state->bag[i] = i;
    }
    for (int i = 6; i > 0; --i) {
        int j = (int)(next_random(state) % (uint32_t)(i + 1));
        int tmp = state->bag[i];
        state->bag[i] = state->bag[j];
        state->bag[j] = tmp;
    }
    state->bag_pos = 0;
}

/* 從亂數袋中抽出下一個方塊 */
static int draw_piece(GameState *state) {
    if (state->bag_pos >= 7) {
        shuffle_bag(state);
    }
    return state->bag[state->bag_pos++];
}

/* 建立或重新配置棋盤記憶體，並檢查最小尺寸限制 */
static int ensure_board(GameState *state, int rows, int cols) {
    if (rows < MIN_SIZE || cols < MIN_SIZE) {
        return 0;
    }
    size_t cell_count = (size_t)rows * (size_t)cols;
    int *buffer = (int *)calloc(cell_count, sizeof(int));
    if (!buffer) {
        return 0;
    }
    free(state->board);
    state->board = buffer;
    state->rows = rows;
    state->cols = cols;
    return 1;
}

/* 計算棋盤一維陣列索引 */
static int cell_index(const GameState *state, int row, int col) {
    return row * state->cols + col;
}

/* 將目前方塊投影到棋盤座標，用於碰撞與繪製 */
static void project_cells(const GameState *state, const ActivePiece *piece, int row_off, int col_off, Cell out[CELLS_PER_PIECE]) {
    const TetrominoDef *def = &kPieces[piece->type_index];
    int rotation = piece->rotation % def->rotation_count;
    if (rotation < 0) {
        rotation += def->rotation_count;
    }
    for (int i = 0; i < CELLS_PER_PIECE; ++i) {
        out[i].x = def->rotations[rotation][i].x + piece->col + col_off;
        out[i].y = def->rotations[rotation][i].y + piece->row + row_off;
    }
}

/* 檢查指定位移後是否碰到牆或既有方塊 */
static int collides(const GameState *state, const ActivePiece *piece, int row_off, int col_off) {
    Cell cells[CELLS_PER_PIECE];
    project_cells(state, piece, row_off, col_off, cells);
    for (int i = 0; i < CELLS_PER_PIECE; ++i) {
        int x = cells[i].x;
        int y = cells[i].y;
        if (x < 0 || x >= state->cols || y < 0 || y >= state->rows) {
            return 1;
        }
        int idx = cell_index(state, y, x);
        if (state->board[idx] != 0) {
            return 1;
        }
    }
    return 0;
}

/* 讓新方塊從中央稍上方生成 */
static void set_spawn_position(const GameState *state, ActivePiece *piece) {
    piece->row = 0;
    piece->col = state->cols / 2 - 2;
    if (piece->col < 0) {
        piece->col = 0;
    }
    piece->rotation = 0;
}

/* 將 next 變成 current，再抽出新的 next；若生成即碰撞則遊戲結束 */
static int spawn_next_piece(GameState *state) {
    state->current = state->next;
    set_spawn_position(state, &state->current);
    state->next.type_index = draw_piece(state);
    state->next.rotation = 0;
    state->next.row = 0;
    state->next.col = 0;
    if (collides(state, &state->current, 0, 0)) {
        state->is_game_over = 1;
        return 0;
    }
    return 1;
}

/* 從底往上檢查並清除已滿列，回傳清除數以計分 */
static int clear_lines(GameState *state) {
    int cleared = 0;
    for (int row = state->rows - 1; row >= 0; --row) {
        int filled = 1;
        for (int col = 0; col < state->cols; ++col) {
            if (state->board[cell_index(state, row, col)] == 0) {
                filled = 0;
                break;
            }
        }
        if (filled) {
            ++cleared;
            for (int move_row = row; move_row > 0; --move_row) {
                memcpy(&state->board[cell_index(state, move_row, 0)],
                       &state->board[cell_index(state, move_row - 1, 0)],
                       (size_t)state->cols * sizeof(int));
            }
            memset(&state->board[cell_index(state, 0, 0)], 0, (size_t)state->cols * sizeof(int));
            ++row; /* re-evaluate current row after shifting */
        }
    }
    return cleared;
}

/* 將方塊寫入棋盤、更新統計並檢查掉落上限 */
static void lock_piece(GameState *state) {
    const TetrominoDef *def = &kPieces[state->current.type_index];
    Cell cells[CELLS_PER_PIECE];
    project_cells(state, &state->current, 0, 0, cells);
    for (int i = 0; i < CELLS_PER_PIECE; ++i) {
        int idx = cell_index(state, cells[i].y, cells[i].x);
        state->board[idx] = def->color_id;
    }
    ++state->drop_count;
    int cleared = clear_lines(state);
    state->lines_cleared += cleared;
    if (cleared > 0) {
        state->score += cleared * 100;
    }
    if (state->max_drops > 0 && state->drop_count >= state->max_drops) {
        state->is_game_over = 1;
        return;
    }
    spawn_next_piece(state);
}

/* 初始化遊戲狀態，並確保棋盤尺寸與掉落上限有效 */
int tetris_init(GameState *state, int rows, int cols, int max_drops) {
    tetris_free(state);
    memset(state, 0, sizeof(*state));
    state->rng_state = 0xC0FFEEu;
    if (max_drops < 1) {
        max_drops = 1;
    }
    if (!ensure_board(state, rows, cols)) {
        return 0;
    }
    state->max_drops = max_drops;
    shuffle_bag(state);
    state->next.type_index = draw_piece(state);
    state->next.rotation = 0;
    state->next.row = 0;
    state->next.col = 0;
    return spawn_next_piece(state);
}

/* 清除配置的棋盤記憶體，供程式結束或重設時呼叫 */
void tetris_free(GameState *state) {
    free(state->board);
    state->board = NULL;
}

/* 重新初始化（語意上同 init，但保留函式介面） */
int tetris_reset(GameState *state, int rows, int cols, int max_drops) {
    return tetris_init(state, rows, cols, max_drops);
}

/* 允許前端指定種子，方便測試或重現 */
void tetris_set_seed(GameState *state, uint32_t seed) {
    if (seed == 0) {
        seed = 0xC0FFEEu;
    }
    state->rng_state = seed;
    shuffle_bag(state);
}

/* 遊戲主循環每次觸發一格重力，若落到底則鎖定方塊 */
int tetris_tick(GameState *state) {
    if (state->is_game_over) {
        return 0;
    }
    if (!tetris_move(state, 0, 1)) {
        lock_piece(state);
        return 0;
    }
    return 1;
}

/* 泛用移動函式，dx/dy 可為正負一，若碰撞則返回 0 */
int tetris_move(GameState *state, int dx, int dy) {
    if (state->is_game_over) {
        return 0;
    }
    ActivePiece trial = state->current;
    trial.col += dx;
    trial.row += dy;
    if (collides(state, &trial, 0, 0)) {
        return 0;
    }
    state->current = trial;
    return 1;
}

/* 軟降：若能下移就加分，不能則鎖定 */
int tetris_soft_drop(GameState *state) {
    if (tetris_move(state, 0, 1)) {
        state->score += 1;
        return 1;
    }
    lock_piece(state);
    return 0;
}

/* 硬降：一路往下直到碰撞，根據移動步數加倍計分 */
int tetris_hard_drop(GameState *state) {
    if (state->is_game_over) {
        return 0;
    }
    int steps = 0;
    while (tetris_move(state, 0, 1)) {
        ++steps;
    }
    state->score += steps * 2;
    lock_piece(state);
    return steps;
}

/* 嘗試旋轉，並加入左右平移的簡易踢牆邏輯 */
int tetris_rotate(GameState *state, int clockwise) {
    if (state->is_game_over) {
        return 0;
    }
    ActivePiece trial = state->current;
    const TetrominoDef *def = &kPieces[trial.type_index];
    int delta = clockwise ? 1 : -1;
    int rotation = trial.rotation + delta;
    rotation %= def->rotation_count;
    if (rotation < 0) {
        rotation += def->rotation_count;
    }
    trial.rotation = rotation;
    int kicks[] = {0, -1, 1, -2, 2};
    for (size_t i = 0; i < sizeof kicks / sizeof kicks[0]; ++i) {
        ActivePiece shifted = trial;
        shifted.col = state->current.col + kicks[i];
        if (!collides(state, &shifted, 0, 0)) {
            state->current = shifted;
            return 1;
        }
    }
    return 0;
}

/* 提供目前方塊的實際座標給 WASM 桥接層 */
int tetris_active_cells(const GameState *state, Cell out[CELLS_PER_PIECE]) {
    if (!out) {
        return 0;
    }
    project_cells(state, &state->current, 0, 0, out);
    return 1;
}

/* 回傳目前方塊的顏色 ID */
int tetris_active_color(const GameState *state) {
    return kPieces[state->current.type_index].color_id;
}

const int *tetris_board(const GameState *state) {
    return state->board;
}

int tetris_rows(const GameState *state) {
    return state->rows;
}

int tetris_cols(const GameState *state) {
    return state->cols;
}

int tetris_score(const GameState *state) {
    return state->score;
}

int tetris_lines(const GameState *state) {
    return state->lines_cleared;
}

int tetris_drop_cap(const GameState *state) {
    return state->max_drops;
}

int tetris_drop_count(const GameState *state) {
    return state->drop_count;
}

int tetris_is_over(const GameState *state) {
    return state->is_game_over;
}
