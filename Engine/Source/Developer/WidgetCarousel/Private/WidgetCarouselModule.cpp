// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WidgetCarouselPrivatePCH.h"

void FWidgetCarouselModule::StartupModule()
{
	FWidgetCarouselModuleStyle::Initialize();
}

void FWidgetCarouselModule::ShutdownModule()
{
	FWidgetCarouselModuleStyle::Shutdown();
}

IMPLEMENT_MODULE(FWidgetCarouselModule, WidgetCarousel);
