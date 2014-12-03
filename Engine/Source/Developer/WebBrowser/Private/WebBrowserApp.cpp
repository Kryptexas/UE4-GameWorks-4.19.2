// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserApp.h"

#if WITH_CEF3
FWebBrowserApp::FWebBrowserApp()
{
}

void FWebBrowserApp::OnContextInitialized()
{
	// Called on the browser process UI thread immediately after the CEF context
	// has been initialized.
}
#endif
