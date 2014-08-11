// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AI/Navigation/NavRelevantComponent.h"
#include "NavModifierComponent.generated.h"

class UNavArea;
struct FCompositeNavModifier;

UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent), hidecategories = (Activation))
class UNavModifierComponent : public UNavRelevantComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Navigation)
	TSubclassOf<UNavArea> AreaClass;

	virtual void OnRegister() override;
	virtual void OnApplyModifiers(FCompositeNavModifier& Modifiers) override;
	virtual void OnOwnerRegistered() override;
	virtual void OnOwnerUnregistered() override;

protected:
	FVector ObstacleExtent;
};
