#include "tareas.h"

// -------
// Colas |
// -------
QueueHandle_t queue_adc;              // Cola para datos del ADC
QueueHandle_t queue_display;          // Cola para datos del display
QueueHandle_t queue_display_variable; // Cola para variable a mostrar en el display
QueueHandle_t queue_lux;              // cola para datos de lux (porcentaje)
QueueHandle_t queue_lux_raw;          // cola para datos de lux (valor bruto)

// ----------
// Semaforo |
// ----------
xSemaphoreHandle semphr_buzz; // Interrupción del Sensor IR
xSemaphoreHandle semphr_usr;  // Boton de USER

// Handler para display (Puntero o referencia para identificar y controlar una tarea específica después de haberla creado)
TaskHandle_t DisplayHandler;

// Setpoint
float setpoint = 50.0f;

// --------------------------
// Definición de las tareas |
// --------------------------

// Tarea 0: Inicialización de periféricos y recursos
void tsk_init(void *params)
{
    // Inicializo los semaforos
    semphr_buzz = xSemaphoreCreateBinary();
    semphr_usr = xSemaphoreCreateBinary();

    // Incializo colas
    queue_adc = xQueueCreate(1, sizeof(adc_data_t));
    queue_display = xQueueCreate(1, sizeof(display_data_t));
    queue_display_variable = xQueueCreate(1, sizeof(display_variable_t));
    queue_lux = xQueueCreate(1, sizeof(float));
    queue_lux_raw = xQueueCreate(1, sizeof(uint16_t));

    // Incialización de GPIO
    wrapper_gpio_init(0);
    wrapper_gpio_init(1);
    // Inicialización del LED
    wrapper_output_init((gpio_t){LED}, true);
    // Inicialización del buzzer
    wrapper_output_init((gpio_t){BUZZER}, false);
    // Inicialización del enable del CNY70
    wrapper_output_init((gpio_t){CNY70_EN}, true);
    // Configuro el ADC
    wrapper_adc_init();
    // Configuro el display
    wrapper_display_init();
    // Configuro botones
    wrapper_btn_init();
    // Configuro interrupción por flancos para el infrarojo y para el botón del user
    wrapper_gpio_enable_irq((gpio_t){CNY70}, kPINT_PinIntEnableBothEdges, cny70_callback);
    wrapper_gpio_enable_irq((gpio_t){USR_BTN}, kPINT_PinIntEnableFallEdge, usr_callback);
    // Inicializo el PWM
    wrapper_pwm_init();
    // Inicializo I2C
    wrapper_i2c_init();
    // Inicializo el BH1750
    wrapper_bh1750_init();

    // Elimino tarea para liberar recursos
    vTaskDelete(NULL);
}

