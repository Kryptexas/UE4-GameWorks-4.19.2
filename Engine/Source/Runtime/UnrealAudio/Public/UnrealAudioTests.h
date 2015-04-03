// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"

namespace UAudio
{
	/**
	* TestDeviceQuery
	*
	* Test which queries input and output devices and displays information to log about device capabilities, format, etc.
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceQuery();

	/**
	* TestDeviceOutputSimple
	*
	* Test which runs a simple sinusoid output on all channels (each channel is one octave higher than left channel)
	* to test that audio output is working and function with no discontinuities.
	* @param TestTime The amount of time to run the test in seconds (negative value means to run indefinitely)
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceOutputSimple(double TestTime);
	
	/**
	* TestDeviceInputSimple
	*
	* Test which test input device functionality by feeding input device data to output device data (pass through).
	* If you have a connected microphone on your system (set to default) you should hear the mic input feed through output speakers.
	* @param TestTime The amount of time to run the test in seconds (negative value means to run indefinitely)
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceInputSimple(double TestTime);
	
	/**
	* TestDeviceOutputRandomizedFm
	*
	* Test which tests output device functionality with a bit more fun than the simple test. You should hear randomized FM synthesis in panning through
	* all connected speaker outputs.
	* @param TestTime The amount of time to run the test in seconds (negative value means to run indefinitely)
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceOutputRandomizedFm(double TestTime);

	/**
	* TestDeviceOutputNoisePan
	*
	* Test which tests output device functionality with white noise that pans clockwise in surround sound setup.
	* @param TestTime The amount of time to run the test in seconds (negative value means to run indefinitely)
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceOutputNoisePan(double TestTime);

	/**
	* TestDeviceInputRandomizedDelay
	*
	* Test input device functionality with a bit more fun that the simple test. You should hear the input data randomly delayed by various amounts
	* with randomized feedback and wetness. Should sound robotic and "interesting".
	* @param TestTime The amount of time to run the test in seconds (negative value means to run indefinitely)
	* @return True if test succeeds.
	*/
	bool UNREALAUDIO_API TestDeviceInputRandomizedDelay(double TestTime);
}

