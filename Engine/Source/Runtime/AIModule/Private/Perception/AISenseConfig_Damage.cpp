// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"

TSubclassOf<UAISense> UAISenseConfig_Damage::GetSenseImplementation() const
{
	return Implementation;
}
