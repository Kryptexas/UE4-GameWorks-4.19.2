// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"
#include "AnimGraphDefinitions.h"
#include "Animation/AnimNodeBase.h"

//
// Forward declarations.
//
class UAnimationGraphSchema;
class UAnimGraphNode_SaveCachedPose;
class UAnimGraphNode_UseCachedPose;
class UBlueprintGeneratedClass;
struct FPoseLinkMappingRecord;

//////////////////////////////////////////////////////////////////////////
// FAnimBlueprintCompiler

class FAnimBlueprintCompiler : public FKismetCompilerContext
{
protected:
	typedef FKismetCompilerContext Super;
public:
	FAnimBlueprintCompiler(UAnimBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions, TArray<UObject*>* InObjLoaded);
	virtual ~FAnimBlueprintCompiler();
protected:
	// Implementation of FKismetCompilerContext interface
	virtual UEdGraphSchema_K2* CreateSchema() override;
	virtual void MergeUbergraphPagesIn(UEdGraph* Ubergraph) override;
	virtual void ProcessOneFunctionGraph(UEdGraph* SourceGraph, bool bInternalFunction = false) override;
	virtual void CreateFunctionList() override;
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void CopyTermDefaultsToDefaultObject(UObject* DefaultObject) override;
	virtual void PostCompileDiagnostics() override;
	virtual void EnsureProperGeneratedClass(UClass*& TargetClass) override;
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO) override;
	virtual void FinishCompilingClass(UClass* Class) override;
	// End of FKismetCompilerContext interface

protected:
	typedef TArray<UEdGraphPin*> UEdGraphPinArray;

