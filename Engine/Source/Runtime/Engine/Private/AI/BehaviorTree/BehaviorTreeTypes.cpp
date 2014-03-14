// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//----------------------------------------------------------------------//
// FBehaviorTreeInstance
//----------------------------------------------------------------------//
void FBehaviorTreeInstance::InitializeMemory(class UBehaviorTreeComponent* OwnerComp, UBTCompositeNode* Node)
{
	if (Node == NULL)
	{
		return;
	}

	for (int32 i = 0; i < Node->Services.Num(); i++)
	{
		Node->Services[i]->InitializeMemory(OwnerComp, Node->Services[i]->GetNodeMemory<uint8>(*this));
	}

	Node->InitializeMemory(OwnerComp, Node->GetNodeMemory<uint8>(*this));

	for (int32 iChild = 0; iChild < Node->Children.Num(); iChild++)
	{
		FBTCompositeChild& ChildInfo = Node->Children[iChild];

		for (int32 i = 0; i < ChildInfo.Decorators.Num(); i++)
		{
			ChildInfo.Decorators[i]->InitializeMemory(OwnerComp, ChildInfo.Decorators[i]->GetNodeMemory<uint8>(*this));
		}

		if (ChildInfo.ChildComposite)
		{
			InitializeMemory(OwnerComp, ChildInfo.ChildComposite);
		}
		else if (ChildInfo.ChildTask)
		{
			ChildInfo.ChildTask->InitializeMemory(OwnerComp, ChildInfo.ChildTask->GetNodeMemory<uint8>(*this));
		}
	}
}

//----------------------------------------------------------------------//
// FBTNodeIndex
//----------------------------------------------------------------------//

bool FBTNodeIndex::TakesPriorityOver(const FBTNodeIndex& Other) const
{
	// instance closer to root is more important
	if (InstanceIndex != Other.InstanceIndex)
	{
		return InstanceIndex < Other.InstanceIndex;
	}

	// higher priority is more important
	return ExecutionIndex < Other.ExecutionIndex;
}

//----------------------------------------------------------------------//
// FBehaviorTreeSearchData
//----------------------------------------------------------------------//
void FBehaviorTreeSearchData::AddUniqueUpdate(const FBehaviorTreeSearchUpdate& UpdateInfo)
{
	static FString UpdateModeDesc[] = { TEXT("Add"), TEXT("Remove") };
	UE_VLOG(OwnerComp->GetOwner(), LogBehaviorTree, Verbose, TEXT("Search node update[%s]: %s"),
		*UpdateModeDesc[UpdateInfo.Mode],
		UpdateInfo.AuxNode ? *UBehaviorTreeTypes::DescribeNodeHelper(UpdateInfo.AuxNode) : *UBehaviorTreeTypes::DescribeNodeHelper(UpdateInfo.TaskNode));

	bool bHasAddRemovePair = false;
	for (int32 i = 0; i < PendingUpdates.Num(); i++)
	{
		const FBehaviorTreeSearchUpdate& Info = PendingUpdates[i];
		if (Info.AuxNode == UpdateInfo.AuxNode && Info.TaskNode == UpdateInfo.TaskNode)
		{
			bHasAddRemovePair = (Info.Mode != UpdateInfo.Mode);
			PendingUpdates.RemoveAt(i, 1, false);
		}
	}

	if (!bHasAddRemovePair)
	{
		const int32 Idx = PendingUpdates.Add(UpdateInfo);
		PendingUpdates[Idx].bPostUpdate = (UpdateInfo.Mode == EBTNodeUpdateMode::Add) && (Cast<UBTService>(UpdateInfo.AuxNode) != NULL);
	}
}

//----------------------------------------------------------------------//
// FBlackboardKeySelector
//----------------------------------------------------------------------//
void FBlackboardKeySelector::CacheSelectedKey(class UBlackboardData* BlackboardAsset)
{
	if (BlackboardAsset)
	{
		if (SelectedKeyName == NAME_None)
		{
			InitSelectedKey(BlackboardAsset);
		}

		SelectedKeyID = BlackboardAsset->GetKeyID(SelectedKeyName);
		SelectedKeyType = BlackboardAsset->GetKeyType(SelectedKeyID);
	}
}

void FBlackboardKeySelector::InitSelectedKey(class UBlackboardData* BlackboardAsset)
{
	check(BlackboardAsset);
	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		for (int32 i = 0; i < It->Keys.Num(); i++)
		{
			const FBlackboardEntry& EntryInfo = It->Keys[i];
			if (EntryInfo.KeyType)
			{
				bool bFilterPassed = false;
				if (AllowedTypes.Num())
				{
					for (int32 iFilter = 0; iFilter < AllowedTypes.Num(); iFilter++)
					{
						if (EntryInfo.KeyType->IsAllowedByFilter(AllowedTypes[iFilter]))
						{
							bFilterPassed = true;
							break;
						}
					}
				}
				else
				{
					bFilterPassed = true;
				}

				SelectedKeyName = EntryInfo.EntryName;
				break;
			}
		}
	}
}

