// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraNodeWithDynamicPins.h"
#include "SGraphPin.h"
#include "NiagaraNodeParameterMapBase.generated.h"

class UEdGraphPin;


/** A node which allows the user to build a set of arbitrary output types from an arbitrary set of input types by connecting their inner components. */
UCLASS()
class UNiagaraNodeParameterMapBase : public UNiagaraNodeWithDynamicPins
{
public:
	GENERATED_BODY()
	UNiagaraNodeParameterMapBase();

	/** Traverse the graph looking for the history of the parameter map specified by the input pin. This will return the list of variables discovered, any per-variable warnings (type mismatches, etc)
		encountered per variable, and an array of pins encountered in order of traversal outward from the input pin.
	*/
	static TArray<FNiagaraParameterMapHistory> GetParameterMaps(UNiagaraNodeOutput* InGraphEnd, bool bLimitToOutputScriptType = false, FString EmitterNameOverride = TEXT(""));
	static TArray<FNiagaraParameterMapHistory> GetParameterMaps(UNiagaraGraph* InGraph, FString EmitterNameOverride = TEXT(""));
	static TArray<FNiagaraParameterMapHistory> GetParameterMaps(class UNiagaraScriptSourceBase* InSource, FString EmitterNameOverride = TEXT(""));

	virtual bool AllowNiagaraTypeForAddPin(const FNiagaraTypeDefinition& InType) override;
	
	virtual TSharedRef<SWidget> GenerateAddPinMenu(const FString& InWorkingPinName, SNiagaraGraphPinAdd* InPin) override;

	/** Gets the description text for a pin. */
	FText GetPinDescriptionText(UEdGraphPin* Pin) const;

	/** Called when a pin's description text is committed. */
	void PinDescriptionTextCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphPin* Pin);
protected:
	virtual void BuildCommonMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin);
	virtual void BuildLocalMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin);
	virtual void BuildEngineMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin);
	virtual void BuildParameterCollectionsMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin);
	virtual void BuildParameterCollectionMenu(FMenuBuilder& InMenuBuilder, UNiagaraParameterCollection* Collection, SNiagaraGraphPinAdd* InPin);

};
