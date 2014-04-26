// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateModule.cpp: Implements the FSlateModule class.
=============================================================================*/

#include "Slate.h"
#include "SlatePrivatePCH.h"
#include "ModuleManager.h"


/**
 * Implements the Slate module.
 */
class FSlateModule
	: public IModuleInterface
{
};


IMPLEMENT_MODULE(FSlateModule, Slate);
