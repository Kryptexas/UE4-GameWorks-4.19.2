// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeManager.generated.h"

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
class ENGINE_API UBehaviorTreeManager : public UObject
{
	GENERATED_UCLASS_BODY()

	/** limit for recording execution steps for debugger */
	UPROPERTY(config)
	int32 MaxDebuggerSteps;

	/** get behavior tree template for given blueprint */
	bool LoadTree(class UBehaviorTree* Asset, class UBTCompositeNode*& Root, uint16& InstanceMemorySize);

	/** check if BT system is to be used. Here only for BT development time */
	static bool IsBehaviorTreeUsageEnabled();

	/** cleanup hooks for map loading */
	virtual void FinishDestroy() OVERRIDE;

protected:

	/** initialized tree templates */
	UPROPERTY()
	TArray<FBehaviorTreeTemplateInfo> LoadedTemplates;
};
