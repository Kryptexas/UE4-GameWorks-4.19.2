// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Decorators/BTDecorator_KeepInCone.h"

UBTDecorator_KeepInCone::UBTDecorator_KeepInCone(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Keep in Cone";

	// accept only actors and vectors
	ConeOrigin.AddObjectFilter(this, AActor::StaticClass());
	ConeOrigin.AddVectorFilter(this);
	Observed.AddObjectFilter(this, AActor::StaticClass());
	Observed.AddVectorFilter(this);

	bNotifyBecomeRelevant = true;
	bNotifyTick = true;

	// KeepInCone always abort current branch
	bAllowAbortLowerPri = false;
	bAllowAbortNone = false;
	FlowAbortMode = EBTFlowAbortMode::Self;
	
	ConeOrigin.SelectedKeyName = FBlackboard::KeySelf;
	ConeHalfAngle = 45.0f;
}

void UBTDecorator_KeepInCone::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);

	ConeHalfAngleDot = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));
	
	if (bUseSelfAsOrigin)
	{
		ConeOrigin.SelectedKeyName = FBlackboard::KeySelf;
		bUseSelfAsOrigin = false;
	}

	if (bUseSelfAsObserved)
	{
		Observed.SelectedKeyName = FBlackboard::KeySelf;
		bUseSelfAsObserved = false;
	}

	UBlackboardData* BBAsset = GetBlackboardAsset();
	ConeOrigin.CacheSelectedKey(BBAsset);
	Observed.CacheSelectedKey(BBAsset);
}

bool UBTDecorator_KeepInCone::CalculateCurrentDirection(const UBehaviorTreeComponent* OwnerComp, FVector& Direction) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp == NULL)
	{
		return false;
	}

	FVector PointA = FVector::ZeroVector;
	FVector PointB = FVector::ZeroVector;
	const bool bHasPointA = BlackboardComp->GetLocationFromEntry(ConeOrigin.GetSelectedKeyID(), PointA);
	const bool bHasPointB = BlackboardComp->GetLocationFromEntry(Observed.GetSelectedKeyID(), PointB);

	if (bHasPointA && bHasPointB)
	{
		Direction = (PointB - PointA).SafeNormal();
		return true;
	}

	return false;
}

void UBTDecorator_KeepInCone::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;
	FVector InitialDir(1.0f, 0, 0);

	CalculateCurrentDirection(OwnerComp, InitialDir);
	DecoratorMemory->InitialDirection = InitialDir;
}

void UBTDecorator_KeepInCone::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;
	FVector CurrentDir(1.0f, 0, 0);
	
	if (CalculateCurrentDirection(OwnerComp, CurrentDir))
	{
		const float Angle = DecoratorMemory->InitialDirection.CosineAngle2D(CurrentDir);
		if (Angle < ConeHalfAngleDot || (IsInversed() && Angle > ConeHalfAngleDot))
		{
			OwnerComp->RequestExecution(this);
		}
	}
}

FString UBTDecorator_KeepInCone::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s in %.2f degree cone of initial direction [%s-%s]"),
		*Super::GetStaticDescription(),
		*Observed.SelectedKeyName.ToString(),
		ConeHalfAngle * 2,
		*ConeOrigin.SelectedKeyName.ToString(),
		*Observed.SelectedKeyName.ToString());
}

void UBTDecorator_KeepInCone::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;
	FVector CurrentDir(1.0f, 0, 0);
	
	if (CalculateCurrentDirection(OwnerComp, CurrentDir))
	{
		const float CurrentAngleDot = DecoratorMemory->InitialDirection.CosineAngle2D(CurrentDir);
		const float CurrentAngleRad = FMath::Acos(CurrentAngleDot);

		Values.Add(FString::Printf(TEXT("Angle: %.0f (%s cone)"),
			FMath::RadiansToDegrees(CurrentAngleRad),
			CurrentAngleDot < ConeHalfAngleDot ? TEXT("outside") : TEXT("inside")
			));

	}
}

uint16 UBTDecorator_KeepInCone::GetInstanceMemorySize() const
{
	return sizeof(TNodeInstanceMemory);
}
