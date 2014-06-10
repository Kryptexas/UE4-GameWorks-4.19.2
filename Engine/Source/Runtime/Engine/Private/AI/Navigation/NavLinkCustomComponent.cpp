// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"
#include "AI/Navigation/NavAreas/NavArea_Null.h"
#include "AI/Navigation/NavAreas/NavArea_Default.h"
// @todo to be addressed when removing AIModule circular dependency
#include "Navigation/PathFollowingComponent.h"
#include "Navigation/NavigationComponent.h"
#include "AI/Navigation/NavLinkCustomComponent.h"

UNavLinkCustomComponent::UNavLinkCustomComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NavLinkUserId = 0;
	LinkRelativeStart = FVector(70, 0, 0);
	LinkRelativeEnd = FVector(-70, 0, 0);
	LinkDirection = ENavLinkDirection::BothWays;
	EnabledAreaClass = UNavArea_Default::StaticClass();
	DisabledAreaClass = UNavArea_Null::StaticClass();
	ObstacleAreaClass = UNavArea_Null::StaticClass();
	ObstacleExtent = FVector(50, 50, 50);
	bLinkEnabled = true;
	bNotifyWhenEnabled = false;
	bNotifyWhenDisabled = false;
	bCreateBoxObstacle = false;
	BroadcastRadius = 0.0f;
	BroadcastChannel = ECC_Pawn;
	BroadcastInterval = 0.0f;
}

void UNavLinkCustomComponent::GetLinkData(FVector& LeftPt, FVector& RightPt, ENavLinkDirection::Type& Direction) const
{
	LeftPt = LinkRelativeStart;
	RightPt = LinkRelativeEnd;
	Direction = LinkDirection;
}

TSubclassOf<UNavArea> UNavLinkCustomComponent::GetLinkAreaClass() const
{
	return bLinkEnabled ? EnabledAreaClass : DisabledAreaClass;
}

uint32 UNavLinkCustomComponent::GetLinkId() const
{
	return NavLinkUserId;
}

bool UNavLinkCustomComponent::IsLinkPathfindingAllowed(const UObject* Querier) const
{
	return true;
}

bool UNavLinkCustomComponent::OnLinkMoveStarted(class UPathFollowingComponent* PathComp, const FVector& DestPoint)
{
	TWeakObjectPtr<UPathFollowingComponent> WeakPathComp = PathComp;
	MovingAgents.Add(WeakPathComp);

	if (OnMoveReachedLink.IsBound())
	{
		OnMoveReachedLink.Execute(this, PathComp, DestPoint);
		return true;
	}

	return false;
}

void UNavLinkCustomComponent::OnLinkMoveFinished(class UPathFollowingComponent* PathComp)
{
	TWeakObjectPtr<UPathFollowingComponent> WeakPathComp;
	MovingAgents.Remove(WeakPathComp);
}

void UNavLinkCustomComponent::OnOwnerRegistered()
{
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys == NULL)
	{
		return;
	}

	if (NavLinkUserId == 0)
	{
		NavLinkUserId = INavLinkCustomInterface::GetUniqueId();
	}

	NavSys->RegisterCustomLink(this);
}

void UNavLinkCustomComponent::OnOwnerUnregistered()
{
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys == NULL)
	{
		return;
	}

	// always try to unregister, even if not relevant right now
	NavSys->UnregisterCustomLink(this);
}

void UNavLinkCustomComponent::OnApplyModifiers(struct FCompositeNavModifier& Modifiers)
{
	FNavigationLink LinkMod = GetLinkModifier();
	Modifiers.Add(FSimpleLinkNavModifier(LinkMod, GetOwner()->GetTransform()));

	if (bCreateBoxObstacle)
	{
		Modifiers.Add(FAreaNavModifier(FBox::BuildAABB(ObstacleOffset, ObstacleExtent), GetOwner()->GetTransform(), ObstacleAreaClass));
	}
}

void UNavLinkCustomComponent::SetLinkData(const FVector& RelativeStart, const FVector& RelativeEnd, ENavLinkDirection::Type Direction)
{
	LinkRelativeStart = RelativeStart;
	LinkRelativeEnd = RelativeEnd;
	LinkDirection = Direction;
	
	RefreshNavigationModifiers();
}

FNavigationLink UNavLinkCustomComponent::GetLinkModifier() const
{
	return INavLinkCustomInterface::GetModifier(this);
}

void UNavLinkCustomComponent::SetEnabledArea(TSubclassOf<class UNavArea> AreaClass)
{
	EnabledAreaClass = AreaClass;
	if (IsNavigationRelevant() && bLinkEnabled)
	{
		UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
		NavSys->UpdateCustomLink(this);
	}
}

void UNavLinkCustomComponent::SetDisabledArea(TSubclassOf<class UNavArea> AreaClass)
{
	DisabledAreaClass = AreaClass;
	if (IsNavigationRelevant() && !bLinkEnabled)
	{
		UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
		NavSys->UpdateCustomLink(this);
	}
}

void UNavLinkCustomComponent::AddNavigationObstacle(TSubclassOf<class UNavArea> AreaClass, const FVector& BoxExtent, const FVector& BoxOffset)
{
	ObstacleOffset = BoxOffset;
	ObstacleExtent = BoxExtent;
	ObstacleAreaClass = AreaClass;
	bCreateBoxObstacle = true;

	RefreshNavigationModifiers();
}

void UNavLinkCustomComponent::ClearNavigationObstacle()
{
	ObstacleAreaClass = NULL;
	bCreateBoxObstacle = false;

	RefreshNavigationModifiers();
}

