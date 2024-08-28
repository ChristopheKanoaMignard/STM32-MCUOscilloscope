# STM32F4-McuOscilloscope
## Overview
In order to familiarize myself with DAC, ADC, and display related peripherals, I created an oscilloscope that can accept signals from a waveform generator or the integrated DAC and display them on the integrated lcd. The oscilloscope works by linear mapping signal time and signal voltage to x- and y-pixel positions respectively. Then line segments are drawn between these data points using drawing functions from the BSP library. 
[waveform and scope image]

## Oscilloscope Features
Eventually I intend to return to this project and create an actual user interface for this project, but for now this section will describe the various oscilloscope features that are available and how they can be edited. Some features can be changed during runtime by writing to certain registers inside the .py file, but most are not user friendly and require editing the firmware. 

Execution of the code occurs within a forever loop which should normally call either OscilloscopeWriteAdcToDisplayContinuous() or OscilloscopeWriteAdcToDisplaySingle() depending on the desired mode of operation. There are also commented-out functions inside the while loop unrelated to the oscilloscope itself from earlier in development to test the various peripherals in isolation that are discussed at the end of this subsection. While in continuous mode, the display refreshes constantly and in single shot mode, the blue user button must be pressed to take a sample. 

In either mode, the resolution of the voltage or time axis can be toggled between default, fine, and coarse modes. For voltage, these resolutions are 500, 200, and 1000 mV/division and and for time they are 50, 20, and 200 ms/division. The resolutions can be changed during runtime by writing 0, 1, or 2 for default, fine, or coarse into the registers RegOScopeResX, or RegOScopeResY from the .py file. The current resolution is displayed on the far right side of the display, as pictured below.
[Resolution changing Images]

By default the color scheme is a blue waveform on a white background with grey gridlines. If desired, these colors can be altered by changing the variables colorWaveform, colorBackground, and colorGridlines inside the oscilloscope.c file. 

The oscilloscope naturally uses the display in a horizontal orientation, but this can changed to a verticle orientation by commenting out #define LandscapeMode in the oscilloscope.h file. The vertical configuration is only included as a proof of concept because that orientation looks unnatural and was thus not maintained as more features were added. So while displaying oscilloscope signals in continuous or single shot mode works, the resolution cannot be changed from default and the resolution is not displayed correctly.

Before implementing the oscilloscope, I started off by experimenting with the display and eventually the DAC. While the following functions serve no purpose for the oscilloscope, they may still be useful for testing the building blocks of the oscilloscope or as a reminder for how the peripherals work before being combined into the oscilloscope. 

 - BackgroundColorChangeButtons(): This draws three colored boxes in the upper right corner of the screen. One is pressed, the background color is changed to that color.
 - DisplayTouchedPixel(): Writes on the display the x- and y-position of the most recently touched pixel.
 - ManuallyDrawSinWaveform(double width, double amplitude): Draws a sin wave from 0 to width on the display without using DAC or ADC.
 - DacEscallatorWaveform(int boolWriteDisplay): Writes a waveform into the DAC in real time, then reads those values from the DAC and displays them on the display. 

## Oscilloscope Details
### Linear Mapping
In order to display a waveform, it is necessary to convert its voltage and time to pixel positions on the display via a linear map. Both the adc and the dac are 12 bit registers--thus the max value of $0xFFF$ corresponds to the GPIO max voltage of $3 V$--while there are 300 pixels along the x-axis and 240 along the y-axis. Technically, there are 320 pixels along the x-axis, but the last 20 are reserved to display the current resolution. Also note that the LCD on the F4 board is intended to operate in a vertical orientation, so by rotating it 90 degrees clockwise, the upper left pixel becomes $[0, 239]$ rather than $[0, 0].$ 

Since our linear transformation does not involve any rotations, we can simply express taking the transformation as 
$$ñ = w_n*n + b_n.$$
For the x-direction we want to map the time range $[0, t]$ onto the pixel range $[299, 0]$ and for the y-direction we want to map the voltage range $[0, 3]$ onto the pixel range $[0, 239].$ By solving this system of equations, we get the mapping parameters:
$$299 = w_x*0 + b_x---->b_x=299$$
$$0 =   w_x*t + b_x --->w_x=b_x/t=alpha$$
$$0 = w_y*0+b_y--->b_y=0$$
$$3 = w_y*239+b_y--->w_y=3/239=0.01255$$

### LCD Properties
H-Sync, V-Sync, etc
