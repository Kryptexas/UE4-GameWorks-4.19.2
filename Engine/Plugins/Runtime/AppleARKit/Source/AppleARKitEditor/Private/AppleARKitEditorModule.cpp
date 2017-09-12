// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

// UE4
#include "ModuleManager.h"
#include "Features/IModularFeature.h"

class FAppleARKitEditorModule : public IModuleInterface, public IModularFeature
{

};

IMPLEMENT_MODULE(FAppleARKitEditorModule, AppleARKitEditor);
