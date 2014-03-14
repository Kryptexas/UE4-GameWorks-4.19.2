// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICursor.h"

class FHTML5Cursor : public ICursor
{
public:

	FHTML5Cursor();

	virtual ~FHTML5Cursor();

	virtual FVector2D GetPosition() const OVERRIDE;

	virtual void SetPosition( const int32 X, const int32 Y ) OVERRIDE;

	virtual void SetType( const EMouseCursor::Type InNewCursor ) OVERRIDE;

	virtual void GetSize( int32& Width, int32& Height ) const OVERRIDE;

	virtual void Show( bool bShow ) OVERRIDE;

	virtual void Lock( const RECT* const Bounds ) OVERRIDE;


private:
	FVector2D Position;
};