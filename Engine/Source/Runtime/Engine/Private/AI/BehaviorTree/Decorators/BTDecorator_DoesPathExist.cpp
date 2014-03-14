// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_DoesPathExist::UBTDecorator_DoesPathExist(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Does path exist";

	// accept only actors and vectors
	BlackboardKeyA.AddObjectFilter(this, AActor::StaticClass());
	BlackboardKeyA.AddVectorFilter(this);
	BlackboardKeyB.AddObjectFilter(this, AActor::StaticClass());
	BlackboardKeyB.AddVectorFilter(this);

	bAllowAbortLowerPri = false;
	bAllowAbortNone = true;
	bAllowAbortChildNodes = false;
	FlowAbortMode = EBTFlowAbortMode::None;

	bUseSelf = true;
	PathQueryType = EPathExistanceQueryType::HierarchicalQuery;
}

void UBTDecorator_DoesPathExist::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (bUseSelf == false)
	{
		BlackboardKeyA.CacheSelectedKey(BBAsset);
	}
	BlackboardKeyB.CacheSelectedKey(BBAsset);
}

bool UBTDecorator_DoesPathExist::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const 
{
	FVector PointA = FVector::ZeroVector;
	FVector PointB = FVector::ZeroVector;
	bool bHasPointA = false;
	bool bHasPointB = false;
	bool bHasPath = false;

	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();

	if (bUseSelf)
	{
		const AController* ControllerOwner = Cast<AController>(OwnerComp->GetOwner());
		const AActor* ActorOwner = ControllerOwner ? ControllerOwner->GetPawn() : OwnerComp->GetOwner();
		if (ActorOwner)
		{
			PointA = ActorOwner->GetActorLocation();
			bHasPointA = true;
		}
	}
	else if (BlackboardComp && BlackboardComp->GetLocationFromEntry(BlackboardKeyA.SelectedKeyID, PointA))
	{
		bHasPointA = true;
	}

	if (BlackboardComp && BlackboardComp->GetLocationFromEntry(BlackboardKeyB.SelectedKeyID, PointB))
	{
		bHasPointB = true;
	}

	const UNavigationSystem* NavSys = OwnerComp->GetWorld()->GetNavigationSystem();
	if (NavSys && bHasPointA && bHasPointB)
	{
		const AAIController* AIOwner = Cast<AAIController>(OwnerComp->GetOwner());
		const ANavigationData* NavData = AIOwner && AIOwner->NavComponent ? AIOwner->NavComponent->GetNavData() : NULL;

		if (PathQueryType == EPathExistanceQueryType::NavmeshRaycast2D)
		{
#if WITH_RECAST
			const ARecastNavMesh* RecastNavMesh = Cast<const ARecastNavMesh>(NavData);
			bHasPath = RecastNavMesh && RecastNavMesh->IsSegmentOnNavmesh(PointA, PointB);
#endif
		}
		else
		{		
			EPathFindingMode::Type TestMode = (PathQueryType == EPathExistanceQueryType::HierarchicalQuery) ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;
			bHasPath = NavSys->TestPathSync(FPathFindingQuery(NavData, PointA, PointB), TestMode);
		}
	}

	return bHasPath;
}

FString UBTDecorator_DoesPathExist::GetStaticDescription() const 
{
	const UEnum* PathTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPathExistanceQueryType"));
	check(PathTypeEnum);

	return FString::Printf(TEXT("%s: Find path from %s to %s (mode:%s)"),
		*Super::GetStaticDescription(),
		bUseSelf ? TEXT("Self") : *BlackboardKeyA.SelectedKeyName.ToString(),
		*BlackboardKeyB.SelectedKeyName.ToString(),
		*PathTypeEnum->GetEnumName(PathQueryType));
}
