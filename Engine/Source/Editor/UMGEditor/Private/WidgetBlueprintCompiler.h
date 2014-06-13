// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// FWidgetBlueprintCompiler

class FWidgetBlueprintCompiler : public FKismetCompilerContext
{
protected:
	typedef FKismetCompilerContext Super;

public:
	FWidgetBlueprintCompiler(UWidgetBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded);
	virtual ~FWidgetBlueprintCompiler();

	// FKismetCompilerContext
	virtual void Compile() override;
	// End FKismetCompilerContext

protected:
	UWidgetBlueprint* WidgetBlueprint() const { return Cast<UWidgetBlueprint>(Blueprint); }

	void ValidateWidgetNames();

	// FKismetCompilerContext
	//virtual UEdGraphSchema_K2* CreateSchema() OVERRIDE;
	virtual void MergeUbergraphPagesIn(UEdGraph* Ubergraph) override;
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO) override;
	virtual void EnsureProperGeneratedClass(UClass*& TargetClass) override;
	virtual void CreateClassVariablesFromBlueprint() override;
	virtual void FinishCompilingClass(UClass* Class) override;
	virtual bool ValidateGeneratedClass(UBlueprintGeneratedClass* Class) override;
	virtual void CopyTermDefaultsToDefaultObject(UObject* DefaultObject) override;
	// End FKismetCompilerContext

protected:
	// Wireup record for a single anim node property (which might be an array)
	struct FAnimNodeSinglePropertyHandler
	{
		TMap<int32, UEdGraphPin*> ArrayPins;

		UEdGraphPin* SinglePin;

		FAnimNodeSinglePropertyHandler()
			: SinglePin(NULL)
		{
		}
	};

	// Record for a property that was exposed as a pin, but wasn't wired up (just a literal)
	struct FEffectiveConstantRecord
	{
	public:
		// The node variable that the handler is in
		class UStructProperty* NodeVariableProperty;

		// The property within the struct to set
		class UProperty* ConstantProperty;

		// The array index if ConstantProperty is an array property, or INDEX_NONE otherwise
		int32 ArrayIndex;

		// The pin to pull the DefaultValue/DefaultObject from
		UEdGraphPin* LiteralSourcePin;

		FEffectiveConstantRecord()
			: NodeVariableProperty(NULL)
			, ConstantProperty(NULL)
			, ArrayIndex(INDEX_NONE)
			, LiteralSourcePin(NULL)
		{
		}

		FEffectiveConstantRecord(UStructProperty* ContainingNodeProperty, UEdGraphPin* SourcePin, UProperty* SourcePinProperty, int32 SourceArrayIndex)
			: NodeVariableProperty(ContainingNodeProperty)
			, ConstantProperty(SourcePinProperty)
			, ArrayIndex(SourceArrayIndex)
			, LiteralSourcePin(SourcePin)
		{
		}

		bool Apply(UObject* Object);
	};

	// Struct used to record information about one evaluation handler
	struct FEvaluationHandlerRecord
	{
	public:
		// The node variable that the handler is in
		class UStructProperty* NodeVariableProperty;

		// The specific evaluation handler inside the specified node
		class UStructProperty* EvaluationHandlerProperty;

		// Set of properties serviced by this handler (Map from property name to the record for that property)
		TMap<FName, FAnimNodeSinglePropertyHandler> ServicedProperties;

		// The resulting function name
		FName HandlerFunctionName;
	public:
		FEvaluationHandlerRecord();

		bool IsValid() const;

		void PatchFunctionNameInto(UObject* TargetObject) const;

		void RegisterPin(UEdGraphPin* SourcePin, UProperty* AssociatedProperty, int32 AssociatedPropertyArrayIndex);
	};

protected:
	void ProcessWidgetNode(UWidgetGraphNode_Base* VisualWidgetNode);

	void CreateEvaluationHandler(UWidgetGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record);

	UWidgetBlueprintGeneratedClass* NewWidgetBlueprintClass;

	// Map of properties created for widgets; to aid in debug data generation
	TMap<class UWidget*, class UProperty*> WidgetToMemberVariableMap;

	///----------------------------------------------------------------
	// Callback system properties.

	// Map of allocated v3 nodes that are members of the class
	TMap<class UWidgetGraphNode_Base*, UProperty*> AllocatedAnimNodes;
	TMap<UProperty*, class UWidgetGraphNode_Base*> AllocatedNodePropertiesToNodes;
	TMap<int32, UProperty*> AllocatedPropertiesByIndex;

	// Map of true source objects (user edited ones) to the cloned ones that are actually compiled
	TMap<class UWidgetGraphNode_Base*, UWidgetGraphNode_Base*> SourceNodeToProcessedNodeMap;

	// Index of the nodes (must match up with the runtime discovery process of nodes, which runs thru the property chain)
	int32 AllocateNodeIndexCounter;
	TMap<class UWidgetGraphNode_Base*, int32> AllocatedAnimNodeIndices;

	// List of successfully created evaluation handlers
	TArray<FEvaluationHandlerRecord> ValidEvaluationHandlerList;

	// List of animation node literals (values exposed as pins but never wired up) that need to be pushed into the CDO
	TArray<FEffectiveConstantRecord> ValidAnimNodePinConstants;

	///----------------------------------------------------------------
};

