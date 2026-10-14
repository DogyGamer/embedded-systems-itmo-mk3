/*
 * screen.h - рисование игры на OLED-дисплее 128x64.
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "snake.h"

/* Рисует экран, подходящий текущему состоянию игры (старт, игра или конец игры)
   в буфер дисплея. Чтобы картинка появилась на дисплее, после этого нужно
   вызвать oled_UpdateScreen(). */
void Screen_Draw(const Game *g);

#endif /* SCREEN_H */
