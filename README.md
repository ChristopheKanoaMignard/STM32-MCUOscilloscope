# STM32F4-McuOscilloscope
## Overview
This project started with my desire to learn more about how to program LCD screens and related peripherals. While this repository contains traces of my early tinkering, it was quickly subsumed into the more interesting and focused task of creating my own oscilloscope. The oscilloscope is designed to convert an external analogue signal-- in my case from a waveform generator or from the integrated DAC--into digital signals using an ADC. Then, to display the signal on the LCD, I used a linear transformation to map the time and signal voltage onto x- and y-pixel positions respectively. Then line segments are drawn between these data points using drawing functions from the BSP library. 
[waveform and scope image]

## Oscilloscope Features
Eventually I intend to return to this project and create an actual user interface for this project, but for now this section will describe the various oscilloscope features that are available and how they can be edited. Some features can be changed during runtime by writing to certain registers from the python file, but most are not user friendly and require editing the firmware. 

Execution of the code occurs within a forever loop which should normally call either OscilloscopeWriteAdcToDisplayContinuous() or OscilloscopeWriteAdcToDisplaySingle() depending on the desired mode of operation. While in continuous mode, the display refreshes constantly; in single shot mode, the blue user button must be pressed to take a sample. 

In either mode, the resolution of the voltage or time axis can be toggled between default, fine, and coarse modes. For voltage, these resolutions are 500, 200, and 1000 mV/division and and for time they are 50, 20, and 200 ms/division. The resolutions can be changed during runtime by writing 0, 1, or 2 for default, fine, or coarse into the registers RegOScopeResX, or RegOScopeResY from the python file. The current resolution is displayed on the far right side of the display, as pictured below.
[Resolution changing Images]

By default the color scheme is a blue waveform on a white background with grey gridlines. If desired, these colors can be altered by changing the variables colorWaveform, colorBackground, and colorGridlines inside the oscilloscope.c file. 

The oscilloscope naturally uses the display in a horizontal orientation, but this can be changed to a verticle orientation by commenting out "#define LandscapeMode" in the oscilloscope.h file. The vertical configuration is only included as a proof of concept because that orientation looks unnatural and was thus not maintained as more features were added. So while displaying oscilloscope signals in continuous or single shot mode works, the resolution cannot be changed from default and the resolution is not displayed correctly.



## Oscilloscope Details
### Linear Mapping

In order to display a waveform, it is necessary to convert its voltage and time to pixel positions on the display via a linear map. Both the adc and the dac are 12 bit registers--thus the max value of $0xFFF$ corresponds to the GPIO max voltage of $3 V$--while there are 300 pixels along the x-axis and 240 along the y-axis. Technically, there are 320 pixels along the x-axis, but the last 20 are reserved to display the current resolution. Also note that the LCD on the F4 board is intended to operate in a vertical orientation, so by rotating it 90 degrees clockwise, the upper left pixel becomes $[0, 239]$ rather than $[0, 0].$ 

Since our linear transformation does not involve any rotations, we can simply express the transformation as $$ñ = w_n * n + b_n.$$ 
For the x-direction we want to map the time range $[0, 299]ms$ onto the pixel range $[299, 0]p$ and for the y-direction we want to map the voltage range $[0, 3]V$ onto the pixel range $[0, 239]p.$ By solving this system of equations, we get the mapping parameters:

$$\begin{aligned} 
299ms = w_{x}* 0p + b_{x}     &⇒ b_{x}=299ms  \\
0ms   = w_{x} * 299p + b_{x}  &⇒ w_{x}=-1ms/p  \\
0V    = w_{y} * 0p + b_{y}    &⇒ b_{y}=0V  \\
3V    = w_{y} * 239p + b_{y}  &⇒ w_{y}=3V/239p=0.01255 V/p
\end{aligned}$$

To get my oscilloscope working initially, I hardcoded these values; but for extra flexibility and functionality, I quickly switched to handling this in code with editable variables.

void CalculateLinearMapVars(uint16_t minPixelPosX, uint16_t maxPixelPosX, double minValueX, double maxValueX, uint16_t minPixelPosY, uint16_t maxPixelPosY, double minValueY, double maxValueY, double linearMapVars[])
{
	linearMapVars[1] = (maxValueX*minPixelPosX - maxPixelPosX*minValueX) / (maxValueX - minValueX);
	linearMapVars[2] = (maxPixelPosX - minPixelPosX)                     / (maxValueX - minValueX);
	linearMapVars[3] = (maxValueX*minPixelPosY - maxPixelPosX*minValueY) / (maxValueY - minValueY);
	linearMapVars[4] = (maxPixelPosY - minPixelPosY)                     / (maxValueY - minValueY);
}


### Aliasing
Like all digital oscilloscopes, this one is susceptible to aliasing. Aliasing is produced when a signal's frequency is close to a whole number multiple of the sampling rate. The signal is sampled in one location, and the next sample is actually on the next period but has a magnitude near to what it should have been. This effect can be seen below. 

### LCD Properties
H-Sync, V-Sync, etc
I am including these properites here simply because the information was so difficult to find online. In the end, initialization functions in the BSP library made this knowledge a moot point. However, they may prove useful again at some point. 

## Extraneous Features
Before implementing the oscilloscope, I started off by experimenting with the display and eventually the DAC. While the following functions serve no purpose for the oscilloscope, they may still be useful for testing the building blocks of the oscilloscope or as a reminder for how the peripherals work before being combined into the oscilloscope. 

 - BackgroundColorChangeButtons(): This draws three colored boxes in the upper right corner of the screen. When one is pressed, the background color is changed to that color.
 - DisplayTouchedPixel(): On the display, writes the x- and y-position of the most recently touched pixel.
 - ManuallyDrawSinWaveform(double width, double amplitude): Draws a sin wave from 0 to width on the display without using DAC or ADC.
 - DacEscallatorWaveform(int boolWriteDisplay): Writes an escallator waveform into the DAC, then reads those values from the DAC and displays them on the display, all continuously in real-time.
