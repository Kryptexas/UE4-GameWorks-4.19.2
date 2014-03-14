// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateViewerApp.h"
#include "RequiredProgramMainCPPInclude.h"
#include "Runtime/Slate/Private/SWidgetReflector.h"


IMPLEMENT_APPLICATION(SlateViewer, "SlateViewer");


namespace WorkspaceMenu
{
	TSharedRef<FWorkspaceItem> DeveloperMenu = FWorkspaceItem::NewGroup(NSLOCTEXT("SlateViewer", "DeveloperMenu", "Developer"));
}


void RunSlateViewer( const TCHAR* CommandLine )
{
	// start up the main loop
	GEngineLoop.PreInit(CommandLine);

	// crank up a normal Slate application using the platform's standalone renderer
	FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());

	// set the application name
	FGlobalTabmanager::Get()->SetApplicationTitle(NSLOCTEXT("SlateViewer", "AppTitle", "Slate Viewer"));

	// Bring up the test suite.
	RestoreSlateTestSuite();

#if WITH_SHARED_POINTER_TESTS
	SharedPointerTesting::TestSharedPointer<ESPMode::Fast>();
	SharedPointerTesting::TestSharedPointer<ESPMode::ThreadSafe>();
#endif

	// loop while the server does the rest
	while (!GIsRequestingExit)
	{
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
		FPlatformProcess::Sleep(0);
	}

	FSlateApplication::Shutdown();
}
