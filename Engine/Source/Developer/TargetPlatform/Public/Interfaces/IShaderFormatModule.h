// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IShaderFormatModule.h: Declares the IShaderFormatModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for shader format modules.
 */
class IShaderFormatModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the shader format.
	 */
	virtual IShaderFormat* GetShaderFormat() = 0;


protected:

	IShaderFormatModule() { }
};
