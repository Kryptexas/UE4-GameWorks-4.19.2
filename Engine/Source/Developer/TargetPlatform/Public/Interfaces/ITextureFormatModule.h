// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITextureFormatModule.h: Declares the ITextureFormatModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for texture format modules.
 */
class ITextureFormatModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the texture format.
	 */
	virtual ITextureFormat* GetTextureFormat() = 0;


protected:

	ITextureFormatModule() { }
};
