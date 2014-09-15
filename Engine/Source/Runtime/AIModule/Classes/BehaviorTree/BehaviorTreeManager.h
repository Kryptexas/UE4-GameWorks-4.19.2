// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeManager.generated.h"

class UBehaviorTreeComponent;

USTRUCT()
struct FBehaviorTreeTemplateInfo
{
	GENERATED_USTRUCT_BODY()

	/** behavior tree asset */
	UPROPERTY()
	class UBehaviorTree* Asset;

	/** initialized template */
	UPROPERTY(transient)
	class UBTCompositeNode* Template;

	/** size required for instance memory */
	uint16 InstanceMemorySize;
};

UCLASS(config=Engine)
class AIMODULE_API UBehaviorTreeManager : public UObject
{
	GENERATED_UCLASS_BODY()

	/** limit for recording execution steps for debugger */
	UPROPERTY(config)
	int32 MaxDebuggerSteps;

	/** get behavior tree template for given blueprint */
	bool LoadTree(class UBehaviorTree* Asset, class UBTCompositeNode*& Root, uint16& InstanceMemorySize);

	/** get aligned memory size */
	static int32 GetAlignedDataSize(int32 Size);

	/** helper function for sorting and aligning node memory */
	static void InitializeMemoryHelper(const TArray<class UBTDecorator*>& Nodes, TArray<uint16>& MemoryOffsets, int32& MemorySize);

	/** cleanup hooks for map loading */
	virtual void FinishDestroy() override;

	void DumpUsageStats() const;

	/** register new behavior tree component for tracking */
	void AddActiveComponent(UBehaviorTreeComponent* Component);

	/** unregister behavior tree component from tracking */
	void RemoveActiveComponent(UBehaviorTreeComponent* Component);

	static UBehaviorTreeManager* GetCurrent(UWorld* World);
	static UBehaviorTreeManager* GetCurrent(UObject* WorldContextObject);

protected:

	/** initialized tree templates */
	UPROPERTY()
	TArray<FBehaviorTreeTemplateInfo> LoadedTemplates;

	UPROPERTY()
	TArray<UBehaviorTreeComponent*> ActiveComponents;
};
