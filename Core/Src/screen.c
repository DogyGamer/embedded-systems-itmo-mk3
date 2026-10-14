/*
 * screen.c - рисование игры на OLED-дисплее 128x64.
 *
 * Экран во время игры:
 *
 *   x = 0..90                  x = 92..127
 *   +------------------------+ SCORE
 *   |  игровое поле          | 12
 *   |  22 x 15 клеток        |
 *   |  (клетка 4x4 пикселя)  | TIME
 *   +------------------------+ 01:23
 *
 * Игровое поле нарисовано внутри рамки, а счет и время - правее нее,
 * поэтому змейка никогда не залезет на счет.
 */

#include <string.h>
#include "oled.h"
#include "fonts.h"
#include "snake.h"
#include "screen.h"

#define CELL_SIZE   4       /* размер клетки поля в пикселях (рисуем квадрат 3x3 и оставляем зазор 1 пиксель) */
#define FIELD_X     2       /* где на экране левый верхний угол клетки (0, 0) */
#define FIELD_Y     2
#define FRAME_X2    (FIELD_W * CELL_SIZE + 2)   /* правая граница рамки поля */
#define FRAME_Y2    (FIELD_H * CELL_SIZE + 2)   /* нижняя граница рамки поля */
#define PANEL_X     92      /* левый край панели со счетом и временем */

/* Записывает число в виде текста, например 42 -> "42" */
static void NumberToText(uint32_t number, char *text)
{
    char digits[11];
    int count = 0;
    int i = 0;

    do
    {
        digits[count] = '0' + number % 10;
        count++;
        number = number / 10;
    } while (number > 0);

    while (count > 0)
    {
        count--;
        text[i] = digits[count];
        i++;
    }
    text[i] = 0;
}

/* Записывает время в виде текста "MM:SS", например 83 секунды -> "01:23" */
static void TimeToText(uint32_t seconds, char *text)
{
    uint32_t minutes = seconds / 60;

    if (minutes > 99)
    {
        minutes = 99;
    }
    seconds = seconds % 60;

    text[0] = '0' + minutes / 10;
    text[1] = '0' + minutes % 10;
    text[2] = ':';
    text[3] = '0' + seconds / 10;
    text[4] = '0' + seconds % 10;
    text[5] = 0;
}

/* Пишет текст в точке (x, y) */
static void PrintText(uint8_t x, uint8_t y, char *text, FontDef font)
{
    oled_SetCursor(x, y);
    oled_WriteString(text, font, White);
}

/* Пишет текст по центру экрана на высоте y */
static void PrintCentered(uint8_t y, char *text, FontDef font)
{
    uint8_t width = strlen(text) * font.FontWidth;

    PrintText((OLED_WIDTH - width) / 2, y, text, font);
}

/* Закрашенный квадрат 3x3 в клетке (cx, cy) - кусочек змейки */
static void DrawSnakePart(uint8_t cx, uint8_t cy)
{
    uint8_t x = FIELD_X + cx * CELL_SIZE;
    uint8_t y = FIELD_Y + cy * CELL_SIZE;
    uint8_t row;

    for (row = 0; row < 3; row++)
    {
        oled_DrawHLine(x, x + 2, y + row, White);
    }
}

/* Пустой квадрат 3x3 в клетке (cx, cy) - еда */
static void DrawFood(uint8_t cx, uint8_t cy)
{
    uint8_t x = FIELD_X + cx * CELL_SIZE;
    uint8_t y = FIELD_Y + cy * CELL_SIZE;

    oled_DrawSquare(x, x + 2, y, y + 2, White);
}

static void DrawPlayScreen(const Game *g)
{
    char text[12];
    int i;

    /* Рамка и игровое поле */
    oled_DrawSquare(0, FRAME_X2, 0, FRAME_Y2, White);
    DrawFood(g->food.x, g->food.y);
    for (i = 0; i < g->length; i++)
    {
        DrawSnakePart(g->body[i].x, g->body[i].y);
    }

    /* Панель справа: счет и время игры */
    PrintText(PANEL_X, 2, "SCORE", Font_7x10);
    NumberToText(g->score, text);
    PrintText(PANEL_X, 14, text, Font_11x18);

    PrintText(PANEL_X, 38, "TIME", Font_7x10);
    TimeToText(g->seconds, text);
    PrintText(PANEL_X, 50, text, Font_7x10);
}

static void DrawStartScreen(void)
{
    PrintCentered(2, "SNAKE", Font_16x26);

    PrintCentered(32, "2:UP    8:DOWN", Font_7x10);
    PrintCentered(42, "4:LEFT  6:RIGHT", Font_7x10);
    PrintCentered(53, "PRESS ANY KEY", Font_7x10);
}

static void DrawGameOverScreen(const Game *g)
{
    char number[12];
    char line[24];

    PrintCentered(2, "GAME OVER", Font_11x18);

    NumberToText(g->score, number);
    strcpy(line, "SCORE: ");
    strcat(line, number);
    PrintCentered(24, line, Font_7x10);

    TimeToText(g->seconds, number);
    strcpy(line, "TIME: ");
    strcat(line, number);
    PrintCentered(36, line, Font_7x10);

    PrintCentered(53, "PRESS ANY KEY", Font_7x10);
}

void Screen_Draw(const Game *g)
{
    oled_Fill(Black);

    switch (g->state)
    {
        case GAME_START:
            DrawStartScreen();
            break;
        case GAME_PLAY:
            DrawPlayScreen(g);
            break;
        case GAME_OVER:
            DrawGameOverScreen(g);
            break;
    }
}
