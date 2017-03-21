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

	/** Options for the FindInputNodes function */
	struct FFindInputNodeOptions
	{
		FFindInputNodeOptions()
			: bSort(false)
			, bIncludeParameters(true)
			, bIncludeAttributes(true)
			, bIncludeSystemConstants(true)
			, bFilterDuplicates(false)
		{
		}

		/** Whether or not to sort the nodes, defaults to false. */
		bool bSort;
		/** Whether or not to include parameters, defaults to true. */
		bool bIncludeParameters;
		/** Whether or not to include attributes, defaults to true. */
		bool bIncludeAttributes;
		/** Whether or not to include system parameters, defaults to true. */
		bool bIncludeSystemConstants;
		/** Whether of not to filter out duplicate nodes, defaults to false. */
		bool bFilterDuplicates;
	};

	/** Finds input nodes in the graph with. */
	void FindInputNodes(TArray<class UNiagaraNodeInput*>& OutInputNodes, FFindInputNodeOptions Options = FFindInputNodeOptions()) const;

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

	/** Determine if there are any external dependencies wrt to scripts and ensure that those dependencies are sucked into the existing package.*/
	void SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions);

	/** Determine if another item has been synchronized with this graph.*/
	bool IsOtherSynchronized(const FGuid& InChangeId) const;

	/** Mark other object as having been synchronized to this graph.*/
	void MarkOtherSynchronized(FGuid& InChangeId) const;

	/** Identify that this graph has undergone changes that will require synchronization with a compiled script.*/
	void MarkGraphRequiresSynchronization();

private:
	/** The current change identifier for this graph. Used to sync status with UNiagaraScripts.*/
	UPROPERTY()
	FGuid ChangeId;

};

