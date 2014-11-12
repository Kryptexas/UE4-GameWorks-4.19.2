// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WidgetCarouselPrivatePCH.h"

void FWidgetCarouselModule::StartupModule()
{
	FWidgetCarouselStyle::Initialize();
}

void FWidgetCarouselModule::ShutdownModule()
{
	FWidgetCarouselStyle::Shutdown();
}

IMPLEMENT_MODULE(FWidgetCarouselModule, WidgetCarousel);
