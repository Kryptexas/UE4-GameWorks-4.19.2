// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"
#include "AndroidWindow.h"


namespace FAndroidAppEntry
{
	void PlatformInit();
	void ReInitWindow();
	void DestroyWindow();
}

class FAndroidApplication : public GenericApplication
{
public:

	static FAndroidApplication* CreateAndroidApplication();


public:	
	
	virtual ~FAndroidApplication() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	virtual void PollGameDeviceState( const float TimeDelta ) override;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const override;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDesktopMetrics ) const override;

	virtual TSharedRef< FGenericWindow > MakeWindow() override;
	
	void InitializeWindow( const TSharedRef< FGenericWindow >& InWindow, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately );

private:

	FAndroidApplication();


private:

	TSharedPtr< class FAndroidInputInterface > InputInterface;

	TArray< TSharedRef< FAndroidWindow > > Windows;
};