// ---------------
// Tarea 1: ADC  |
// ---------------
void tsk_adc(void *params)
{

    while (1)
    {
        // Inicio una conversion
        ADC_DoSoftwareTriggerConvSeqA(ADC0);
        // Bloqueo la tarea por 250 ms
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// ---------------------------------------
// Tarea 2: Control del botón de usuario |
// ---------------------------------------
void tsk_display_change(void *params)
{
    // Dato para pasar
    display_variable_t variable = kDISPLAY_TEMP;

    while (1)
    {
        // Escribe el dato en la cola
        xQueueOverwrite(queue_display_variable, &variable);
        // Intenta tomar el semáforo
        xSemaphoreTake(semphr_usr, portMAX_DELAY);
        // Si se presionó, cambio la variable
        variable = (variable == kDISPLAY_TEMP) ? kDISPLAY_REF : kDISPLAY_TEMP;
    }
}

// --------------------------------
// Tarea 3: Escribe en el display |
// --------------------------------
void tsk_control(void *params)
{
    // Variable a mostrar
    display_variable_t variable = kDISPLAY_TEMP;
    float lux_pct = 0;
    float setpoint_local = 25;
    display_data_t disp = {0};

    while (1)
    {
        // Veo que variable hay que mostrar
        xQueuePeek(queue_display_variable, &variable, portMAX_DELAY);

        if (variable == kDISPLAY_TEMP)
        {
            // Leo los datos del luminosidad
            xQueuePeek(queue_lux, &lux_pct, portMAX_DELAY);
            disp.value = (uint16_t)lux_pct;
            disp.show_dp = false;
        }
        else
        {
            // Muestro Setpoint
            setpoint_local = setpoint;
            disp.value = (uint16_t)setpoint_local;
            disp.show_dp = true; // Muestra el punto decimal
        }

        // Si el valor es mayor a 99, lo limito a 99
        if (disp.value > 99)
        {
            disp.value = 99;
        }
        // Escribe el valor en la cola
        xQueueOverwrite(queue_display, &disp);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ----------------------------------------
// Tarea 4: Escribe un numero en el display
// ----------------------------------------
void tsk_display_write(void *params)
{
    // Variable con el dato para escribir
    display_data_t data;

    while (1)
    {
        // Mira el dato que haya en la cola
        if (!xQueuePeek(queue_display, &data, pdMS_TO_TICKS(100)))
        {
            continue;
        }
        // Muestro el número
        wrapper_display_off();
        wrapper_display_write((uint8_t)(data.value / 10), false);
        wrapper_display_on((gpio_t){COM_1});
        vTaskDelay(pdMS_TO_TICKS(10));

        // Si se debe mostrar el punto decimal, lo muestro
        wrapper_display_off();
        wrapper_display_write((uint8_t)(data.value % 10), data.show_dp);
        wrapper_display_on((gpio_t){COM_2});
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --------------------------
// Tarea 5: Sensor BH1750
// --------------------------
void tsk_BH1750(void *params)
{
    // Valor de intensidad luminica
    uint16_t lux = 0;
    float lux_pct = 0;

    while (1)
    {
        // Bloqueo por 160 ms (requisito)
        vTaskDelay(pdMS_TO_TICKS(200));

        // Leo el valor de lux
        lux = wrapper_bh1750_read();
        if (lux > 30000)
            lux = 30000;

        // Calculo porcentaje.
        lux_pct = (lux / 30000.0f) * 100.0f;

        // Muestro por consola
        xQueueOverwrite(queue_lux, &lux_pct);
        xQueueOverwrite(queue_lux_raw, &lux);
    }
}

// ------------------------------------
// Tarea 6: Control de LED Azul con RV22
// ------------------------------------
void tsk_led_azul(void *params)
{
    adc_data_t adc_data;
    int16_t duty_led = 0;

    while (1)
    {
        // Leeo datos de ADC
        xQueuePeek(queue_adc, &adc_data, portMAX_DELAY);

        // Escalo a 0-100%
        duty_led = (adc_data.temp_raw * 100) / 4095;

        // Actualizo el PWM del LED azul
        wrapper_pwm_update_led_azul(duty_led);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --------------------------
// Tarea 7: Setpoint
// --------------------------
void tsk_setpoint(void *params)
{
    while (1)
    {
        // Chequeo el botón S1 (incrementa)
        if (wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S1_BTN}))
        {
            if (setpoint < 75.0f)
                setpoint += 1.0f;
            // Espera a que se suelte el botón (para evitar múltiples incrementos)
            while (!wrapper_btn_get((gpio_t){S1_BTN}))
                vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Chequear botón S2 (decrementa)
        if (wrapper_btn_get_with_debouncing_with_pull_up((gpio_t){S2_BTN}))
        {
            if (setpoint > 25.0f)
                setpoint -= 1.0f;
            // Espera a que se suelte el botón
            while (!wrapper_btn_get((gpio_t){S2_BTN}))
                vTaskDelay(pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Polling cada 50ms
    }
}

// --------------------------
// Tarea 8: Buzzer
// --------------------------
void tsk_buzzer(void *params)
{
    while (1)
    {
        xSemaphoreTake(semphr_buzz, portMAX_DELAY);
        // Enciende el buzzer
        GPIO_PinWrite(GPIO_DESTRUCT((gpio_t){BUZZER}), 1);
        vTaskDelay(pdMS_TO_TICKS(200)); // 200 ms encendido
        // Apaga el buzzer
        GPIO_PinWrite(GPIO_DESTRUCT((gpio_t){BUZZER}), 0);
    }
}

// --------------------------
// Tarea 9: LEDs tricolor
// --------------------------
void tsk_leds_control(void *params)
{
    float lux_pct = 0;
    float setpoint_local = 0;
    int16_t duty_rled = 0;
    int16_t duty_bled = 0;
    const float DEADZONE = 1.0f; // Zona muerta para evitar parpadeos por ruido

    while (1)
    {
        // Leer luminosidad actual
        xQueuePeek(queue_lux, &lux_pct, portMAX_DELAY);

        // Leer setpoint actual
        setpoint_local = setpoint;

        float diff = lux_pct - setpoint_local;

        if (diff > DEADZONE)
        {
            // Más luminosidad que setpoint: encender rojo proporcional
            duty_rled = (int16_t)(diff);
            if (duty_rled > 100)
                duty_rled = 100;
            duty_bled = 0;
        }
        else if (diff < -DEADZONE)
        {
            // Menos luminosidad que setpoint: encender azul proporcional
            duty_bled = (int16_t)(-diff);
            if (duty_bled > 100)
                duty_bled = 100;
            duty_rled = 0;
        }
        else
        {
            // Diferencia muy chica: ambos apagados
            duty_rled = 0;
            duty_bled = 0;
        }

        // Actualizar PWM de los LEDs
        wrapper_pwm_update_rled(duty_rled);
        wrapper_pwm_update_bled(duty_bled);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --------------------------
// Tarea 10: Monitor de consola
// --------------------------
void tsk_console_monitor(void *params)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    float lux_pct = 0;
    uint32_t tiempo_ms = 0;

    while (1)
    {
        // Intento leer el valor de lux
        xQueuePeek(queue_lux, &lux_pct, 0);

        // Calculo el tiempo en ms
        tiempo_ms = (xTaskGetTickCount() - xLastWakeTime) * portTICK_PERIOD_MS;

        // Imprimo por consola los valores
        PRINTF("Tiempo: %lu ms | Lux: %.1f%%\r\n", tiempo_ms, lux_pct);

        // Delay de 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}