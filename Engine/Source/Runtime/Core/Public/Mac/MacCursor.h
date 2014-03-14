// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICursor.h"

#ifndef __OBJC__
class NSCursor;
#endif

class FMacCursor : public ICursor
{
public:

	FMacCursor();

	virtual ~FMacCursor();

	virtual FVector2D GetPosition() const OVERRIDE;

	virtual void SetPosition( const int32 X, const int32 Y ) OVERRIDE;

	virtual void SetType( const EMouseCursor::Type InNewCursor ) OVERRIDE;

	virtual void GetSize( int32& Width, int32& Height ) const OVERRIDE;

	virtual void Show( bool bShow ) OVERRIDE;

	virtual void Lock( const RECT* const Bounds ) OVERRIDE;


public:

	bool UpdateCursorClipping( FVector2D& CursorPosition );
	
	void WarpCursor( const int32 X, const int32 Y );
	
	FVector2D GetMouseWarpDelta( bool const bClearAccumulatedDelta );


private:

	void UpdateVisibility();

	/** Cursors */
	NSCursor* CursorHandles[EMouseCursor::TotalCursorCount];

	FIntRect CusorClipRect;

	bool bIsVisible;
	NSCursor* CurrentCursor;
	
	FVector2D MouseWarpDelta;
};
