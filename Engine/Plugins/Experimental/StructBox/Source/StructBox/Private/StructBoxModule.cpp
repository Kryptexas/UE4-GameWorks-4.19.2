// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IStructBoxModule.h"

class FStructBoxModule : public IStructBoxModuleInterface
{

};

IMPLEMENT_MODULE(FStructBoxModule, StructBox);
