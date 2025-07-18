#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "wrappers.h"
#include "isr.h"

#ifndef _APP_TASKS_H
#define _APP_TASKS_H

// Prioridad de las tareas
#define tskINIT_PRIORITY (tskIDLE_PRIORITY + 3UL)			  // UL (unsigned long) para evitar warnings
#define tskADC_PRIORITY (tskIDLE_PRIORITY + 1UL)			  // Tarea de ADC
#define tskBH1750_PRIORITY (tskIDLE_PRIORITY + 1UL)			  // Tarea de BH1750
#define tskDisplayChange_PRIORITY (tskIDLE_PRIORITY + 2UL)	  // Tarea de cambio de display
#define tskControl_PRIORITY (tskIDLE_PRIORITY + 1UL)		  // Tarea de control
#define tskDisplayWrite_PRIORITY (tskIDLE_PRIORITY + 1UL)	  // Tarea de display write
#define tsk_setpoint_PRIORITY (tskIDLE_PRIORITY + 1UL)		  // Tarea de setpoint
#define tskBuzzer_PRIORITY (tskIDLE_PRIORITY + 2UL)			  // Tarea del Sensor IR
#define tskLED_Azul_PRIORITY (tskIDLE_PRIORITY + 1UL)		  // Tarea de LED Azul
#define tsk_leds_control_PRIORITY (tskIDLE_PRIORITY + 1UL)	  // Tarea de control de LEDs
#define tsk_console_monitor_PRIORITY (tskIDLE_PRIORITY + 1UL) // Tarea de consola

// Stack size de las tareas (Tamaño de la pila de cada tarea)
#define tskINIT_STACK (3 * configMINIMAL_STACK_SIZE)
#define tskADC_STACK (configMINIMAL_STACK_SIZE)
#define tskBH1750_STACK (configMINIMAL_STACK_SIZE)
#define tsk_setpoint_STACK (configMINIMAL_STACK_SIZE)
#define tskDisplayChange_STACK (configMINIMAL_STACK_SIZE)
#define tskDisplayWrite_STACK (3 * configMINIMAL_STACK_SIZE / 2)
#define tskControl_STACK (configMINIMAL_STACK_SIZE)
#define tskBuzzer_STACK (configMINIMAL_STACK_SIZE)
#define tskLED_Azul_STACK (configMINIMAL_STACK_SIZE)
#define tsk_leds_control_STACK (configMINIMAL_STACK_SIZE)
#define tsk_console_monitor_STACK (2 * configMINIMAL_STACK_SIZE)

// Handler para la tarea de display write
extern TaskHandle_t handle_display;

// Prototipos de las tareas
void tsk_init(void *params);
void tsk_adc(void *params);
void tsk_display_change(void *params);
void tsk_control(void *params);
void tsk_display_write(void *params);
void tsk_BH1750(void *params);
void tsk_setpoint(void *params);
void tsk_buzzer(void *params);
void tsk_led_azul(void *params);
void tsk_leds_control(void *params);
void tsk_console_monitor(void *params);

/**
 * @brief Wrapper que verifica el estado de un pulsador con pull-up
 * aplicando un antirebote
 * @param btn estructura al GPIO del pulsador
 */
static inline bool wrapper_btn_get_with_debouncing_with_pull_up(gpio_t btn)
{
	//	Pregunto si se precionó el pulsador
	if (!wrapper_btn_get(btn))
	{
		// Antirebote
		vTaskDelay(pdMS_TO_TICKS(20));
		if (!wrapper_btn_get(btn))
		{
			return true;
		}
	}
	return false;
}

#endif