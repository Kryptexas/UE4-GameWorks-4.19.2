// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateApplicationBase.cpp: Implements the FSlateApplicationBase class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

TSharedPtr<FSlateApplicationBase> FSlateApplicationBase::CurrentBaseApplication = nullptr;
TSharedPtr<GenericApplication> FSlateApplicationBase::PlatformApplication = nullptr;
