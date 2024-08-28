/**
  ******************************************************************************
  * @file           : oscilloscope.c
  * @brief          : Code to read and write waveforms to display
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "oscilloscope.h"
#include "main.h"
#include "hwConfig.h"
#include "regs.h"
#include "math.h"
#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_sdram.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include "fonts.h"
/* Exported variables --------------------------------------------------------*/
double linearMapVars[5];				//{maxLines or numPixelsToWriteDefault, biasX, weightX, biasY, weightY };
double biasX = 0;				
double weightX = 1;	
double biasY = 0;						
double weightY = (maxPixels - 1) / maxYValue;
uint8_t oscopeGridResolutionX  = oscopeGridResolutionXDefault;				//default values for the oscilloscope grid lines. Can be changed by writing to RegOScopeResX, RegOScopeResY
uint8_t oscopeGridResolutionY = oscopeGridResolutionYDefault;	
/*
*/
/* Private variables ---------------------------------------------------------*/
uint16_t pixelsToDraw[2][maxLines];			//{x-axis pixels[], y-axis pixels[]} note that axes get flipped in landscape mode.


uint32_t adcCurrentTime, adcLastTime = 0;
uint32_t adcDuration = 3;
int adcIndex = 0; 
uint32_t adcDataVolt[maxLines], adcDataTime[maxLines];		//Initialize all data-containing arrays to maxLines since that is the most pixels that will ever be drawn. In reality, only linarMapVars[0] pixels will be drawn.
int boolReadyToWriteAdc = 0;
uint8_t boolButtonPressed = 0;
uint32_t colorBackground = LCD_COLOR_WHITE;			//These variables control the colors of the display elements on osciloscope
uint32_t colorWaveform = LCD_COLOR_BLUE;
uint32_t colorGridlines = LCD_COLOR_GRAY;



TS_StateTypeDef  ts;
char xTouchStr[10];				//For storing characters to be displayed to string. Tells user what pixels were just touched
uint16_t xPos;					//Stored the position of the most recently touched pixel
uint16_t yPos;					//Stored the position of the most recently touched pixel

/* Private function prototypes -----------------------------------------------*/
void OscilloscopeDisplayDefaultSetup(void);
void ManuallyDrawSinWaveform(double width, double amplitude);
void DacEscallatorWaveform(int boolWriteDisplay);
void OscilloscopeWriteAdcToDisplayContinuous(void);
void OscilloscopeWriteAdcToDisplaySingle(void);
	
void CalculateLinearMapVars(uint16_t minPixelPosX, uint16_t maxPixelPosX, double minValueX, double maxValueX, uint16_t minPixelPosY, uint16_t maxPixelPosY, double minValueY, double maxValueY, double linearMapVars[]);
void LinearMapDouble(double xValueToMap[maxLines], double yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines]);
void LinearMapInt32(uint32_t xValueToMap[maxLines], uint32_t yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines]);
void DrawArrayOfPixels(uint16_t x[maxLines], uint16_t y[maxLines], uint32_t RGB_Code);
void DrawArrayOfPixelSegments(uint16_t x[maxLines], uint16_t y[maxLines], uint32_t RGB_Code);
void DrawGridLines(uint8_t resolutionColumn, uint8_t resolutionRow, uint32_t RGB_Code);

void DisplayTouchedPixel(void);
void BackgroundColorChangeButtons(void);

/* Oscilloscope functions ----------------------------------------------------*/
void OscilloscopeDisplayDefaultSetup()
{
	linearMapVars[0] = (double)(numPixelsToWriteDefault);	//Number of pixels to draw. Controls how many times to loop through linear mapping and pixel drawing	
	/*
	linearMapVars[1] = biasX;								//x-axis bias
	linearMapVars[2] = weightX;								//x-axis weight
	linearMapVars[3] = biasY;								//y-axis bias
	linearMapVars[4] = weightY;								//y-axis weight
	*/
	CalculateLinearMapVars(0, maxLines, 0, maxLines, 0, maxPixels, 0, 0xFFF, linearMapVars);
	BSP_LCD_SetFont(&Font20);								//Must be small enough that displaying ms/div and mV/div to user never falls off the edge of the screen, but large enough to easily read
}

