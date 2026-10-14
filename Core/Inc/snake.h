/*
 * snake.h - правила игры "Змейка".
 *
 * Здесь только логика игры: где змейка, где еда, сколько очков.
 * Ничего про экран, кнопки и FreeRTOS этот модуль не знает.
 */

#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>

/* Размер игрового поля в клетках */
#define FIELD_W   22
#define FIELD_H   15

/* Змейка не может быть длиннее, чем все поле */
#define MAX_LEN   (FIELD_W * FIELD_H)

typedef enum {
    GAME_START,     /* стартовый экран, ждем нажатия кнопки */
    GAME_PLAY,      /* идет игра */
    GAME_OVER       /* экран конца игры */
} GameState;

/* Порядок важен: противоположное направление = (dir + 2) % 4 */
typedef enum {
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} Direction;

/* Координаты клетки на поле: x = 0..FIELD_W-1, y = 0..FIELD_H-1 */
typedef struct {
    uint8_t x;
    uint8_t y;
} Cell;

typedef struct {
    GameState state;
    Cell      body[MAX_LEN];    /* клетки змейки, body[0] - голова */
    uint16_t  length;           /* сколько клеток в змейке */
    Direction dir;              /* куда змейка двигалась на последнем шаге */
    Direction next_dir;         /* куда игрок хочет повернуть на следующем шаге */
    Cell      food;             /* где лежит еда */
    uint16_t  score;            /* сколько еды съедено */
    uint32_t  start_ms;         /* когда началась игра */
    uint32_t  seconds;          /* сколько секунд идет игра */
} Game;

/* Единственная игра в программе. Меняет ее только Task_Game, а читает Task_Display.
   Чтобы Task_Display не прочитала змейку посреди шага, Task_Game меняет game
   под мьютексом gameMutex (см. freertos.c). */
extern Game game;

void     Game_Init(void);                   /* показать стартовый экран */
void     Game_Start(uint32_t now_ms);       /* начать новую игру */
int      Game_SetDirection(Direction dir);  /* игрок нажал кнопку поворота; 1 - змейка повернет, 0 - кнопка ничего не изменила */
void     Game_Step(uint32_t now_ms);        /* сдвинуть змейку на одну клетку */
uint32_t Game_StepDelay(void);              /* пауза до следующего шага, мс */

#endif /* SNAKE_H */