protected:
	// Wireup record for a single anim node property (which might be an array)
	struct FAnimNodeSinglePropertyHandler
	{
		TMap<int32, UEdGraphPin*> ArrayPins;

		UEdGraphPin* SinglePin;

		// If this is valid we are a simple property copy, avoiding BP thunk
		FName SimpleCopyPropertyName;

		FAnimNodeSinglePropertyHandler()
			: SinglePin(NULL)
			, SimpleCopyPropertyName(NAME_None)
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
		FEvaluationHandlerRecord()
			: NodeVariableProperty(NULL)
			, EvaluationHandlerProperty(NULL)
			, HandlerFunctionName(NAME_None)
		{
		}

		bool OnlyUsesCopyRecords() const
		{
			for(TMap<FName, FAnimNodeSinglePropertyHandler>::TConstIterator It(ServicedProperties); It; ++It)
			{
				const FAnimNodeSinglePropertyHandler& AnimNodeSinglePropertyHandler = It.Value();
				if(AnimNodeSinglePropertyHandler.SimpleCopyPropertyName == NAME_None)
				{
					return false;
				}
			}

			return true;
		}

		bool IsValid() const
		{
			return (NodeVariableProperty != NULL) && (EvaluationHandlerProperty != NULL);
		}

		void PatchFunctionNameAndCopyRecordsInto(UObject* TargetObject) const
		{
			FExposedValueHandler* HandlerPtr = EvaluationHandlerProperty->ContainerPtrToValuePtr<FExposedValueHandler>(NodeVariableProperty->ContainerPtrToValuePtr<void>(TargetObject));
			HandlerPtr->CopyRecords.Empty();

			bool bOnlyUsesCopyRecords = true;
			for(TMap<FName, FAnimNodeSinglePropertyHandler>::TConstIterator PropertyIt(ServicedProperties); PropertyIt; ++PropertyIt)
			{
				const FAnimNodeSinglePropertyHandler& AnimNodeSinglePropertyHandler = PropertyIt.Value();
				if(AnimNodeSinglePropertyHandler.SimpleCopyPropertyName != NAME_None)
				{
					// Local variable get, no need to thunk to BP to grab this value

					UProperty* SimpleCopyPropertySource = TargetObject->GetClass()->FindPropertyByName(AnimNodeSinglePropertyHandler.SimpleCopyPropertyName);
					check(SimpleCopyPropertySource);

					if(AnimNodeSinglePropertyHandler.ArrayPins.Num() > 0)
					{
						UArrayProperty* SimpleCopyPropertyDest = CastChecked<UArrayProperty>(NodeVariableProperty->Struct->FindPropertyByName(PropertyIt.Key()));
						check(SimpleCopyPropertySource->GetSize() == SimpleCopyPropertyDest->Inner->GetSize());

						for(TMap<int32, UEdGraphPin*>::TConstIterator ArrayIt(AnimNodeSinglePropertyHandler.ArrayPins); ArrayIt; ++ArrayIt)
						{
							FExposedValueCopyRecord CopyRecord;
							CopyRecord.DestProperty = SimpleCopyPropertyDest;
							CopyRecord.DestArrayIndex = ArrayIt.Key();
							CopyRecord.SourceProperty = SimpleCopyPropertySource;
							CopyRecord.SourceArrayIndex = 0;
							CopyRecord.Size = SimpleCopyPropertyDest->Inner->GetSize();
							HandlerPtr->CopyRecords.Add(CopyRecord);	
						}
					}
					else
					{
						UProperty* SimpleCopyPropertyDest = NodeVariableProperty->Struct->FindPropertyByName(PropertyIt.Key());
						check(SimpleCopyPropertySource->GetSize() == SimpleCopyPropertyDest->GetSize());

						// Local variable get, no need to thunk to BP to grab this value
						FExposedValueCopyRecord CopyRecord;
						CopyRecord.DestProperty = SimpleCopyPropertyDest;
						CopyRecord.DestArrayIndex = 0;
						CopyRecord.SourceProperty = SimpleCopyPropertySource;
						CopyRecord.SourceArrayIndex = 0;
						CopyRecord.Size = SimpleCopyPropertyDest->GetSize();
						HandlerPtr->CopyRecords.Add(CopyRecord);
					}
				}
				else
				{
					bOnlyUsesCopyRecords = false;
				}
			}

			if(!bOnlyUsesCopyRecords)
			{
				// not all of our pins use copy records so we will need to call our exposed value handler
				HandlerPtr->BoundFunction = HandlerFunctionName;
			}
		}

		void RegisterPin(UEdGraphPin* SourcePin, UProperty* AssociatedProperty, int32 AssociatedPropertyArrayIndex)
		{
			FAnimNodeSinglePropertyHandler& Handler = ServicedProperties.FindOrAdd(AssociatedProperty->GetFName());

			if (AssociatedPropertyArrayIndex != INDEX_NONE)
			{
				Handler.ArrayPins.Add(AssociatedPropertyArrayIndex, SourcePin);
			}
			else
			{
				check(Handler.SinglePin == NULL);
				Handler.SinglePin = SourcePin;
			}

			if(GetDefault<UEngine>()->bOptimizeAnimBlueprintMemberVariableAccess)
			{
				UK2Node_VariableGet* VariableGetNode = Cast<UK2Node_VariableGet>(SourcePin->LinkedTo[0]->GetOwningNode());
				if(VariableGetNode && VariableGetNode->IsNodePure() && VariableGetNode->VariableReference.IsSelfContext())
				{
					Handler.SimpleCopyPropertyName = VariableGetNode->VariableReference.GetMemberName();
				}
			}
		}
	};

	// State machines may get processed before their inner graphs, so their node index needs to be patched up later
	// This structure records pending fixups.
	struct FStateRootNodeIndexFixup
	{
	public:
		int32 MachineIndex;
		int32 StateIndex;
		UAnimGraphNode_StateResult* ResultNode;

	public:
		FStateRootNodeIndexFixup(int32 InMachineIndex, int32 InStateIndex, UAnimGraphNode_StateResult* InResultNode)
			: MachineIndex(InMachineIndex)
			, StateIndex(InStateIndex)
			, ResultNode(InResultNode)
		{
		}
	};

