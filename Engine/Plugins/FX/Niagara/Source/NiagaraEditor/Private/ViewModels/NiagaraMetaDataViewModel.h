// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraTypes.h"

class UNiagaraNodeInput;
class UNiagaraGraph;
class UEdGraphNode;
class FStructOnScope;
struct FNiagaraVariable;
struct FNiagaraVariableMetaData;

/** A view model for a variable metadata. */
class FNiagaraMetaDataViewModel
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnMetadataChanged);
public:
	/*
	 * Create a new variable metadata view model.
	 * @param InGraphVariable The variable which is owned by the graph which connects to this metadata.
	 * @param InGraph The graph where the metadata lives.
	 */
	FNiagaraMetaDataViewModel(FNiagaraVariable& InGraphVariable, UNiagaraGraph& InGraph);
	
	virtual ~FNiagaraMetaDataViewModel();

	virtual FName GetName() const;

	FNiagaraVariable GetVariable() const;
	
	/** Gets a pointer to the actual metadata on the graph */
	FNiagaraVariableMetaData* GetGraphMetaData();
	
	/** Callback for when a change in the value structure needs to be copied into the graph*/
	void NotifyMetaDataChanged();

	/** Gets the FStructOnScope object that holds the editable value of the metadata*/
	TSharedRef<FStructOnScope> GetValueStruct();

	/** Gets a multicast delegate which is called whenever the metadata changes. */
	FOnMetadataChanged& OnMetadataChanged() { return OnMetadataChangedDelegate; }

	/** associates a node to this metadata*/
	void AssociateNode(UEdGraphNode* InNode);
	
private:
	/** Refreshes the parameter value struct from the variable data. */
	void RefreshMetaDataValue();

private:
	/** The graph variable which corresponds to the metadata in our viewmodel */
	FNiagaraVariable GraphVariable;
	
	/** The metadata which is being displayed and edited by this view model. */
	FNiagaraVariableMetaData* GraphMetaData;
	
	/** graph who owns the metadata*/
	UNiagaraGraph* CurrentGraph;

	/** graph nodes associated with this metadata*/
	TArray<TWeakObjectPtr<UObject>> AssociatedNodes;
	
	/** actual value being edited*/
	TSharedPtr<FStructOnScope> ValueStruct;
	
	/** A multicast delegate which is called whenever the metadata is changed. */
	FOnMetadataChanged OnMetadataChangedDelegate;
};