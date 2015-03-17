// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavNodeInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UNavNodeInterface : public UInterface
{
	GENERATED_BODY()
public:
	ENGINE_API UNavNodeInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class INavNodeInterface
{
	GENERATED_BODY()
public:


	/**
	 *	Retrieves pointer to implementation's UNavigationGraphNodeComponent
	 */
	virtual class UNavigationGraphNodeComponent* GetNavNodeComponent() PURE_VIRTUAL(FNavAgentProperties::GetNavNodeComponent,return NULL;);

};
