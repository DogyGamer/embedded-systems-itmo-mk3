/*
 * snake.c - правила игры "Змейка" (см. snake.h).
 */

#include "snake.h"

/* Пауза между шагами змейки: в начале игры медленно, с каждой съеденной едой быстрее */
#define STEP_START_MS     250
#define STEP_MIN_MS       100
#define STEP_SPEEDUP_MS   10

/* Начальная длина змейки */
#define START_LENGTH      3

Game game;

static uint32_t random_seed;

/* Простой генератор случайных чисел: возвращает число от 0 до max-1 */
static uint32_t Random(uint32_t max)
{
    random_seed = random_seed * 1664525u + 1013904223u;
    return (random_seed >> 16) % max;
}

/* Занята ли клетка (x, y) одной из первых count клеток змейки? */
static int IsSnake(int x, int y, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (game.body[i].x == x && game.body[i].y == y)
        {
            return 1;
        }
    }
    return 0;
}

/* Кладем еду в случайную свободную клетку */
static void PlaceFood(void)
{
    do
    {
        game.food.x = Random(FIELD_W);
        game.food.y = Random(FIELD_H);
    } while (IsSnake(game.food.x, game.food.y, game.length));
}

void Game_Init(void)
{
    game.state = GAME_START;
}

void Game_Start(uint32_t now_ms)
{
    int i;

    /* Змейка стоит в центре поля, голова справа, хвост слева */
    game.length = START_LENGTH;
    for (i = 0; i < START_LENGTH; i++)
    {
        game.body[i].x = FIELD_W / 2 - i;
        game.body[i].y = FIELD_H / 2;
    }
    game.dir = DIR_RIGHT;
    game.next_dir = DIR_RIGHT;

    game.score = 0;
    game.start_ms = now_ms;
    game.seconds = 0;

    /* Время нажатия кнопки каждый раз разное - годится для начального числа */
    random_seed = now_ms;
    PlaceFood();

    game.state = GAME_PLAY;
}

int Game_SetDirection(Direction dir)
{
    /* Ехать дальше в ту же сторону поворотом не считается. Развернуться на 180 градусов
       нельзя - змейка врежется сама в себя. Новое направление dir сравниваем с game.dir
       (куда змейка реально двигалась на прошлом шаге), а не с game.next_dir
       (что игрок нажал последним). */
    if (dir == game.dir || dir == (game.dir + 2) % 4)
    {
        return 0;
    }

    game.next_dir = dir;
    return 1;
}

void Game_Step(uint32_t now_ms)
{
    /* Как меняются координаты головы при движении в каждую сторону */
    static const int dx[4] = { 0, 1, 0, -1 };   /* вверх, вправо, вниз, влево */
    static const int dy[4] = { -1, 0, 1, 0 };

    int x, y, i, ate;

    game.seconds = (now_ms - game.start_ms) / 1000;

    /* Куда придет голова */
    game.dir = game.next_dir;
    x = game.body[0].x + dx[game.dir];
    y = game.body[0].y + dy[game.dir];

    /* Врезались в стену? */
    if (x < 0 || x >= FIELD_W || y < 0 || y >= FIELD_H)
    {
        game.state = GAME_OVER;
        return;
    }

    /* Врезались в себя? Хвост не проверяем: на этом шаге он отодвинется. */
    if (IsSnake(x, y, game.length - 1))
    {
        game.state = GAME_OVER;
        return;
    }

    /* Съели еду? Тогда змейка вырастает: хвост остается на месте */
    ate = (x == game.food.x && y == game.food.y);
    if (ate)
    {
        game.score++;
        game.length++;
    }

    /* Сдвигаем змейку: каждая клетка занимает место предыдущей, голова - новое место */
    for (i = game.length - 1; i > 0; i--)
    {
        game.body[i] = game.body[i - 1];
    }
    game.body[0].x = x;
    game.body[0].y = y;

    if (ate)
    {
        if (game.length == MAX_LEN)
        {
            game.state = GAME_OVER;     /* змейка заняла все поле - больше расти некуда */
        }
        else
        {
            PlaceFood();
        }
    }
}

uint32_t Game_StepDelay(void)
{
    uint32_t speedup = game.score * STEP_SPEEDUP_MS;

    if (speedup > STEP_START_MS - STEP_MIN_MS)
    {
        return STEP_MIN_MS;
    }
    return STEP_START_MS - speedup;
}