void ManuallyDrawSinWaveform(double width, double amplitude)	//Portrait mode. Swap maxPixels and maxLines to do landscape mode.
{
	double maxXValueSin = width;
	double minXValueSin = 0;
	double biasX = 0;				//Supposed to be 0, but this could be used to cause a phase shift, or to scroll.
	double weightX = maxLines / (maxXValueSin - minXValueSin);
	double maxYValueSin = amplitude;
	double minYValueSin = -1*amplitude;
	double biasY = maxPixels / (maxYValueSin - minYValueSin);
	double weightY = biasY;
	linearMapVars[0] = (double)(maxLines);		//How many times to loop through linear mapping and pixel drawing
	linearMapVars[1] = biasX;							//x-axis bias
	linearMapVars[2] = weightX;							//x-axis weight
	linearMapVars[3] = biasY;							//y-axis bias
	linearMapVars[4] = weightY;							//y-axis weight
	
	double x[maxLines], sinOfX[maxLines] = { 0 };
	//uint16_t pixelsToDraw[2][maxPixels];
	x[0] = 0;
	double xStepSize = maxXValueSin / maxLines;
	for (int ii = 1; ii < maxLines; ii++)
	{
		x[ii] = x[ii - 1] + xStepSize;	
		sinOfX[ii] = sin(x[ii]);
	}
	LinearMapDouble(x, sinOfX, linearMapVars, pixelsToDraw);
	DrawArrayOfPixelSegments(pixelsToDraw[0], pixelsToDraw[1], colorWaveform);
	
}
void DacEscallatorWaveform(int boolWriteDisplay) 
{
	//Generate a DAC escelator by increasing the value of the DAC every 50msm then resetting the value of DAC to 0 every 200ms
	static uint32_t dacOutEscalator[4] = { 0, 0xFFF / 3, 0xFFF * 2 / 3, 0xFFF };	//DAC is 12bit, so 0xFFF is the max value which produces 3V.
	static int dacEscalatorStep = 0;
	static uint32_t dacCurrentTime, dacLastTime, dacwriteCurrentTime, dacwriteLastTime = 0;
	static uint32_t dacDuration = 50;
	static uint32_t dacwriteDuration = 3;
	static uint32_t dacDataVolt[maxLines], dacDataTime[maxLines];
	static int dacIndex = 0;
	
	dacCurrentTime = HAL_GetTick();
	if ((dacCurrentTime - dacLastTime) >= dacDuration)		//Every dacDuration (50ms) step up/down the value written to the dac 
	{
		dacLastTime = HAL_GetTick();
		HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dacOutEscalator[dacEscalatorStep]);
		dacEscalatorStep++;
		if (dacEscalatorStep == 4) {
			dacEscalatorStep = 0;
		}
	}
	//Write DAC output to display
	dacwriteCurrentTime = HAL_GetTick();
	if (boolWriteDisplay && (dacwriteCurrentTime - dacwriteLastTime) >= dacwriteDuration)
	{
		dacwriteLastTime = HAL_GetTick();
		dacDataVolt[dacIndex] = HAL_DAC_GetValue(&hdac, DAC_CHANNEL_2);
		dacDataTime[dacIndex] = dacwriteDuration*dacIndex;
		dacIndex++;
	}
		if (dacIndex == linearMapVars[0] - 1) {
			BSP_LCD_Clear(colorBackground);	
		}
		else if (dacIndex >= linearMapVars[0]) 
		{
			dacIndex = 0;			
			DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionX, colorGridlines);
			LinearMapInt32(dacDataTime, dacDataVolt, linearMapVars, pixelsToDraw);
			//BSP_LCD_SelectLayer(1);							//!! Trying to use different layers to reduce flickering
			DrawArrayOfPixelSegments(pixelsToDraw[0], pixelsToDraw[1], colorWaveform);
			DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionX, colorGridlines);
		}
}
void OscilloscopeWriteAdcToDisplayContinuous()
{
	if (adcIndex == linearMapVars[0] - 1) {
		BSP_LCD_Clear(colorBackground);				//!! Clear the background just before drawing the updated waveform. Causes flickering, should find a better solution. Perhaps take insiration from the way retro games place sprites on backgrounds!! 
	}
	if (boolReadyToWriteAdc == 1)					//Satisfied when the adc callback has completley refilled the adcDataTime and adcDataVolt arrays
	{
		boolReadyToWriteAdc = 0;														//Reset the booleans to prep for next time adc callback fills the adcDataTime and adcDataVolt arrays
		DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionY, colorGridlines);	//Draw grid
		LinearMapInt32(adcDataTime, adcDataVolt, linearMapVars, pixelsToDraw);			//Convert from mV and ms to pixel positions
		DrawArrayOfPixelSegments(pixelsToDraw[0], pixelsToDraw[1], colorWaveform);		//Draw waveform
		DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionY, colorGridlines);	//!! Redraw grid. For some reason, at least one line is always missing unless grid is drawn before and after drawing waveform!!
	}	
}

