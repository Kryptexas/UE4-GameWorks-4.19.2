// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GoogleVRTransition2D.h"
#include "GoogleVRTransition2DBPLibrary.h"

#define LOCTEXT_NAMESPACE "FGoogleVRTransition2DModule"

void FGoogleVRTransition2DModule::StartupModule()
{
	UGoogleVRTransition2DBPLibrary::Initialize();
}

void FGoogleVRTransition2DModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGoogleVRTransition2DModule, GoogleVRTransition2D)