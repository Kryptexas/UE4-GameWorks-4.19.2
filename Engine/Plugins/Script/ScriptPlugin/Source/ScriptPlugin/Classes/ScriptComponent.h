// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ScriptAsset.h"
#include "ScriptComponent.generated.h"

/** Script context for this component */
class FScriptContextBase
{
public:
	virtual bool Initialize(class UScriptComponent* Owner) = 0;
	virtual void Tick(float DeltaTime) = 0;
	virtual void Destroy() = 0;
	virtual bool CanTick() = 0;
};

/** Component that allows you to specify custom triangle mesh geometry */
UCLASS(hidecategories=(Object, ActorComponent), editinlinenew, meta=(BlueprintSpawnableComponent), ClassGroup=Script)
class UScriptComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** Script code for this component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Script)
	UScriptAsset* Script;

	// Begin UActorComponent interface.
	virtual void OnRegister() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	// Begin UActorComponent interface.

protected:

	FScriptContextBase* Context;
};