void FBlackboardKeySelector::AddObjectFilter(UObject* Owner, TSubclassOf<UObject> AllowedClass)
{
	UBlackboardKeyType_Object* FilterOb = NewObject<UBlackboardKeyType_Object>(Owner);
	FilterOb->BaseClass = AllowedClass;
	AllowedTypes.Add(FilterOb);
}

void FBlackboardKeySelector::AddClassFilter(UObject* Owner, TSubclassOf<UClass> AllowedClass)
{
	UBlackboardKeyType_Class* FilterOb = NewObject<UBlackboardKeyType_Class>(Owner);
	FilterOb->BaseClass = AllowedClass;
	AllowedTypes.Add(FilterOb);
}

void FBlackboardKeySelector::AddEnumFilter(UObject* Owner, UEnum* AllowedEnum)
{
	UBlackboardKeyType_Enum* FilterOb = NewObject<UBlackboardKeyType_Enum>(Owner);
	FilterOb->EnumType = AllowedEnum;
	AllowedTypes.Add(FilterOb);
}

void FBlackboardKeySelector::AddNativeEnumFilter(UObject* Owner, const FString& AllowedEnumName)
{
	UBlackboardKeyType_NativeEnum* FilterOb = NewObject<UBlackboardKeyType_NativeEnum>(Owner);
	FilterOb->EnumName = AllowedEnumName;
	AllowedTypes.Add(FilterOb);
}

void FBlackboardKeySelector::AddIntFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_Int>(Owner));
}

void FBlackboardKeySelector::AddFloatFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_Float>(Owner));
}

void FBlackboardKeySelector::AddBoolFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_Bool>(Owner));
}

void FBlackboardKeySelector::AddVectorFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_Vector>(Owner));
}

void FBlackboardKeySelector::AddStringFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_String>(Owner));
}

void FBlackboardKeySelector::AddNameFilter(UObject* Owner)
{
	AllowedTypes.Add(NewObject<UBlackboardKeyType_Name>(Owner));
}

//----------------------------------------------------------------------//
// UBehaviorTreeTypes
//----------------------------------------------------------------------//
UBehaviorTreeTypes::UBehaviorTreeTypes(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

FString UBehaviorTreeTypes::DescribeNodeResult(EBTNodeResult::Type NodeResult)
{
	static FString ResultDesc[] = { TEXT("Succeeded"), TEXT("Failed"), TEXT("Optional"), TEXT("Aborted"), TEXT("InProgress") };
	return (NodeResult < ARRAY_COUNT(ResultDesc)) ? ResultDesc[NodeResult] : FString();
}

FString UBehaviorTreeTypes::DescribeFlowAbortMode(EBTFlowAbortMode::Type AbortMode)
{
	static FString AbortModeDesc[] = { TEXT("None"), TEXT("Lower Priority"), TEXT("Self"), TEXT("Both") };
	return (AbortMode < ARRAY_COUNT(AbortModeDesc)) ? AbortModeDesc[AbortMode] : FString();
}

FString UBehaviorTreeTypes::DescribeActiveNode(EBTActiveNode::Type ActiveNodeType)
{
	static FString ActiveDesc[] = { TEXT("Composite"), TEXT("ActiveTask"), TEXT("AbortingTask"), TEXT("InactiveTask") };
	return (ActiveNodeType < ARRAY_COUNT(ActiveDesc)) ? ActiveDesc[ActiveNodeType] : FString();
}

FString UBehaviorTreeTypes::DescribeTaskStatus(EBTTaskStatus::Type TaskStatus)
{
	static FString TaskStatusDesc[] = { TEXT("Active"), TEXT("Aborting"), TEXT("Inactive") };
	return (TaskStatus < ARRAY_COUNT(TaskStatusDesc)) ? TaskStatusDesc[TaskStatus] : FString();
}

FString UBehaviorTreeTypes::DescribeNodeUpdateMode(EBTNodeUpdateMode::Type UpdateMode)
{
	static FString UpdateModeDesc[] = { TEXT("Add"), TEXT("Remove") };
	return (UpdateMode < ARRAY_COUNT(UpdateModeDesc)) ? UpdateModeDesc[UpdateMode] : FString();
}

FString UBehaviorTreeTypes::DescribeNodeHelper(const UBTNode* Node)
{
	return Node ? FString::Printf(TEXT("%s[%d]"), *Node->GetNodeName(), Node->GetExecutionIndex()) : FString();
}

FString UBehaviorTreeTypes::GetShortTypeName(const UObject* Ob)
{
	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return Ob->GetClass()->GetName().LeftChop(2);
	}

	FString TypeDesc = Ob->GetClass()->GetName();
	const int32 ShortNameIdx = TypeDesc.Find(TEXT("_"));
	if (ShortNameIdx != INDEX_NONE)
	{
		TypeDesc = TypeDesc.Mid(ShortNameIdx + 1);
	}

	return TypeDesc;
}
