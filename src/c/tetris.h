/* 檔案：src/c/tetris.h | 作者：JimmyChang | 日期：2025/11/24
 * 說明：定義俄羅斯方塊核心所需之資料結構與函式介面，供遊戲邏輯與 WASM 橋接層共用
 */

#ifndef TETRIS_H
#define TETRIS_H

#include <stdint.h>

/* Cell 紀錄單一方塊格子的相對座標（x 為列內位置、y 為列數） */
typedef struct {
    int x;
    int y;
} Cell;

#define CELLS_PER_PIECE 4
#define MAX_ROTATIONS 4

/* TetrominoDef 描述每種方塊的所有旋轉狀態，以及對應顏色 ID（供前端著色） */
typedef struct {
    int rotation_count;
    Cell rotations[MAX_ROTATIONS][CELLS_PER_PIECE];
    int color_id;
} TetrominoDef;

/* ActivePiece 表示目前控制中的方塊種類、旋轉角度與棋盤位置 */
typedef struct {
    int type_index;
    int rotation;
    int row;
    int col;
} ActivePiece;

/* GameState 儲存整體棋盤資訊、統計數值與隨機袋狀態 */
typedef struct {
    int rows;
    int cols;
    int max_drops;
    int drop_count;
    int score;
    int lines_cleared;
    int is_game_over;
    int *board;            /* rows * cols entries */
    uint32_t rng_state;
    int bag[7];
    int bag_pos;
    ActivePiece current;
    ActivePiece next;
} GameState;

/* 初始化、資源釋放與重設相關操作 */
int tetris_init(GameState *state, int rows, int cols, int max_drops);
void tetris_free(GameState *state);
int tetris_reset(GameState *state, int rows, int cols, int max_drops);
void tetris_set_seed(GameState *state, uint32_t seed);

/* 遊戲推進與玩家操作 */
int tetris_tick(GameState *state);
int tetris_move(GameState *state, int dx, int dy);
int tetris_soft_drop(GameState *state);
int tetris_hard_drop(GameState *state);
int tetris_rotate(GameState *state, int clockwise);
int tetris_active_cells(const GameState *state, Cell out[CELLS_PER_PIECE]);
int tetris_active_color(const GameState *state);

/* 給前端/橋接層查詢用的存取器 */
const int *tetris_board(const GameState *state);
int tetris_rows(const GameState *state);
int tetris_cols(const GameState *state);
int tetris_score(const GameState *state);
int tetris_lines(const GameState *state);
int tetris_drop_cap(const GameState *state);
int tetris_drop_count(const GameState *state);
int tetris_is_over(const GameState *state);

#endif /* TETRIS_H */
