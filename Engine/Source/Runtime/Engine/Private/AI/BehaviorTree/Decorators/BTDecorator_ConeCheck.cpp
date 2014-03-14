// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_ConeCheck::UBTDecorator_ConeCheck(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Keep in Cone";

	// accept only actors and vectors
	ConeOrigin.AddObjectFilter(this, AActor::StaticClass());
	ConeOrigin.AddVectorFilter(this);
	ConeDirection.AddObjectFilter(this, AActor::StaticClass());
	ConeDirection.AddVectorFilter(this);
	Observed.AddObjectFilter(this, AActor::StaticClass());
	Observed.AddVectorFilter(this);

	bNotifyBecomeRelevant = true;
	bNotifyTick = true;

	// KeepInCone always abort current branch
	FlowAbortMode = EBTFlowAbortMode::None;
	
	ConeHalfAngle = 45.0f;
}

void UBTDecorator_ConeCheck::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);

	ConeHalfAngleDot = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));

	UBlackboardData* BBAsset = GetBlackboardAsset();
	ConeOrigin.CacheSelectedKey(BBAsset);
	ConeDirection.CacheSelectedKey(BBAsset);
	Observed.CacheSelectedKey(BBAsset);
}

bool UBTDecorator_ConeCheck::CalculateDirection(const UBlackboardComponent* BlackboardComp, const FBlackboardKeySelector& Origin, const FBlackboardKeySelector& End, FVector& Direction) const
{
	FVector PointA = FVector::ZeroVector;
	FVector PointB = FVector::ZeroVector;

	if (BlackboardComp && BlackboardComp->GetLocationFromEntry(Origin.SelectedKeyID, PointA)
		&& BlackboardComp->GetLocationFromEntry(End.SelectedKeyID, PointB))
	{
		Direction = (PointB - PointA).SafeNormal();
		return true;
	}

	return false;
}

FORCEINLINE bool UBTDecorator_ConeCheck::CalcConditionImpl(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BBComponent = OwnerComp->GetBlackboardComponent();

	FVector ConeDir;
	FVector DirectionToObserved;

	return CalculateDirection(BBComponent, ConeOrigin, Observed, DirectionToObserved)
		&& CalculateDirection(BBComponent, ConeOrigin, ConeDirection, ConeDir)
		&& ConeDir.CosineAngle2D(DirectionToObserved) > ConeHalfAngleDot;
}

bool UBTDecorator_ConeCheck::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return CalcConditionImpl(OwnerComp, NodeMemory);
}

void UBTDecorator_ConeCheck::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;
	DecoratorMemory->bLastRawResult = CalcConditionImpl(OwnerComp, NodeMemory);
}

void UBTDecorator_ConeCheck::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{

}

void UBTDecorator_ConeCheck::OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID)
{
	check(false);
}

void UBTDecorator_ConeCheck::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	const TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;

	if (CalcConditionImpl(OwnerComp, NodeMemory) != DecoratorMemory->bLastRawResult)
	{
		OwnerComp->RequestExecution(this);
	}
}

FString UBTDecorator_ConeCheck::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: is %s in %.2f degree %s-%s cone")
		, *Super::GetStaticDescription(), *Observed.SelectedKeyName.ToString()
		, ConeHalfAngle * 2, *ConeOrigin.SelectedKeyName.ToString(), *ConeDirection.SelectedKeyName.ToString());
}

void UBTDecorator_ConeCheck::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	const UBlackboardComponent* BBComponent = OwnerComp->GetBlackboardComponent();
			
	FVector ConeDir;
	FVector DirectionToObserved;

	if (CalculateDirection(BBComponent, ConeOrigin, Observed, DirectionToObserved)
		&& CalculateDirection(BBComponent, ConeOrigin, ConeDirection, ConeDir))
	{
		const float CurrentAngleDot = ConeDir.CosineAngle2D(DirectionToObserved);
		const float CurrentAngleRad = FMath::Acos(CurrentAngleDot);

		Values.Add(FString::Printf(TEXT("Angle: %.0f (%s cone)"),
			FMath::RadiansToDegrees(CurrentAngleRad),
			CurrentAngleDot < ConeHalfAngleDot ? TEXT("outside") : TEXT("inside")
			));

	}
}

uint16 UBTDecorator_ConeCheck::GetInstanceMemorySize() const
{
	return sizeof(TNodeInstanceMemory);
}
