/*
 * keypad.h - опрос матричной клавиатуры 3x4 стенда SDK-1.1M.
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

#define KEY_NONE    (-1)    /* ничего не нажато */

/* Номера кнопок (по порядку слева направо, сверху вниз):
 *
 *      0   1   2
 *      3   4   5
 *      6   7   8
 *      9  10  11
 */
#define KEY_UP      1       /* кнопка "2" */
#define KEY_LEFT    3       /* кнопка "4" */
#define KEY_RIGHT   5       /* кнопка "6" */
#define KEY_DOWN    7       /* кнопка "8" */

/* Возвращает номер нажатой кнопки (0..11) или KEY_NONE.
   Работает по шине I2C, поэтому вызывать ее нужно под мьютексом шины. */
int8_t Keypad_Scan(void);

#endif /* KEYPAD_H */