void OscilloscopeWriteAdcToDisplaySingle()
{
	if (boolButtonPressed == 1)
		{
			BSP_LCD_Clear(colorBackground);
			if (boolReadyToWriteAdc == 1)		//!! Probably unnecessary condition because user is unlikely to press button repeatedly faster than the adc callback fills the adcDataTime and adcDataVolt arrays!!
			{
				boolReadyToWriteAdc = 0;														//Reset the booleans to prep for next time adc callback fills the adcDataTime and adcDataVolt arrays
				boolButtonPressed = 0;															//Reset the booleans to prep for next time user button is pressed
				DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionY, colorGridlines);	//Draw grid
				LinearMapInt32(adcDataTime, adcDataVolt, linearMapVars, pixelsToDraw);			//Convert from mV and ms to pixel positions
				DrawArrayOfPixelSegments(pixelsToDraw[0], pixelsToDraw[1], colorWaveform);		//Draw waveform
				DrawGridLines(oscopeGridResolutionX, oscopeGridResolutionY, colorGridlines);	//!! Redraw grid. For some reason, at least one line is always missing unless grid is drawn before and after drawing waveform!!
			}
		}	
}


















#ifdef LandscapeMode
void CalculateLinearMapVars(uint16_t minPixelPosX, uint16_t maxPixelPosX, double minValueX, double maxValueX, uint16_t minPixelPosY, uint16_t maxPixelPosY, double minValueY, double maxValueY, double linearMapVars[])
{
	linearMapVars[1] = (maxValueX*minPixelPosX - maxPixelPosX*minValueX) / (maxValueX - minValueX);
	linearMapVars[2] = (maxPixelPosX - minPixelPosX) / (maxValueX - minValueX);
	linearMapVars[3] = (maxValueX*minPixelPosY - maxPixelPosX*minValueY) / (maxValueY - minValueY);
	linearMapVars[4] = (maxPixelPosY - minPixelPosY) / (maxValueY - minValueY);
}
/*
 *Maps a signal in abstract [x, y] cartesian coordinate system to the [xPixel, yPixel] cartesian coordinate system of LCD.
*/
void LinearMapDouble(double xValueToMap[maxLines], double yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines])
{	static double a;
	for (int ii = 0; ii < (int)linearMapVars[0]; ii++)	//ii < maxPixels
	{
		pixelsToDraw[0][ii] = (uint16_t)(linearMapVars[2] * xValueToMap[ii] + linearMapVars[1]);	//pixel position = Weight * cartesian value + bias
		//pixelsToDraw[1][ii] = (uint16_t)(linearMapVars[4] * yValueToMap[ii] + linearMapVars[3]);
		//pixelsToDraw[0][ii] = (uint16_t)(20 * xValueToMap[ii] + 1);	//pixel position = Weight * cartesian value + bias
		pixelsToDraw[1][ii] = (uint16_t)(120 * yValueToMap[ii] + 120);
	}
}
/*
 *Maps a signal in abstract [x, y] cartesian coordinate system to the [xPixel, yPixel] cartesian coordinate system of LCD.
*/
void LinearMapInt32(uint32_t xValueToMap[maxLines], uint32_t yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines])
{
	for (int ii = 0; ii < (int)linearMapVars[0]; ii++)	//ii < maxPixels
	{
		pixelsToDraw[0][ii] = (uint16_t)(linearMapVars[2] * xValueToMap[ii] + linearMapVars[1]);	//pixel position = Weight * cartesian value + bias
		pixelsToDraw[1][ii] = (uint16_t)(linearMapVars[4] * yValueToMap[ii] + linearMapVars[3]);
		if (pixelsToDraw[1][ii] > maxPixels) {				//Prevents values that are too large due to resolution scaling from overflowing and wrapping around from bottom of display
			pixelsToDraw[1][ii] = maxPixels;
		}
	}
}
/*
 *Draw a series of black pixel at specififed pixel postitions.
*/
void DrawArrayOfPixels(uint16_t x[maxLines], uint16_t y[maxLines], uint32_t RGB_Code)
{
	for (int ii = 0; ii < (int)linearMapVars[0]; ii++)
	{
		BSP_LCD_DrawPixel(y[ii], x[ii], LCD_COLOR_BLACK);
	}

}
/*
 *Draw black lines of pixels between a sequence of pixel postition.
 *In landscape mode, the variables I am calling x and y are swapped from what the BSP library calls x and y.
*/
void DrawArrayOfPixelSegments(uint16_t x[maxLines], uint16_t y[maxLines], uint32_t RGB_Code)
{
	for (int ii = 1; ii < (int)linearMapVars[0]; ii++)
	{
		BSP_LCD_DrawLine(y[ii - 1], x[ii - 1], y[ii], x[ii]);
	}

}
#endif

