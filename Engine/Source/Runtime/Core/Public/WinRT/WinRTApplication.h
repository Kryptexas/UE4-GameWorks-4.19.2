// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"

class FWinRTCursor : public ICursor
{
public:

	virtual FVector2D GetPosition() const OVERRIDE;

	virtual void SetPosition( const int32 X, const int32 Y ) OVERRIDE;

	void UpdatePosition( const FVector2D& NewPosition );

	virtual void SetType( const EMouseCursor::Type InNewCursor ) OVERRIDE;

	virtual void GetSize( int32& Width, int32& Height ) const OVERRIDE;

	virtual void Show( bool bShow ) OVERRIDE;

	virtual void Lock( const RECT* const Bounds ) OVERRIDE;


private:

	FVector2D CursorPosition;
};

class FWinRTApplication : public GenericApplication
{
public:

	static FWinRTApplication* CreateWinRTApplication();

	static FWinRTApplication* GetWinRTApplication();


public:	

	virtual ~FWinRTApplication() {}

	virtual void PollGameDeviceState( const float TimeDelta ) OVERRIDE;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

	TSharedRef< class FGenericApplicationMessageHandler > GetMessageHandler() const;

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) OVERRIDE;

	TSharedRef< class FWinRTCursor > GetCursor() const;


private:

	FWinRTApplication();


private:

	TSharedPtr< class FWinRTInputInterface > InputInterface;

	FVector2D CursorPosition;
};