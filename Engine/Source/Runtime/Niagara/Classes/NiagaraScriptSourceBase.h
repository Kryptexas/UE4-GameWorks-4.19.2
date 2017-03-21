// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NiagaraCommon.h"
#include "NiagaraScriptSourceBase.generated.h"

struct EditorExposedVectorConstant
{
	FName ConstName;
	FVector4 Value;
};

struct EditorExposedVectorCurveConstant
{
	FName ConstName;
	class UCurveVector *Value;
};

/** Runtime data for a Niagara system */
UCLASS(MinimalAPI)
class UNiagaraScriptSourceBase : public UObject
{
	GENERATED_UCLASS_BODY()

	TArray<TSharedPtr<EditorExposedVectorConstant> > ExposedVectorConstants;
	TArray<TSharedPtr<EditorExposedVectorCurveConstant> > ExposedVectorCurveConstants;

	/** Implements compilation of a Niagara script.*/
	virtual ENiagaraScriptCompileStatus Compile(FString& OutGraphLevelErrorMessages) { return ENiagaraScriptCompileStatus::NCS_Unknown; }

	/** Determines if the input change id is equal to the current source graph's change id.*/
	virtual bool IsSynchronized(const FGuid& InChangeId) { return true; }

	/** Determine if there are any external dependencies wrt to scripts and ensure that those dependencies are sucked into the existing package.*/
	virtual void SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions) {}

	/** Enforce that the source graph is now out of sync with the script.*/
	virtual void MarkNotSynchronized() {}
};
