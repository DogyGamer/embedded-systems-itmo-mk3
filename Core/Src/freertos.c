/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "keypad.h"
#include "snake.h"
#include "screen.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/*
 * Как устроена игра "Змейка"
 *
 *   Task_Input --keyQueue--> Task_Game --redrawSem--> Task_Display
 *   (клавиатура)             (правила игры)           (экран)
 *
 * Task_Input   каждые 20 мс опрашивает клавиатуру и кладет номера нажатых кнопок в очередь.
 * Task_Game    двигает змейку, проверяет столкновения, считает очки и время.
 * Task_Display рисует игру и отправляет картинку на OLED-дисплей.
 *
 * Общие ресурсы, которые нельзя трогать сразу из двух задач:
 *   i2cMutex  - шина I2C. Через нее работают и дисплей, и клавиатура.
 *   gameMutex - структура game. Task_Game меняет ее, а Task_Display читает.
 *               Меняет game только Task_Game, поэтому свои же поля он читает без мьютекса.
 */
osThreadId Task_InputHandle;
osThreadId Task_GameHandle;
osThreadId Task_DisplayHandle;

osMessageQId keyQueueHandle;
osMutexId i2cMutexHandle;
osMutexId gameMutexHandle;
osSemaphoreId redrawSemHandle;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartTaskInput(void const * argument);
void StartTaskGame(void const * argument);
void StartTaskDisplay(void const * argument);
/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* Мьютекс шины I2C: дисплей и клавиатура работают через одну шину */
  osMutexDef(i2cMutex);
  i2cMutexHandle = osMutexCreate(osMutex(i2cMutex));

  /* Мьютекс данных игры: Task_Game пишет, Task_Display читает */
  osMutexDef(gameMutex);
  gameMutexHandle = osMutexCreate(osMutex(gameMutex));
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* Двоичный семафор "экран нужно перерисовать". Сразу после создания он свободен,
     поэтому Task_Display нарисует стартовый экран без ожидания */
  osSemaphoreDef(redrawSem);
  redrawSemHandle = osSemaphoreCreate(osSemaphore(redrawSem), 1);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* Очередь нажатых кнопок: до 4 номеров кнопок */
  osMessageQDef(keyQueue, 4, uint32_t);
  keyQueueHandle = osMessageCreate(osMessageQ(keyQueue), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* USER CODE BEGIN RTOS_THREADS */
  /* Приоритеты: клавиатура важнее всего (чтобы не пропустить нажатие),
     дисплей - наименее важная задача. Размер стека задан в словах (1 слово = 4 байта). */
  osThreadDef(Task_Input, StartTaskInput, osPriorityAboveNormal, 0, 256);
  Task_InputHandle = osThreadCreate(osThread(Task_Input), NULL);

  osThreadDef(Task_Game, StartTaskGame, osPriorityNormal, 0, 256);
  Task_GameHandle = osThreadCreate(osThread(Task_Game), NULL);

  osThreadDef(Task_Display, StartTaskDisplay, osPriorityBelowNormal, 0, 256);
  Task_DisplayHandle = osThreadCreate(osThread(Task_Display), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ---------- Task_Input: опрос клавиатуры ---------- */

void StartTaskInput(void const * argument)
{
  int8_t key;
  int8_t last_key = KEY_NONE;

  for(;;)
  {
    osMutexWait(i2cMutexHandle, osWaitForever);     /* шина I2C только для нас */
    key = Keypad_Scan();
    osMutexRelease(i2cMutexHandle);

    /* Сообщаем только о новом нажатии, а не о том, что кнопку продолжают держать */
    if (key != KEY_NONE && key != last_key)
    {
      osMessagePut(keyQueueHandle, key, 0);
    }
    last_key = key;

    osDelay(20);
  }
}

/* ---------- Task_Game: правила игры ---------- */

/* Достает из очереди нажатые кнопки, пока одна из них не повернет змейку.
   Остальные кнопки ждут в очереди следующего шага: если игрок быстро нажал "вверх"
   и "влево", змейка сначала повернет вверх, а на следующем шаге - влево. */
static void HandleKeys(void)
{
  osEvent event = osMessageGet(keyQueueHandle, 0);
  int turned = 0;

  while (event.status == osEventMessage && turned == 0)
  {
    switch (event.value.v)
    {
      case KEY_UP:
        turned = Game_SetDirection(DIR_UP);
        break;
      case KEY_RIGHT:
        turned = Game_SetDirection(DIR_RIGHT);
        break;
      case KEY_DOWN:
        turned = Game_SetDirection(DIR_DOWN);
        break;
      case KEY_LEFT:
        turned = Game_SetDirection(DIR_LEFT);
        break;
    }

    if (turned == 0)
    {
      event = osMessageGet(keyQueueHandle, 0);      /* эта кнопка ничего не изменила - берем следующую */
    }
  }
}

/* Стартовый экран или экран конца игры: ждем любую кнопку и начинаем новую игру */
static void WaitForNewGame(void)
{
  osEvent event;

  if (game.state == GAME_OVER)
  {
    /* Игрок мог еще жать кнопки поворота, поэтому пока показываем результат, нажатия игнорируем */
    osDelay(800);
  }

  /* Выбрасываем кнопки, которые нажали заранее */
  event = osMessageGet(keyQueueHandle, 0);
  while (event.status == osEventMessage)
  {
    event = osMessageGet(keyQueueHandle, 0);
  }

  osMessageGet(keyQueueHandle, osWaitForever);      /* задача спит, пока не придет кнопка */

  osMutexWait(gameMutexHandle, osWaitForever);
  Game_Start(osKernelSysTick());
  osMutexRelease(gameMutexHandle);

  osSemaphoreRelease(redrawSemHandle);              /* просим перерисовать экран */
}

/* Один шаг игры: подождать, применить нажатые кнопки, сдвинуть змейку */
static void PlayOneStep(void)
{
  osDelay(Game_StepDelay());
  HandleKeys();

  osMutexWait(gameMutexHandle, osWaitForever);
  Game_Step(osKernelSysTick());
  osMutexRelease(gameMutexHandle);

  osSemaphoreRelease(redrawSemHandle);              /* просим перерисовать экран */
}

void StartTaskGame(void const * argument)
{
  for(;;)
  {
    if (game.state == GAME_PLAY)
    {
      PlayOneStep();
    }
    else
    {
      WaitForNewGame();
    }
  }
}

/* ---------- Task_Display: вывод на дисплей ---------- */

void StartTaskDisplay(void const * argument)
{
  for(;;)
  {
    osSemaphoreWait(redrawSemHandle, osWaitForever);    /* спим, пока игра не попросит обновить экран */

    osMutexWait(gameMutexHandle, osWaitForever);
    Screen_Draw(&game);                                 /* рисуем в буфер в памяти - быстро */
    osMutexRelease(gameMutexHandle);

    osMutexWait(i2cMutexHandle, osWaitForever);
    oled_UpdateScreen();                                /* отправляем буфер на дисплей по I2C - медленно */
    osMutexRelease(i2cMutexHandle);
  }
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
