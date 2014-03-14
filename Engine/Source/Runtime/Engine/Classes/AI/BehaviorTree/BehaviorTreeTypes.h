// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeTypes.generated.h"

// Visual logging helper
#define BT_VLOG(Context, Verbosity, Format, ...) UE_VLOG(Context->OwnerComp.IsValid() ? Context->OwnerComp->GetOwner() : NULL, LogBehaviorTree, Verbosity, Format, ##__VA_ARGS__)
#define BT_SEARCHLOG(SearchData, Verbosity, Format, ...) UE_VLOG(SearchData.OwnerComp->GetOwner(), LogBehaviorTree, Verbosity, Format, ##__VA_ARGS__)

// Behavior tree debugger in editor
#define USE_BEHAVIORTREE_DEBUGGER	(1 && WITH_EDITORONLY_DATA)

DECLARE_STATS_GROUP(TEXT("BehaviorTree"),STATGROUP_AIBehaviorTree);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Tick"),STAT_AI_BehaviorTree_Tick,STATGROUP_AIBehaviorTree, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Load Time"),STAT_AI_BehaviorTree_LoadTime,STATGROUP_AIBehaviorTree, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Search Time"),STAT_AI_BehaviorTree_SearchTime,STATGROUP_AIBehaviorTree, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Execution Time"),STAT_AI_BehaviorTree_ExecutionTime,STATGROUP_AIBehaviorTree, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Auxiliary Update Time"),STAT_AI_BehaviorTree_AuxUpdateTime,STATGROUP_AIBehaviorTree, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Templates"),STAT_AI_BehaviorTree_NumTemplates,STATGROUP_AIBehaviorTree, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Instances"),STAT_AI_BehaviorTree_NumInstances,STATGROUP_AIBehaviorTree, );
DECLARE_MEMORY_STAT_EXTERN(TEXT("Instance memory"),STAT_AI_BehaviorTree_InstanceMemory,STATGROUP_AIBehaviorTree, ENGINE_API);

// delegate defines
DECLARE_DELEGATE_TwoParams(FOnBlackboardChange, const class UBlackboardComponent*, uint8 /*key ID*/);

namespace BTSpecialChild
{
	const int32 NotInitialized = -1;	// special value for child indices: needs to be initialized
	const int32 ReturnToParent = -2;	// special value for child indices: return to parent node
}

UENUM(BlueprintType)
namespace EBTNodeResult
{
	// keep in sync with DescribeNodeResult()
	enum Type
	{
		Succeeded,		// finished as success
		Failed,			// finished as failure
		Optional,		// finished, parent should continue
		Aborted,		// finished aborting = failure
		InProgress,		// not finished yet
	};
}

namespace EBTExecutionMode
{
	enum Type
	{
		SingleRun,
		Looped,
	};
}

UENUM()
namespace EBTFlowAbortMode
{
	// keep in sync with DescribeFlowAbortMode()
	enum Type
	{
		None				UMETA(DisplayName="Nothing"),
		LowerPriority		UMETA(DisplayName="Lower Priority"),
		Self				UMETA(DisplayName="Self"),
		Both				UMETA(DisplayName="Both"),
	};
}

namespace EBTActiveNode
{
	// keep in sync with DescribeActiveNode()
	enum Type
	{
		Composite,
		ActiveTask,
		AbortingTask,
		InactiveTask,
	};
}

namespace EBTTaskStatus
{
	// keep in sync with DescribeTaskStatus()
	enum Type
	{
		Active,
		Aborting,
		Inactive,
	};
}

namespace EBTNodeUpdateMode
{
	// keep in sync with DescribeNodeUpdateMode()
	enum Type
	{
		Add,
		Remove,
	};
}

/** wrapper struct for holding a parallel task node and its status */
struct FBehaviorTreeParallelTask
{
	/** worker object */
	const class UBTTaskNode* TaskNode;

	/** additional mode data used for context switching */
	TEnumAsByte<EBTTaskStatus::Type> Status;

	FBehaviorTreeParallelTask() : TaskNode(NULL) {}
	FBehaviorTreeParallelTask(const UBTTaskNode* InTaskNode, EBTTaskStatus::Type InStatus) : TaskNode(InTaskNode), Status(InStatus) {}

	bool operator==(const FBehaviorTreeParallelTask& Other) const { return TaskNode == Other.TaskNode; }
	bool operator==(const UBTTaskNode* OtherTask) const { return TaskNode == OtherTask; }
};

namespace EBTExecutionSnap
{
	enum Type
	{
		Regular,
		OutOfNodes,
	};
}

namespace EBTDescriptionVerbosity
{
	enum Type
	{
		Basic,
		Detailed,
	};
}

/** debugger data about subtree instance */
struct FBehaviorTreeDebuggerInstance
{
	struct FNodeFlowData
	{
		uint16 ExecutionIndex;
		uint16 bPassed : 1;
		uint16 bOptional : 1;
		uint16 bTrigger : 1;
		uint16 bDiscardedTrigger : 1;

