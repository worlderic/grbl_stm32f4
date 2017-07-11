//-----------------------------------------------------------------------------
/*

*/
//-----------------------------------------------------------------------------

#include "stm32f4xx_hal.h"
#include "SEGGER_RTT.h"

#include "gpio.h"
#include "debounce.h"
#include "timers.h"

#ifdef USB_SERIAL
  #include "usbd_desc.h"
  #include "usbd_cdc.h"
  #include "usbd_cdc_interface.h"
#else
  #include "usart.h"
#endif

//-----------------------------------------------------------------------------

extern int grbl_main(void);

//-----------------------------------------------------------------------------

#ifdef USB_SERIAL
USBD_HandleTypeDef hUSBDDevice;
#endif

//-----------------------------------------------------------------------------

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    while (1);
}
#endif

void Error_Handler(void)
{
    while (1);
}

//-----------------------------------------------------------------------------

static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    // Enable Power Control clock
    __PWR_CLK_ENABLE();

    // The voltage scaling allows optimizing the power consumption when the device is
    // clocked below the maximum system frequency, to update the voltage scaling value
    // regarding system frequency refer to product datasheet.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Enable HSE Oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

//-----------------------------------------------------------------------------

void limits_isr(void);
void control_isr(void);

int limits_enabled = 0;
int controls_enabled = 0;

void debounce_on_handler(uint32_t bits)
{
    if (bits & (1 << PUSH_BUTTON_BIT)) {
        gpio_set(LED_RED);
    }

    // check limits
    if (limits_enabled && (bits & LIMIT_MASK)) {
        limits_isr();
    }

    // check machine switches
    if (controls_enabled && (bits & CONTROL_MASK)) {
        control_isr();
    }
}

void debounce_off_handler(uint32_t bits)
{
    if (bits & (1 << PUSH_BUTTON_BIT)) {
        gpio_clr(LED_RED);
    }
}

//-----------------------------------------------------------------------------

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    SEGGER_RTT_Init();
    gpio_init();
    timers_init();
    debounce_init();

    SEGGER_RTT_TerminalOut(0, "msg1on0");
    SEGGER_RTT_TerminalOut(0, "message2on0");
    SEGGER_RTT_TerminalOut(1, "msg3on1");
    SEGGER_RTT_TerminalOut(1, "message4on1");

#ifdef USB_SERIAL
    USBD_Init(&hUSBDDevice, &VCP_Desc, 0);
    USBD_RegisterClass(&hUSBDDevice, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUSBDDevice, &USBD_CDC_fops);
    USBD_Start(&hUSBDDevice);
    // Delay any output to serial until the USB CDC port is working.
    HAL_Delay(1500);
#else
    usart_init();
#endif
    grbl_main();
    return 0;
}

//-----------------------------------------------------------------------------