#ifndef LandscapeMode
/*
 *Maps a signal in abstract [x, y] cartesian coordinate system to the [xPixel, yPixel] cartesian coordinate system of LCD.
 *Use to build an oscilloscope.
 **/
void LinearMapDouble(double xValueToMap[maxLines], double yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines])
{
	//uint16_t pixelPositions[2][maxPixels];
	for (int ii = 0; ii < (int)linearMapVars[0]; ii++)	//ii < maxPixels
	{
		pixelsToDraw[0][ii] = (uint16_t)(linearMapVars[2] * xValueToMap[ii] + linearMapVars[1]);	//pixel position = Weight * cartesian value + bias
		pixelsToDraw[1][ii] = (uint16_t)(linearMapVars[4] * yValueToMap[ii] + linearMapVars[3]);
	}
	//return pixelPositions;
}
/*
 *Maps a signal in abstract [x, y] cartesian coordinate system to the [xPixel, yPixel] cartesian coordinate system of LCD.
 *Use to build an oscilloscope.
 **/
void LinearMapInt32(uint32_t xValueToMap[maxLines], uint32_t yValueToMap[maxLines], double linearMapVars[], uint16_t pixelsToDraw[2][maxLines])
{
	//uint16_t pixelPositions[2][maxLines];
	for (int ii = 0; ii < (int)linearMapVars[0]; ii++)	//ii < maxPixels
	{
		pixelsToDraw[0][ii] = (uint16_t)(linearMapVars[2] * xValueToMap[ii] + linearMapVars[1]);	//pixel position = Weight * cartesian value + bias
		pixelsToDraw[1][ii] = (uint16_t)(linearMapVars[4] * yValueToMap[ii] + linearMapVars[3]);
	}
	//return pixelPositions;
}
/*
 *Draw a black pixel at the specififed pixel postition.
 **/
void DrawArrayOfPixels(uint16_t x[maxLines], uint16_t y[maxLines])
{
	for (int ii = 0; ii < maxLines; ii++)
	{
		BSP_LCD_DrawPixel(x[ii], y[ii], LCD_COLOR_BLACK);
	}

}

