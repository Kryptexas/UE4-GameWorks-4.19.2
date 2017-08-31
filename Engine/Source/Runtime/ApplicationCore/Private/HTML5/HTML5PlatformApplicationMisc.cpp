// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HTML5PlatformApplicationMisc.h"
#include "HTML5Application.h"
#include "Modules/ModuleManager.h"

void FHTML5PlatformApplicationMisc::PostInit()
{
	FModuleManager::Get().LoadModule("MapPakDownloader");
}

GenericApplication* FHTML5PlatformApplicationMisc::CreateApplication()
{
	return FHTML5Application::CreateHTML5Application();
}
