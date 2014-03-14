// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HeadMountedDisplay.h"

class FHeadMountedDisplayModule : public IHeadMountedDisplayModule
{
	virtual TSharedPtr< class IHeadMountedDisplay > CreateHeadMountedDisplay()
	{
		TSharedPtr<IHeadMountedDisplay> DummyVal = NULL;
		return DummyVal;
	}
};

IMPLEMENT_MODULE( FHeadMountedDisplayModule, HeadMountedDisplay );

IHeadMountedDisplay::IHeadMountedDisplay()
{
	PreFullScreenRect = FSlateRect(-1.f, -1.f, -1.f, -1.f);
}

void IHeadMountedDisplay::PushPreFullScreenRect(const FSlateRect& InPreFullScreenRect)
{
	PreFullScreenRect = InPreFullScreenRect;
}

void IHeadMountedDisplay::PopPreFullScreenRect(FSlateRect& OutPreFullScreenRect)
{
	OutPreFullScreenRect = PreFullScreenRect;
	PreFullScreenRect = FSlateRect(-1.f, -1.f, -1.f, -1.f);
}