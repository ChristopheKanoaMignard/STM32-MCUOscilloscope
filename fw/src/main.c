/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal_conf.h"
#include "hwConfig.h"
#include "regs.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "math.h"

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_sdram.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

#include "oscilloscope.h"
/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint16_t FirmWareVersion = 1;
uint8_t txBuf[2048];			//to receive data over usb_user
uint8_t rxBuf[2048];			//to receive data over usb_user
uint32_t rxLen = 0;				//to receive data over usb_user
uint32_t lastBlink;				//for blinking hearbeat LED


/*DELETE?
uint32_t oscopeTime;
uint16_t adcEvents = 0;
uint32_t adcLastConversionTime = 42;
*/
/***------------------------START: Display DAC-generated escelator function by using ADC------------------------***/
	

	
/***END: Display DAC-generated escelator function by using ADC***/

/* Private function prototypes -----------------------------------------------*/
static void BlinkGreenLed(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_NVIC_Init();
	//MX_SPI2_Init();
	BSP_LCD_Init();			//Need to init this twice to avoid screen flickering. Solve the order of inits

	MX_ADC1_Init();
	//MX_ADC2_Init();
	MX_TIM3_Init();
	/**/
	
	if (HAL_ADC_Start_IT(&hadc1) != HAL_OK) {Error_Handler();}														//!! These should be started right before they are needed, not here. 
												
	//if( HAL_ADC_Start_IT(&hadc2) != HAL_OK ) {Error_Handler();}													//!! These should be started right before they are needed, not here. 
		
	if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {Error_Handler();}										//!! These should be started right before they are needed, not here. 
	
	MX_USB_DEVICE_Init();
	InitRegs();
	//Load_Params(0); //this causes a HardFault, probably because reading beyond flash
	
	/***Initialize all things related to LCD***/
	
	//MX_LTDC_Init();		//Use BSP function instead
	
	BSP_SDRAM_Init();					// Initializes the SDRAM device
	__HAL_RCC_CRC_CLK_ENABLE();	//Probably need MX_CRC_Init		// Enable the CRC Module
	BSP_TS_Init(maxPixels, maxLines);
	BSP_LCD_Init();						//Use in place of doing everything manually with MX_LTDC_Init()
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
	//BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER);
	BSP_LCD_DisplayOn();
	BSP_LCD_SelectLayer(0);					//Layer to write gridlines to
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	OscilloscopeDisplayDefaultSetup();
	MX_DAC_Init();
	HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
	/***------------------------START: write a sine wave to display------------------------***/
/*
	
*/
	/***End: write a sine wave to display***/

	while (1)
	{	
		/*The following functions were implemented to test display and DAC before I assembled the oscilloscope with them*/
		//BackgroundColorChangeButtons();
		//DisplayTouchedPixel();
		//ManuallyDrawSinWaveform((2*M_PI), 1);
		//DacEscallatorWaveform(1);
		
		OscilloscopeWriteAdcToDisplayContinuous();
		//OscilloscopeWriteAdcToDisplaySingle();
		
		
		//BlinkGreenLed();
		if (rxLen > 0) {
			Parse();
		}
	}
}

void BlinkGreenLed()
{
	//period is blinkTime + blinkOnTime = 889*(1+1/8) = 1000
	static uint32_t blinkTime = 889; //off time
	static uint8_t bBlinkOn = 1;
	uint32_t blinkOnTime = blinkTime >> 3; //on time
	uint32_t sysTick = HAL_GetTick();
  
	if ((sysTick - lastBlink) > blinkOnTime) {
		if (bBlinkOn) {
			// LED off
			HAL_GPIO_WritePin(GPIOG, Pin_LedGreen, GPIO_PIN_RESET);
			bBlinkOn = 0;
			lastBlink = sysTick;
		}
		else if ((sysTick - lastBlink) > blinkTime) {
			// LED on
			HAL_GPIO_WritePin(GPIOG, Pin_LedGreen, GPIO_PIN_SET);
			bBlinkOn = 1;
			lastBlink = sysTick;
		}
	}
}

/*
## Overview
The goal of this project was to familiarize myself with the way LCDs are programmed. After creating some elementary UI elements and drawing basic shapes, the project pivoted slightly, and I decided to make a simplistic oscilloscope. 

I started by writing a function that would convert arbitrary x and y values into pixel positions, and then drew a sine waveform on the display. Initially I drew individual pixels, but discontinuities appeared when using longer time intervals, so I switched to drawing lines between pixels.
[Sin Wave HERE]
Next, I generated an escalator step function using a $12$--bit digital--to--analog converter (DAC) whose output increased by $1V$ every $50ms$, and dropped back down to $0V$ after reaching the maximum pin voltage of $3V$. At first, I simply wrote this waveform to the display like my sine wave, but to make the most basic version of my oscilloscope I initialized an analog--to--digital converter (ADC) and bridged the two converters. The output of this system was a hex value that I transformed into the pixel space and wrote to the display.
[DAC Escalator HERE]
## Explanation of important functions

 - LinearMapDouble and LinearMapInt32
 &emsp;Both these functions transform two arrays containing data on the time and voltage domains into the x--- and y--axes of the pixel space. The variable/parameter linearMapVars[] contains number of pixels to draw, and the weight and bias for the x--- and y--axes. The weight and bias are derived by solving the system of equations
$$\overline{x} = w_x*x+b_x \\
	\overline{y} = w_y*y+b_y$$
	where $x$ and $y$ are time in ms and voltage in adc units respectively, and $\overline{x}$ and $\overline{y}$ are the $x$ and $y$ positions of a pixel. By default, I set the range of $x$ to be $[0, 320)$ because the display has $320$ pixel positions on the x--axis. Therefore every pixel represents $1ms$, and $w_x=1$ and $b_y=0$. The range of $y$ was $[0, 0xFFF]$ while the range of $\overline{y}$ was $[0, 240)$, so solving the system of equations yields $w_y = \frac{239}{0xFFF}$ and $b_y=0$.
	 &emsp;More generally, $w_n = \frac{\overline{n}_2-\overline{n}_1}{n_2-n_1}$ and $b_n=\frac{n_2\overline{n}_1-\overline{n}_2n_1}{n_2-n_1}$ where $\overline{n}\in[\overline{n}_1, \overline{n}_2]$ and $n\in[n_1, n_2].$ I considered including a function that automatically handles this transformation, but the calculation is simple enough and I do not forsee re-evaluating these transformation variables to be significantly important. 
 - DrawArrayOfPixelSegments
 - DrawGridLines
 - OscilloscopeWriteAdcToDisplayContinuous and OscilloscopeWriteAdcToDisplaySingle

## Known problems and limitations

&emsp;Currently there are only three resolution settings (normal, coarse, and fine) each for voltage and time axes. While the display and the 12-bit ADC place constraints that make such restrictions reasonable, they may become a problem if this code is reused for larger displays or ADCs that can handle a wider voltage range. 
&emsp;Waveform data displayed on the first thirty or so columns is often distorted. This is likely do to the ADC not having sufficient time to complete it averaging before that data is transformed and written to the display. One possible solution is to take extra data points and not reveal the most recent points. 
&emsp;Aliasing problem. 
&emsp;Flickering in continuous mode. 
*/