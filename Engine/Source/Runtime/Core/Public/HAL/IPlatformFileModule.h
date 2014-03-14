// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	IPlatformFileModule.h: Platform file module interface
==============================================================================================*/

#pragma once

/**
 * Platform File Module Interface
 */
class IPlatformFileModule : public IModuleInterface
{
public:

	/**
	 * Creates a platform file instance.
	 *
	 * @return Platform file instance.
	 */
	virtual IPlatformFile* GetPlatformFile() = 0;
};