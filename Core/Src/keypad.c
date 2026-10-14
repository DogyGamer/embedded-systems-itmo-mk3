/*
 * keypad.c - опрос матричной клавиатуры 3x4.
 *
 * Построен на готовой функции Check_Row() из примера SDK_Keyboard
 * (опрашивает одну строку клавиатуры через расширитель PCA9538).
 * Опрос по строкам - так же, как в Keypad_Scan() из SDK_Keyboard/Src/calc.c.
 */

#include "main.h"
#include "kb.h"
#include "keypad.h"

int8_t Keypad_Scan(void)
{
    static const uint8_t rows[4] = { ROW1, ROW2, ROW3, ROW4 };
    uint8_t row;
    uint8_t columns;

    for (row = 0; row < 4; row++)
    {
        columns = Check_Row(rows[row]);

        if (columns == 0x04)        /* нажата кнопка в левом столбце */
        {
            return row * 3;
        }
        if (columns == 0x02)        /* в среднем */
        {
            return row * 3 + 1;
        }
        if (columns == 0x01)        /* в правом */
        {
            return row * 3 + 2;
        }
    }
    return KEY_NONE;
}