		FNodeFlowData() : ExecutionIndex(INDEX_NONE), bPassed(0), bOptional(0), bTrigger(0), bDiscardedTrigger(0) {}
	};

	FBehaviorTreeDebuggerInstance() : TreeAsset(NULL), RootNode(NULL) {}

	/** behavior tree asset */
	class UBehaviorTree* TreeAsset;

	/** root node in template */
	class UBTCompositeNode* RootNode;

	/** execution indices of active nodes */
	TArray<uint16> ActivePath;

	/** execution indices of active nodes */
	TArray<uint16> AdditionalActiveNodes;

	/** search flow from previous state */
	TArray<FNodeFlowData> PathFromPrevious;

	/** runtime descriptions for each execution index */
	TArray<FString> RuntimeDesc;

	FORCEINLINE bool IsValid() const { return ActivePath.Num() != 0; }
};

/** debugger data about current execution step */
struct FBehaviorTreeExecutionStep
{
	FBehaviorTreeExecutionStep() : TimeStamp(0.f), StepIndex(INDEX_NONE) {}

	/** subtree instance stack */
	TArray<FBehaviorTreeDebuggerInstance> InstanceStack;

	/** blackboard snapshot: value descriptions */
	TArray<FString> BlackboardValues;

	/** Game world's time stamp of this step */
	float TimeStamp;

	/** index of execution step */
	int32 StepIndex;
};

/** data required for instance of single subtree */
struct FBehaviorTreeInstance
{
	/** behavior tree asset */
	class UBehaviorTree* TreeAsset;

	/** root node in template */
	class UBTCompositeNode* RootNode;

	/** active node in template */
	class UBTNode* ActiveNode;

	/** active auxiliary nodes */
	TArray<class UBTAuxiliaryNode*> ActiveAuxNodes;

	/** active parallel tasks */
	TArray<FBehaviorTreeParallelTask> ParallelTasks;

	/** memory: instance */
	TArray<uint8> InstanceMemory;

	/** active node type */
	TEnumAsByte<EBTActiveNode::Type> ActiveNodeType;

	FBehaviorTreeInstance() { IncMemoryStats(); }
	FBehaviorTreeInstance(const FBehaviorTreeInstance& Other) { *this = Other; IncMemoryStats(); }
	FBehaviorTreeInstance(int32 MemorySize) { InstanceMemory.AddZeroed(MemorySize); IncMemoryStats(); }
	~FBehaviorTreeInstance() { DecMemoryStats(); }

#if STATS
	FORCEINLINE void IncMemoryStats() { INC_MEMORY_STAT_BY(STAT_AI_BehaviorTree_InstanceMemory, GetAllocatedSize()); }
	FORCEINLINE void DecMemoryStats() { DEC_MEMORY_STAT_BY(STAT_AI_BehaviorTree_InstanceMemory, GetAllocatedSize()); }
	FORCEINLINE uint32 GetAllocatedSize() const 
	{
		return sizeof(*this) + ActiveAuxNodes.GetAllocatedSize() + ParallelTasks.GetAllocatedSize() + InstanceMemory.GetAllocatedSize(); 
	}
#else
	FORCEINLINE uint32 GetAllocatedSize() const { return 0; }
	FORCEINLINE void IncMemoryStats() {}
	FORCEINLINE void DecMemoryStats() {}
#endif // STATS

	void InitializeMemory(class UBehaviorTreeComponent* OwnerComp, UBTCompositeNode* Node);
};

struct FBTNodeIndex
{
	/** index of instance of stack */
	uint16 InstanceIndex;

	/** execution index within instance */
	uint16 ExecutionIndex;

	FBTNodeIndex() : InstanceIndex(MAX_uint16), ExecutionIndex(MAX_uint16) {}
	FBTNodeIndex(uint16 InInstanceIndex, uint16 InExecutionIndex) : InstanceIndex(InInstanceIndex), ExecutionIndex(InExecutionIndex) {}

	bool TakesPriorityOver(const FBTNodeIndex& Other) const;
	bool IsSet() const { return InstanceIndex < MAX_uint16; }

	FORCEINLINE bool operator==(const FBTNodeIndex& Other) const { return Other.ExecutionIndex == ExecutionIndex && Other.InstanceIndex == InstanceIndex; }
	FORCEINLINE friend uint32 GetTypeHash(const FBTNodeIndex& Other) { return Other.ExecutionIndex ^ Other.InstanceIndex; }

	FORCEINLINE FString Describe() const { return FString::Printf(TEXT("[%d:%d]"), InstanceIndex, ExecutionIndex); }
};

/** node update data */
struct FBehaviorTreeSearchUpdate
{
	class UBTAuxiliaryNode* AuxNode;
	class UBTTaskNode* TaskNode;