protected:
	UAnimBlueprintGeneratedClass* NewAnimBlueprintClass;
	UAnimBlueprint* AnimBlueprint;

	UAnimationGraphSchema* AnimSchema;

	// Map of allocated v3 nodes that are members of the class
	TMap<class UAnimGraphNode_Base*, UProperty*> AllocatedAnimNodes;
	TMap<UProperty*, class UAnimGraphNode_Base*> AllocatedNodePropertiesToNodes;
	TMap<int32, UProperty*> AllocatedPropertiesByIndex;

	// Map of true source objects (user edited ones) to the cloned ones that are actually compiled
	TMap<class UAnimGraphNode_Base*, UAnimGraphNode_Base*> SourceNodeToProcessedNodeMap;

	// Index of the nodes (must match up with the runtime discovery process of nodes, which runs thru the property chain)
	int32 AllocateNodeIndexCounter;
	TMap<class UAnimGraphNode_Base*, int32> AllocatedAnimNodeIndices;

	// Map from pose link LinkID address
	//@TODO: Bad structure for a list of these
	TArray<FPoseLinkMappingRecord> ValidPoseLinkList;

	// List of successfully created evaluation handlers
	TArray<FEvaluationHandlerRecord> ValidEvaluationHandlerList;

	// List of animation node literals (values exposed as pins but never wired up) that need to be pushed into the CDO
	TArray<FEffectiveConstantRecord> ValidAnimNodePinConstants;

	// Map of cache name to encountered save cached pose nodes
	TMap<FString, UAnimGraphNode_SaveCachedPose*> SaveCachedPoseNodes;

	// True if any parent class is also generated from an animation blueprint
	bool bIsDerivedAnimBlueprint;
private:
	int32 FindOrAddNotify(FAnimNotifyEvent& Notify);

	UK2Node_CallFunction* SpawnCallAnimInstanceFunction(UEdGraphNode* SourceNode, FName FunctionName);

	// Creates an evaluation handler for an FExposedValue property in an animation node
	void CreateEvaluationHandler(UAnimGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record);

	// Prunes any nodes that aren't reachable via a pose link
	void PruneIsolatedAnimationNodes(const TArray<UAnimGraphNode_Base*>& RootSet, TArray<UAnimGraphNode_Base*>& GraphNodes);

	// Compiles one animation node
	void ProcessAnimationNode(UAnimGraphNode_Base* VisualAnimNode);

	// Compiles one state machine
	void ProcessStateMachine(UAnimGraphNode_StateMachineBase* StateMachineInstance);

	// Compiles one use cached pose instance
	void ProcessUseCachedPose(UAnimGraphNode_UseCachedPose* UseCachedPose);

	// Compiles an entire animation graph
	void ProcessAllAnimationNodes();

	// Convert transition getters into a function call/etc...
	void ProcessTransitionGetter(class UK2Node_TransitionRuleGetter* Getter, UAnimStateTransitionNode* TransitionNode);

	//
	void ProcessAnimationNodesGivenRoot(TArray<UAnimGraphNode_Base*>& AnimNodeList, const TArray<UAnimGraphNode_Base*>& RootSet);

	// Automatically fill in parameters for the specified Getter node
	void AutoWireAnimGetter(class UK2Node_AnimGetter* Getter, UAnimStateTransitionNode* InTransitionNode);

	// This function does the following steps:
	//   Clones the nodes in the specified source graph
	//   Merges them into the ConsolidatedEventGraph
	//   Processes any animation nodes
	//   Returns the index of the processed cloned version of SourceRootNode
	//	 If supplied, will also return an array of all cloned nodes
	int32 ExpandGraphAndProcessNodes(UEdGraph* SourceGraph, UAnimGraphNode_Base* SourceRootNode, UAnimStateTransitionNode* TransitionNode = NULL, TArray<UEdGraphNode*>* ClonedNodes = NULL);

	// Dumps compiler diagnostic information
	void DumpAnimDebugData();

	// Returns the allocation index of the specified node, processing it if it was pending
	int32 GetAllocationIndexOfNode(UAnimGraphNode_Base* VisualAnimNode);
};

