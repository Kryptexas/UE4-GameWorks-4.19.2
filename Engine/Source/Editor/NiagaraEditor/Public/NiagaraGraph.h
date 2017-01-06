// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "EdGraph/EdGraph.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.generated.h"

/** This is the type of action that occurred on a given Niagara graph. Note that this should follow from EEdGraphActionType, leaving some slop for growth. */
enum ENiagaraGraphActionType
{
	GRAPHACTION_GenericNeedsRecompile = 0x1 << 16,
};

UCLASS(MinimalAPI)
class UNiagaraGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;
	//~ End UObjet Interface
	
	/** Get the source that owns this graph */
	class UNiagaraScriptSource* GetSource() const;

	/** Gets the usage of this graph based on the owning script. */
	ENiagaraScriptUsage GetUsage() const;
		
	class UNiagaraNodeOutput* FindOutputNode() const;
	void FindInputNodes(TArray<class UNiagaraNodeInput*>& OutInputNodes, bool bSort = false) const;

	/** Generates a list of unique input and output parameters for when this script is used as a function. */
	void GetParameters(TArray<FNiagaraVariable>& Inputs, TArray<FNiagaraVariable>& Outputs) const;

	void FindReadDataSetNodes(TArray<class UNiagaraNodeReadDataSet*>& OutReadNodes) const;
	void FindWriteDataSetNodes(TArray<class UNiagaraNodeWriteDataSet*>& OutWriteNodes) const;

	/** Returns the index of this attribute in the output node of the graph. INDEX_NONE if this is not a valid attribute. */
	int32 GetAttributeIndex(const FNiagaraVariable& Attr)const;
	void GetAttributes(TArray< FNiagaraVariable >& OutAttributes)const;

	bool HasNumericParameters()const;

	/** Signal to listeners that the graph has changed */
	void NotifyGraphNeedsRecompile();

	/** Get all referenced graphs in this specified graph, including this graph. */
	void GetAllReferencedGraphs(TArray<const UNiagaraGraph*>& Graphs) const;

};

