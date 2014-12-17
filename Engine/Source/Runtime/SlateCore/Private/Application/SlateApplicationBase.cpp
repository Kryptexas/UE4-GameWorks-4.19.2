// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "HittestGrid.h"
#include "ActiveTickHandle.h"


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

void FSlateApplicationBase::RegisterActiveTick( const TSharedRef<FActiveTickHandle>& ActiveTickHandle )
{
	ActiveTickHandles.Add(ActiveTickHandle);
}

void FSlateApplicationBase::UnRegisterActiveTick( const TSharedRef<FActiveTickHandle>& ActiveTickHandle )
{
	ActiveTickHandles.Remove(ActiveTickHandle);
}

bool FSlateApplicationBase::AnyActiveTicksArePending()
{
	// first remove any tick handles that may have become invalid.
	// If we didn't remove invalid handles here, they would never get removed because
	// we don't force widgets to UnRegister before they are destroyed.
	ActiveTickHandles.RemoveAll([](const TWeakPtr<FActiveTickHandle>& ActiveTickHandle)
	{
		// only check the weak pointer to the handle. Just want to make sure to clear out any widgets that have since been deleted.
		return !ActiveTickHandle.IsValid();
	});

	// The rest are valid. Update their pending status and see if any are ready.
	const double CurrentTime = GetCurrentTime();
	bool bAnyTickReady = false;
	for ( auto& ActiveTickInfo : ActiveTickHandles )
	{
		auto ActiveTickInfoPinned = ActiveTickInfo.Pin();
		check( ActiveTickInfoPinned.IsValid() );

		// If an active tick is still pending execution from last frame, it is collapsed 
		// or otherwise blocked from ticking. Disregard until it executes.
		if ( ActiveTickInfoPinned->IsPendingExecution() )
		{
			continue;
		}

		if ( ActiveTickInfoPinned->UpdateExecutionPendingState( CurrentTime ) )
		{
			bAnyTickReady = true;
		}
	}

	return bAnyTickReady;
}
