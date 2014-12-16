// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "HittestGrid.h"


/* Static initialization
 *****************************************************************************/

TSharedPtr<FSlateApplicationBase> FSlateApplicationBase::CurrentBaseApplication = nullptr;
TSharedPtr<GenericApplication> FSlateApplicationBase::PlatformApplication = nullptr;
// TODO: Identifier the cursor index in a smarter way.
const uint32 FSlateApplicationBase::CursorPointerIndex = EKeys::NUM_TOUCH_KEYS - 1;

FWidgetPath FHitTesting::LocateWidgetInWindow(FVector2D ScreenspaceMouseCoordinate, const TSharedRef<SWindow>& Window, bool bIgnoreEnabledStatus) const
{
	return SlateApp->LocateWidgetInWindow(ScreenspaceMouseCoordinate, Window, bIgnoreEnabledStatus);
}


FSlateApplicationBase::FSlateApplicationBase()
: Renderer()
, HitTesting(this)
{

}

const FHitTesting& FSlateApplicationBase::GetHitTesting() const
{
	return HitTesting;
}