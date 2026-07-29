#include "ano_scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver_registry.h"
#include "cmsis_os2.h"
#include "leds.h"

extern osThreadId_t task_10ms_highHandle;

extern osThreadId_t task_10_ms_lowHandle;


void task_1ms_fun(void *argument)
{

  driver_init_all();

  for(;;)
  {
      ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
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


void task_10ms_high_fun(void *argument)
{

  for(;;)
  {
    osDelay(1);
  }

}



void task_10ms_low_fun(void *argument)
{

  for(;;)
  {
    osDelay(1);
  }

}

