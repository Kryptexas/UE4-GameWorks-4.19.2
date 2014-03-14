// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IAudioFormatModule.h: Declares the IAudioFormatModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for audio format modules.
 */
class IAudioFormatModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the audio format.
	 */
	virtual IAudioFormat* GetAudioFormat() = 0;


protected:

	IAudioFormatModule() { }
};
