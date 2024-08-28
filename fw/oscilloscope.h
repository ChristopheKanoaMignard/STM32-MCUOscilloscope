/**
  ******************************************************************************
  * @file           : oscilloscope.h
  * @brief          : Code to read and write waveforms to display
  ******************************************************************************
  */
#ifndef __OSCILLOSCOPE_H__
#define __OSCILLOSCOPE_H__

#ifdef __cplusplus
extern "C" {
#endif
	
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

/* Defines ---------------------------------------------------------------------*/	
#define LandscapeMode				//Works best in landscape Mode. Comment out to operate in vertical mode
	
#define maxPixels					((uint16_t) 240)
#define maxLines					((uint16_t) 320)
	
#define maxXValue					((double)(240-1))	
#define minXValue					((double) 0)
#define maxYValue					((double)(0xFFF))	//adc and dac are both 12 bit regs, maxing out at 0xFFF
#define minYValue					((double) 0)
	
#define oscopeGridResolutionXDefault	((uint8_t) 50)		//6 divisions on screen
#define oscopeGridResolutionYDefault	((uint8_t) 40)		//6 divisions on screen	
#define oscopeGridResolutionXChanged	((uint8_t) 60)		//5 divisions on screen	
#define oscopeGridResolutionYChanged	((uint8_t) 48)		//5 divisions on screen
	
#ifdef LandscapeMode
#define numPixelsToWriteDefault			((uint16_t)(320/2))			  
#endif
	
#ifndef LandscapeMode
#define numPixelsToWrite			((uint16_t)(240/3))			  
#endif
	
/* Exported variables -------------------------------------------------------*/
extern double linearMapVars[5];				//{ numPixelsToWrite, biasX, weightX, biasY, weightY };
extern double weightX, weightY;
extern uint8_t oscopeGridResolutionX, oscopeGridResolutionY;
/* Exported functions prototypes ---------------------------------------------*/
void OscilloscopeDisplayDefaultSetup(void);
void ManuallyDrawSinWaveform(double width, double amplitude);
void DacEscallatorWaveform(int boolWriteDisplay);
void OscilloscopeWriteAdcToDisplayContinuous(void);
void OscilloscopeWriteAdcToDisplaySingle(void);
void DrawGridLines(uint8_t resolutionColumn, uint8_t resolutionRow, uint32_t RGB_Code);
void CalculateLinearMapVars(uint16_t minPixelPosX, uint16_t maxPixelPosX, double minValueX, double maxValueX, uint16_t minPixelPosY, uint16_t maxPixelPosY, double minValueY, double maxValueY, double linearMapVars[]);
void DisplayTouchedPixel(void);
void BackgroundColorChangeButtons(void);
#ifdef __cplusplus
}
#endif

#endif // __OSCILLOSCOPE_H__