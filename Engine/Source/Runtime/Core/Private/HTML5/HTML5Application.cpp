// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "HTML5Application.h"
#include "HTML5Cursor.h"
#include "HTML5InputInterface.h"

FHTML5Application* FHTML5Application::CreateHTML5Application()
{
	return new FHTML5Application();
}

FHTML5Application::FHTML5Application()
	: GenericApplication( MakeShareable( new FHTML5Cursor() ) )
	, InputInterface( FHTML5InputInterface::Create( MessageHandler, Cursor ) )
{
}

void FHTML5Application::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler(InMessageHandler);
	InputInterface->SetMessageHandler( MessageHandler );
}

void FHTML5Application::PollGameDeviceState( const float TimeDelta )
{
	// Poll game device state and send new events
	InputInterface->Tick( TimeDelta );
	InputInterface->SendControllerEvents();
}

FPlatformRect FHTML5Application::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	//@todo HTML5: Use the actual device settings here.
	FPlatformRect WorkArea;
	WorkArea.Left = 0;
	WorkArea.Top = 0;
	WorkArea.Right = 800;
	WorkArea.Bottom = 600;

	return WorkArea;
}

void FHTML5Application::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	//@todo HTML5: Use the actual device settings here.
	OutDisplayMetrics.PrimaryDisplayWidth = 800;
	OutDisplayMetrics.PrimaryDisplayHeight = 600;

	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = 0;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = 0;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = 800;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = 600;

	OutDisplayMetrics.VirtualDisplayRect.Left = 0;
	OutDisplayMetrics.VirtualDisplayRect.Top = 0;
	OutDisplayMetrics.VirtualDisplayRect.Right = 800;
	OutDisplayMetrics.VirtualDisplayRect.Bottom = 600;
}
