// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIPerceptionListenerInterface.generated.h"

class UAIPerceptionComponent;

UINTERFACE()
class AIMODULE_API UAIPerceptionListenerInterface : public UInterface
{
	GENERATED_BODY()
public:
	UAIPerceptionListenerInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class AIMODULE_API IAIPerceptionListenerInterface
{
	GENERATED_BODY()
public:

	virtual UAIPerceptionComponent* GetPerceptionComponent() { return NULL; }
};

