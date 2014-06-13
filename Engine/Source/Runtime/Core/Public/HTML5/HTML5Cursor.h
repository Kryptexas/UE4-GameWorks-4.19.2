// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICursor.h"

class FHTML5Cursor : public ICursor
{
public:

	FHTML5Cursor();

	virtual ~FHTML5Cursor();

	virtual FVector2D GetPosition() const override;

	virtual void SetPosition( const int32 X, const int32 Y ) override;

	virtual void SetType( const EMouseCursor::Type InNewCursor ) override;

	virtual void GetSize( int32& Width, int32& Height ) const override;

	virtual void Show( bool bShow ) override;

	virtual void Lock( const RECT* const Bounds ) override;


private:
	FVector2D Position;
};