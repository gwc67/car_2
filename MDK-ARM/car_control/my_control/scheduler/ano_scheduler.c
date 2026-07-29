#include "ano_scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver_registry.h"
#include "cmsis_os2.h"
#include "leds.h"
#include "encode.h"
#include "Drv_Key.h"
#include "Key_func.h"
#include "OLED_Menu.h"
#include "tim.h"
#include "OLED.h"
#include "Drv\Motor\motor.h"
#include "Drv\xvji\xvji.h"
#include "Drv\NRF\nrf.h"

extern osThreadId_t task_10ms_highHandle;

extern osThreadId_t task_10_ms_lowHandle;


void task_1ms_fun(void *argument)
{

  driver_init_all();
  HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);
  xTaskNotifyGive(task_10_ms_lowHandle);
  xTaskNotifyGive(task_10ms_highHandle);
  for(;;)
  {
      
      
      
      
  }

}


void task_10ms_high_fun(void *argument)
{

    TickType_t xLastWakeTime;
    const TickType_t xFlightCorePeriod = pdMS_TO_TICKS(10);
    // 增加超时兜底，防止启动通知丢失卡死
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xLastWakeTime = xTaskGetTickCount(); // 收到通知之后再初始化基准tick

  for(;;)
  {
    vTaskDelayUntil(&xLastWakeTime, xFlightCorePeriod);
    xvji_sample();
    encoder_sample_all();

    /* NRF 收发: 收飞控指令 + 发状态 */
    NRF_Receive();

    menu_request_refresh(g_encode_pst);
  }

}



void task_10ms_low_fun(void *argument)
{

    TickType_t xLastWakeTime;
    const TickType_t xFlightCorePeriod = pdMS_TO_TICKS(10);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFlightCorePeriod);

        key_scan_v(xTaskGetTickCount());
        keyfunc_scan_v(xTaskGetTickCount());
        menu_task_v();
        static uint32_t test_ul = 0;
      static bool s_b = false;
      uint32_t curr_tick_ul = xTaskGetTickCount();

      if (curr_tick_ul - test_ul > 500)
      {
        test_ul = curr_tick_ul;
        if (s_b == false)
        {
          s_b = true;
          led_on(g_led_pid_gpio_pst);
        }
        else if (s_b == true)
        {
          s_b = false;
          led_off(g_led_pid_gpio_pst);
        }
      }
    }

}

