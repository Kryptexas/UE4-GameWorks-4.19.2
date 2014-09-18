// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

TSharedPtr<FSlateApplicationBase> FSlateApplicationBase::CurrentBaseApplication = nullptr;
TSharedPtr<GenericApplication> FSlateApplicationBase::PlatformApplication = nullptr;
const uint32 FSlateApplicationBase::CursorPointerIndex = 1000;
