// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"

/**
 * HTML5-specific application implementation.
 */
class FHTML5Application : public GenericApplication
{
public:

	static FHTML5Application* CreateHTML5Application();


public:	

	virtual ~FHTML5Application() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	virtual void PollGameDeviceState( const float TimeDelta ) OVERRIDE;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDesktopMetrics ) const OVERRIDE;


private:

	FHTML5Application();


private:

	TSharedPtr< class FHTML5InputInterface > InputInterface;
};