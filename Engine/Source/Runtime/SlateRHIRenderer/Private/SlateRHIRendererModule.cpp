// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "SlateRHIRendererModule.h"
#include "SlateRHIRenderer.h"

void FSlateRHIRendererModule::StartupModule()
{

}

void FSlateRHIRendererModule::ShutdownModule()
{

}

TSharedRef<FSlateRenderer> FSlateRHIRendererModule::CreateSlateRHIRenderer()
{
	return TSharedRef<FSlateRHIRenderer>( new FSlateRHIRenderer );
}

IMPLEMENT_MODULE( FSlateRHIRendererModule, SlateRHIRenderer ) 
