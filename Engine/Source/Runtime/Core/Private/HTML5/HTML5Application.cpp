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
	, ApplicationWindow(FHTML5Window::Make())
{
}

void FHTML5Application::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler(InMessageHandler);
	InputInterface->SetMessageHandler( MessageHandler );
}

void FHTML5Application::PollGameDeviceState( const float TimeDelta )
{
	SDL_Event Event;
	while (SDL_PollEvent(&Event)) 
	{
		// Tick Input Interface. 
		if ( Event.type == SDL_VIDEORESIZE )
		{
			SDL_ResizeEvent *r = (SDL_ResizeEvent*)&Event;
			MessageHandler->OnSizeChanged(  ApplicationWindow, r->w,r->h); 
		}
		else 
		{
			InputInterface->Tick( TimeDelta, Event );
		}
	}
	InputInterface->SendControllerEvents();
}

FPlatformRect FHTML5Application::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	return  FHTML5Window::GetScreenRect();
}

void FHTML5Application::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect = FHTML5Window::GetScreenRect();

	OutDisplayMetrics.VirtualDisplayRect = OutDisplayMetrics.PrimaryDisplayWorkAreaRect;
	// Total screen size of the primary monitor - or in HTML5's case- total size of the webpage. Make it arbitrary large. 
	OutDisplayMetrics.PrimaryDisplayWidth = 4096;
	OutDisplayMetrics.PrimaryDisplayHeight = 4096; 

}

TSharedRef< FGenericWindow > FHTML5Application::MakeWindow()
{
	return ApplicationWindow;
}
