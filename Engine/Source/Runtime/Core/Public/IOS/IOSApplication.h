// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"
#include "IOSWindow.h"

class FIOSApplication : public GenericApplication
{
public:

	static FIOSApplication* CreateIOSApplication();


public:

	virtual ~FIOSApplication() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	virtual void PollGameDeviceState( const float TimeDelta ) OVERRIDE;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

	virtual TSharedRef< FGenericWindow > MakeWindow() OVERRIDE;

protected:
	virtual void InitializeWindow( const TSharedRef< FGenericWindow >& Window, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately ) OVERRIDE;

private:

	FIOSApplication();


private:

	TSharedPtr< class FIOSInputInterface > InputInterface;

	TArray< TSharedRef< FIOSWindow > > Windows;
};