void DrawArrayOfPixelSegments(uint16_t x[maxLines], uint16_t y[maxLines])
{
	for (int ii = 1; ii < (int)linearMapVars[0]; ii++)
	{
		BSP_LCD_DrawLine(x[ii - 1], y[ii - 1], x[ii], y[ii]);
		//BSP_LCD_DrawPixel(x[ii], y[ii], LCD_COLOR_BLACK);
	}

}
#endif
/**
  * @brief  Regular conversion complete callback in non blocking mode 
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)	//Currently triggers at 1KHz as desired. Taking a sample every 1ms and each line represents 1ms.
{
	/*	//Test to eyeball the frequency of the adc capture frequency
	adcEvents++;
	adcLastConversionTime = HAL_ADC_GetValue(hadc);
	if (adcEvents % 1000 == 0) {
		HAL_GPIO_TogglePin(GPIOG, Pin_LedGreen);
		}
	*/
	adcCurrentTime = HAL_GetTick();
	if ((adcCurrentTime - adcLastTime) >= adcDuration)
	{
		adcLastTime = HAL_GetTick();
		adcDataVolt[adcIndex] = (uint32_t)(HAL_ADC_GetValue(hadc));
		adcDataTime[adcIndex] = adcIndex * adcDuration;		//!! Making a (safe) assumption that (adcCurrentTime - adcLastTime) is always approx adcDuration !!
		adcIndex++;
		if (adcIndex >= (int)linearMapVars[0])		//!! Setting this to a macro rather than a variable changed within SetReg means at coarser time resolutions, the waveform doesn't go accross the entire screen. !!
		{
			adcIndex = 0;
			boolReadyToWriteAdc = 1;
		}
	}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == Pin_Button) {	
		boolButtonPressed = 1;		//Flags oscilloscope when in single-shot mode to convert data using ADC and display it to user.
		
		
		//Quick test for changing the resolution. 
		static int resChanger = 0;
		//SetReg(RegOScopeResY, resChanger % 3);
		//SetReg(RegOScopeResX, resChanger % 3);
		//SetReg(RegOScopeResY, 2);
		resChanger++;
		/**/
	}
}
void DrawGridLines(uint8_t resolutionColumn, uint8_t resolutionRow, uint32_t RGB_Code) 
{
	BSP_LCD_SelectLayer(0);
	BSP_LCD_SetTextColor(RGB_Code);
	static int currentColumn, currentRow;
	for (currentColumn = (resolutionColumn - 1); currentColumn <= maxLines; currentColumn += resolutionColumn)		//Draw columns in landscape
	{
		BSP_LCD_DrawHLine(0, currentColumn, maxPixels);
	}
	for (currentRow = (resolutionRow - 1); currentRow <= maxPixels; currentRow += resolutionRow)					//Draw rows in landscape
	{
		BSP_LCD_DrawVLine(currentRow, 0, maxLines);
	}
	
	static char resolutionDisp[20];
	sprintf(resolutionDisp, "%1dms/d %1dmV/d    ", (int)(resolutionColumn / linearMapVars[2]), (int) ((resolutionRow / linearMapVars[4] * 3000/0xFFF)/10)*10  );	//Conversions are ( pixels/div / (pixel/ms) ) and ( pixels/div / (pixels/adcvalue) * mV/adcvalue and truncate ones place for floating-point error)
	BSP_LCD_DisplayStringAt(0, 300, (uint8_t*)resolutionDisp, LEFT_MODE);			//Write information to display about the time and voltage resolution
}

/* Experimental functions ----------------------------------------------------*/
/*Display the most recently touched pixel position to the user.*/
void DisplayTouchedPixel(void)
{
	BSP_TS_GetState(&ts);
	xPos = ts.X;
	yPos = ts.Y;
	sprintf(xTouchStr, "X: %3d", xPos);
	BSP_LCD_DisplayStringAt(20, 20, (uint8_t *)xTouchStr, LEFT_MODE);
	sprintf(xTouchStr, "Y: %3d", yPos);
	BSP_LCD_DisplayStringAt(20, 60, (uint8_t *)xTouchStr, LEFT_MODE);
}
/*
 *A simple test of making UI buttons. Each button changes the background color. Pixel positions need to be updated from Stm32F7 implementation.
*/
void BackgroundColorChangeButtons(void)
{
	BSP_LCD_DisplayStringAt(40, 20, (uint8_t *)"RED", RIGHT_MODE);
	BSP_LCD_DrawRect(350, 20, 100, 25);
	BSP_LCD_DisplayStringAt(40, 60, (uint8_t *)"Green", RIGHT_MODE);
	BSP_LCD_DrawRect(350, 55, 100, 25);
	BSP_LCD_DisplayStringAt(40, 100, (uint8_t *)"Blue", RIGHT_MODE);
	BSP_LCD_DrawRect(350, 100, 100, 25);
	
	if (350 < xPos && xPos < 450 && 20 < yPos && yPos < 45) {
		BSP_LCD_SetBackColor(LCD_COLOR_RED);
	}
	else if (350 < xPos && xPos < 450 && 55 < yPos && yPos < 80) {
		BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
	}
	else if (350 < xPos && xPos < 450 && 100 < yPos && yPos < 125) {
		BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	}
	else {
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	}
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
