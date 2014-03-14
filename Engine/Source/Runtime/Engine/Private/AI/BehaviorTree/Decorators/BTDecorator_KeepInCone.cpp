// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

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
	
	bUseSelfAsOrigin = true;
	ConeHalfAngle = 45.0f;
}

void UBTDecorator_KeepInCone::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);

	ConeHalfAngleDot = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));

	UBlackboardData* BBAsset = GetBlackboardAsset();
	ConeOrigin.CacheSelectedKey(BBAsset);
	Observed.CacheSelectedKey(BBAsset);
}

bool UBTDecorator_KeepInCone::CalculateCurrentDirection(const UBehaviorTreeComponent* OwnerComp, FVector& Direction) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	FVector PointA = FVector::ZeroVector;
	FVector PointB = FVector::ZeroVector;
	bool bHasPointA = false;
	bool bHasPointB = false;

	if (bUseSelfAsOrigin)
	{
		const AController* ControllerOwner = Cast<AController>(OwnerComp->GetOwner());
		const AActor* ActorOwner = ControllerOwner ? ControllerOwner->GetPawn() : OwnerComp->GetOwner();
		if (ActorOwner)
		{
			PointA = ActorOwner->GetActorLocation();
			bHasPointA = true;
		}
	}
	else if (BlackboardComp && BlackboardComp->GetLocationFromEntry(ConeOrigin.SelectedKeyID, PointA))
	{
		bHasPointA = true;
	}

	if (bUseSelfAsObserved)
	{
		const AController* ControllerOwner = Cast<AController>(OwnerComp->GetOwner());
		const AActor* ActorOwner = ControllerOwner ? ControllerOwner->GetPawn() : OwnerComp->GetOwner();
		if (ActorOwner)
		{
			PointB = ActorOwner->GetActorLocation();
			bHasPointB = true;
		}
	}
	else if (BlackboardComp && BlackboardComp->GetLocationFromEntry(Observed.SelectedKeyID, PointB))
	{
		bHasPointB = true;
	}

	if (bHasPointA && bHasPointB)
	{
		Direction = (PointB - PointA).SafeNormal();
		return true;
	}

	return false;
}

void UBTDecorator_KeepInCone::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	TNodeInstanceMemory* DecoratorMemory = (TNodeInstanceMemory*)NodeMemory;
	FVector InitialDir(1.0f, 0, 0);

	CalculateCurrentDirection(OwnerComp, InitialDir);
	DecoratorMemory->InitialDirection = InitialDir;
}

void UBTDecorator_KeepInCone::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
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
	const FString OriginString = bUseSelfAsOrigin ? TEXT("Self") : *ConeOrigin.SelectedKeyName.ToString();
	const FString ObservedString = bUseSelfAsObserved ? TEXT("Self") : *Observed.SelectedKeyName.ToString();

	return FString::Printf(TEXT("%s: %s in %.2f degree cone of initial direction: %s-%s"),
		*Super::GetStaticDescription(), *ObservedString, ConeHalfAngle * 2, *OriginString, *ObservedString);
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
