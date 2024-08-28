/**
  ******************************************************************************
  * @file           : regs.c
  * @brief          : Code to handle registers that do not survive power cycle
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "regs.h"
#include "spi.h"
#include "oscilloscope.h"

volatile REG_BLOCK Regs;
extern uint16_t FirmWareVersion;
extern SPI_HandleTypeDef hspi2;
extern ADC_HandleTypeDef hadc1;


uint16_t ReadAdc(uint16_t chan);


void InitRegs() {
	//Power-on intitializations
	Regs.u16[RegOScopeResX], Regs.u16[RegOScopeResX] = 50;		//default values for the oscilloscope grid lines. Can be changed by writing to RegOscScopeResX, RegOscScopeResY
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
		Error_Handler();

}	

//UpdateRegs called periodically in main()
uint8_t UpdateRegs() {
    uint8_t err = 0;
    static uint16_t nReg=0;
    
    err = ReadReg(nReg);
    nReg++;
    if (nReg>=RegLast || nReg>=REG_SIZE16) nReg = 0;
    
    return err;
}

uint8_t ReadReg(uint16_t nReg) {
	//This routine called if a read register command received over USB, or periodically to update volatile registers.
	//In most cases registers are always valid, but for some the value needs to be updated.
    uint16_t temp = 0, limSw = 0;
    uint8_t err = 0;
    
    if (nReg>=RegLast || nReg>=REG_SIZE16) return 0; //not an error == just don't read past the end of the register block
    
    switch (nReg) {
        case RegFirmWareVersion:
	        Regs.u16[nReg] = FirmWareVersion;
            break;
		case RegUniqueID:
			Regs.u16[nReg] = *(uint16_t *)(0x1fff7a10UL);
			break;
		case RegTick:
			Regs.u16[nReg] = (uint16_t)(HAL_GetTick() & 0xFFFF);
	        break;
		case RegAdcTemp:
			//Regs.u16[nReg] = ReadAdc(16); //temperature sensor is channel 16, 0.76V @25degC + 2.5mV/degC, temp = 25+(nAdc*3.3/4095-0.76)/0.025, 1022 => 27.5degC
			break;
		case RegAdcRef:
			//Regs.u16[nReg] = ReadAdc(17); //internal reference voltage (1.2Vnom), should be around 1.2/3.3*4095 = 1500, can use to calculate voltage of 3.3V supply
			break;
		
		default:
			break;
    }  
    return err;
}
void SetReg(uint16_t nReg, uint16_t value) 
{   
    int16_t sval;
	uint16_t dummyVal;
	//This routine called if a write register command received over USB
    if (nReg>=RegLast || nReg>=REG_SIZE16) return;
    
    switch (nReg) {
	//Use CalculateLinearMapVars() instead of this jankiness
    case RegOScopeResX:			//make sure value is 0, 1, or 2. Normal is 0; Fine is odd; Coarse is even
	    if (value == 0)
	    {
		    linearMapVars[2] = weightX;
		    oscopeGridResolutionX = oscopeGridResolutionXDefault;
		    linearMapVars[0] = (double)(numPixelsToWriteDefault);
	    }
	    else if (value % 2 == 1)	//if odd, become more fine
	    {
		    //linearMapVars[2] = weightX * 100/300;	//could multiply by an additional ((int)(value / 2)+1) if I wanted to allow finer resolutions to each be double the previous fineness
		   linearMapVars[2] = weightX * 3;
		    oscopeGridResolutionX = oscopeGridResolutionXChanged;
		    linearMapVars[0] = (double)(numPixelsToWriteDefault/2);
		
	    }
	    else if(value % 2 == 0)	//if even, become more coarse
	    {
		    linearMapVars[2] = weightX * 3/10;	//could multiply by an additional ((int)(value / 2)) if I wanted to allow coarser resolutions to each be double the previous courseness
		    oscopeGridResolutionX = oscopeGridResolutionXChanged;
		    linearMapVars[0] = (double)(maxLines);
	    }
			break;
    case RegOScopeResY:
	    //Regs.u16[nReg] = value;
	    //linearMapVars[4] = weightY * (0.01*Regs.u16[nReg]);		//Scale native y-axis resolution by the newly set value of this reg
	    //oscopeGridResolutionY = oscopeGridResolutionYDefault * (0.01*Regs.u16[nReg]);	//Scale default grid lines by the entered percent
	    if (value == 0)
	    {
		    linearMapVars[4] = weightY;
		    oscopeGridResolutionY = oscopeGridResolutionYDefault;
	    }
	    else if (value % 2 == 1)	//if odd, become more fine
	    {
		    linearMapVars[4] = weightY * 3;	//could divide by an additional ((int)(value / 2)+1) if I wanted to allow finer resolutions to each be double the previous fineness
		    oscopeGridResolutionY = oscopeGridResolutionYChanged;
	    }
	    else if(value % 2 == 0)	//if even, become more coarse
	    {
		    linearMapVars[4] = weightY * 3/5;	//could multiply by an additional ((int)(value / 2)) if I wanted to allow coarser resolutions to each be double the previous courseness
		    oscopeGridResolutionY = oscopeGridResolutionYChanged;
	    }
		    break;

		default :
			break;
    }
    
    ReadReg(nReg);
}

uint16_t ReadAdc(uint16_t chan)
{
	ADC_ChannelConfTypeDef sConfig = { 0 };
	if (16 == chan)
		sConfig.Channel = ADC_CHANNEL_16;
	else if (17 == chan)
		sConfig.Channel = ADC_CHANNEL_17;
	else
		sConfig.Channel = chan;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);	
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 100);
	uint16_t retVal = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	return (retVal);
}