	uint16 InstanceIndex;

	TEnumAsByte<EBTNodeUpdateMode::Type> Mode;

	/** if set, this entry will be applied AFTER other are processed */
	uint8 bPostUpdate : 1;

	FBehaviorTreeSearchUpdate() : AuxNode(0), TaskNode(0), InstanceIndex(0) {}
	FBehaviorTreeSearchUpdate(const class UBTAuxiliaryNode* InAuxNode, uint16 InInstanceIndex, EBTNodeUpdateMode::Type InMode) :
		AuxNode((class UBTAuxiliaryNode*)InAuxNode), TaskNode(0), InstanceIndex(InInstanceIndex), Mode(InMode) 
	{}
	FBehaviorTreeSearchUpdate(const class UBTTaskNode* InTaskNode, uint16 InInstanceIndex, EBTNodeUpdateMode::Type InMode) :
		AuxNode(0), TaskNode((class UBTTaskNode*)InTaskNode), InstanceIndex(InInstanceIndex), Mode(InMode) 
	{}
};

/** node search data */
struct FBehaviorTreeSearchData
{
	/** BT component */
	class UBehaviorTreeComponent* OwnerComp;

	/** requested updates of additional nodes (preconditions, services, parallels)
	 *  buffered during search to prevent instant add & remove pairs */
	TArray<struct FBehaviorTreeSearchUpdate> PendingUpdates;

	/** first node allowed in search */
	struct FBTNodeIndex SearchStart;

	/** adds update info to PendingUpdates array, removing all previous updates for this node */
	void AddUniqueUpdate(const FBehaviorTreeSearchUpdate& UpdateInfo);
};

/** property block in blueprint defined nodes */
struct FBehaviorTreePropertyMemory
{
	uint16 Offset;
	uint16 BlockSize;
	
	FBehaviorTreePropertyMemory() {}
	FBehaviorTreePropertyMemory(int32 Value) : Offset((uint32)Value >> 16), BlockSize((uint32)Value & 0xFFFF) {}

	int32 Pack() const { return (int32)(((uint32)Offset << 16) | BlockSize); }
};

/** helper struct for defining types of allowed blackboard entries
 *  (e.g. only entries holding points and objects derived form actor class) */
USTRUCT(BlueprintType)
struct ENGINE_API FBlackboardKeySelector
{
	GENERATED_USTRUCT_BODY()

	/** array of allowed types with additional properties (e.g. ubject's base class) 
	  * EditDefaults is required for FBlackboardSelectorDetails::CacheBlackboardData() */
	UPROPERTY(transient, EditDefaultsOnly, Category=Blackboard)
	TArray<class UBlackboardKeyType*> AllowedTypes;

	/** name of selected key */
	UPROPERTY(EditInstanceOnly, Category=Blackboard)
	FName SelectedKeyName;

	/** ID of selected key */
	UPROPERTY(transient, EditInstanceOnly, Category=Blackboard)
	uint8 SelectedKeyID;

	/** class of selected key  */
	UPROPERTY(transient, EditInstanceOnly, Category=Blackboard)
	TSubclassOf<class UBlackboardKeyType> SelectedKeyType;

	/** cache ID and class of selected key */
	void CacheSelectedKey(class UBlackboardData* BlackboardAsset);

	/** find initial selection */
	void InitSelectedKey(class UBlackboardData* BlackboardAsset);

	/** helper functions for setting basic filters */
	void AddObjectFilter(UObject* Owner, TSubclassOf<UObject> AllowedClass);
	void AddClassFilter(UObject* Owner, TSubclassOf<UClass> AllowedClass);
	void AddEnumFilter(UObject* Owner, UEnum* AllowedEnum);
	void AddNativeEnumFilter(UObject* Owner, const FString& AllowedEnumName);
	void AddIntFilter(UObject* Owner);
	void AddFloatFilter(UObject* Owner);
	void AddBoolFilter(UObject* Owner);
	void AddVectorFilter(UObject* Owner);
	void AddStringFilter(UObject* Owner);
	void AddNameFilter(UObject* Owner);
};

UCLASS(Abstract)
class ENGINE_API UBehaviorTreeTypes : public UObject
{
	GENERATED_UCLASS_BODY()

	static FString DescribeNodeHelper(const UBTNode* Node);

	static FString DescribeNodeResult(EBTNodeResult::Type NodeResult);
	static FString DescribeFlowAbortMode(EBTFlowAbortMode::Type FlowAbortMode);
	static FString DescribeActiveNode(EBTActiveNode::Type ActiveNodeType);
	static FString DescribeTaskStatus(EBTTaskStatus::Type TaskStatus);
	static FString DescribeNodeUpdateMode(EBTNodeUpdateMode::Type UpdateMode);

	/** returns short name of object's class (BTTaskNode_Wait -> Wait) */
	static FString GetShortTypeName(const UObject* Ob);
};
