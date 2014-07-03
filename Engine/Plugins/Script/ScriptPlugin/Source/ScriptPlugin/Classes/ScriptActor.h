// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ScriptBlueprintGeneratedClass.h"
#include "ScriptActor.generated.h"

/**
* Script-extendable actor class
*/
UCLASS(BlueprintType, Abstract, EarlyAccessPreview)
class SCRIPTPLUGIN_API AScriptActor : public AActor
{
	GENERATED_UCLASS_BODY()

	// UObject interface
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;

	// AActor interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void ReceiveDestroyed() override;
	virtual void ReceiveEndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 
	 * Invokes script-defined function
	 */
	void InvokeScriptFunction(FFrame& Stack, RESULT_DECL);

private:

	/** Script context (code) for this actor */
	FScriptContextBase* ScriptContext;
};