void UNavLinkCustomComponent::OnRegister()
{
	Super::OnRegister();

	const AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		FBox NewBounds(0);
		NewBounds += MyOwner->GetActorLocation();
		NewBounds += MyOwner->GetActorLocation() + LinkRelativeStart;
		NewBounds += MyOwner->GetActorLocation() + LinkRelativeEnd;
		
		if (bCreateBoxObstacle)
		{
			NewBounds += FBox::BuildAABB(MyOwner->GetActorLocation() + ObstacleOffset, ObstacleExtent);
		}

		Bounds = NewBounds;
	}
}

void UNavLinkCustomComponent::SetEnabled(bool bNewEnabled)
{
	if (bLinkEnabled != bNewEnabled)
	{
		bLinkEnabled = bNewEnabled;

		UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
		if (NavSys)
		{
			NavSys->UpdateCustomLink(this);
		}

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(this, &UNavLinkCustomComponent::BroadcastStateChange);

			if ((bLinkEnabled && bNotifyWhenEnabled) || (!bLinkEnabled && bNotifyWhenDisabled))
			{
				BroadcastStateChange();
			}
		}
	}
}

void UNavLinkCustomComponent::SetMoveReachedLink(FOnMoveReachedLink const& InDelegate)
{
	OnMoveReachedLink = InDelegate;
}

bool UNavLinkCustomComponent::HasMovingAgents() const
{
	for (int32 i = 0; i < MovingAgents.Num(); i++)
	{
		if (MovingAgents[i].IsValid())
		{
			return true;
		}
	}

	return false;
}

void UNavLinkCustomComponent::SetBroadcastData(float Radius, ECollisionChannel TraceChannel, float Interval)
{
	BroadcastRadius = Radius;
	BroadcastChannel = TraceChannel;
	BroadcastInterval = Interval;
}

void UNavLinkCustomComponent::SendBroadcastWhenEnabled(bool bEnabled)
{
	bNotifyWhenEnabled = bEnabled;
}

void UNavLinkCustomComponent::SendBroadcastWhenDisabled(bool bEnabled)
{
	bNotifyWhenDisabled = bEnabled;
}

void UNavLinkCustomComponent::CollectNearbyAgents(TArray<UNavigationComponent*>& NotifyList)
{
	AActor* MyOwner = GetOwner();
	if (BroadcastRadius < KINDA_SMALL_NUMBER || MyOwner == NULL)
	{
		return;
	}

	static FName SmartLinkBroadcastTrace(TEXT("SmartLinkBroadcastTrace"));
	FCollisionQueryParams Params(SmartLinkBroadcastTrace, false, MyOwner);
	TArray<FOverlapResult> OverlapsL, OverlapsR;

	const FVector LocationL = GetStartPoint();
	const FVector LocationR = GetEndPoint();
	const float LinkDistSq = (LocationL - LocationR).SizeSquared();
	const float DistThresholdSq = FMath::Square(BroadcastRadius * 0.25f);
	if (LinkDistSq > DistThresholdSq)
	{
		GetWorld()->OverlapMulti(OverlapsL, LocationL, FQuat::Identity, BroadcastChannel, FCollisionShape::MakeSphere(BroadcastRadius), Params);
		GetWorld()->OverlapMulti(OverlapsR, LocationR, FQuat::Identity, BroadcastChannel, FCollisionShape::MakeSphere(BroadcastRadius), Params);
	}
	else
	{
		const FVector MidPoint = (LocationL + LocationR) * 0.5f;
		GetWorld()->OverlapMulti(OverlapsL, MidPoint, FQuat::Identity, BroadcastChannel, FCollisionShape::MakeSphere(BroadcastRadius), Params);
	}

	TArray<APawn*> PawnList;
	for (int32 i = 0; i < OverlapsL.Num(); i++)
	{
		APawn* MovingPawn = Cast<APawn>(OverlapsL[i].GetActor());
		if (MovingPawn && MovingPawn->GetController())
		{
			PawnList.Add(MovingPawn);
		}
	}
	for (int32 i = 0; i < OverlapsR.Num(); i++)
	{
		APawn* MovingPawn = Cast<APawn>(OverlapsR[i].GetActor());
		if (MovingPawn && MovingPawn->GetController())
		{
			PawnList.AddUnique(MovingPawn);
		}
	}

	for (int32 i = 0; i < PawnList.Num(); i++)
	{
		UNavigationComponent* NavComp = PawnList[i]->GetController()->FindComponentByClass<UNavigationComponent>();
		if (NavComp && NavComp->WantsSmartLinkUpdates())
		{
			NotifyList.Add(NavComp);
		}
	}
}

void UNavLinkCustomComponent::BroadcastStateChange()
{
	TArray<UNavigationComponent*> NearbyAgents;

	CollectNearbyAgents(NearbyAgents);
	OnBroadcastFilter.ExecuteIfBound(this, NearbyAgents);

	for (int32 i = 0; i < NearbyAgents.Num(); i++)
	{
		NearbyAgents[i]->OnCustomLinkBroadcast(this);
	}

	if (BroadcastInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(this, &UNavLinkCustomComponent::BroadcastStateChange, BroadcastInterval);
	}
}

FVector UNavLinkCustomComponent::GetStartPoint() const
{
	return GetOwner()->GetTransform().TransformPosition(LinkRelativeStart);
}

FVector UNavLinkCustomComponent::GetEndPoint() const
{
	return GetOwner()->GetTransform().TransformPosition(LinkRelativeEnd);
}
