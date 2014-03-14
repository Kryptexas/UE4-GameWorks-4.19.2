// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineNavigationClasses.h"
#include "VisualLog.h"

USmartNavLinkComponent::USmartNavLinkComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NavLinkUserId = 0;
	LinkRelativeStart = FVector(70, 0, 0);
	LinkRelativeEnd = FVector(-70, 0, 0);
	LinkDirection = ESmartNavLinkDir::BothWays;
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

void USmartNavLinkComponent::SetLinkData(const FVector& RelativeStart, const FVector& RelativeEnd, ESmartNavLinkDir::Type Direction)
{
	LinkRelativeStart = RelativeStart;
	LinkRelativeEnd = RelativeEnd;
	LinkDirection = Direction;
	
	RefreshNavigationModifiers();
}

FNavigationLink USmartNavLinkComponent::GetLink() const
{
	FNavigationLink LinkMod(LinkRelativeStart, LinkRelativeEnd);
	LinkMod.Direction = LinkDirection == ESmartNavLinkDir::OneWay ? ENavLinkDirection::LeftToRight : ENavLinkDirection::BothWays;
	LinkMod.AreaClass = bLinkEnabled ? EnabledAreaClass : DisabledAreaClass;
	LinkMod.UserId = NavLinkUserId;

	return LinkMod;
}

void USmartNavLinkComponent::SetEnabledArea(TSubclassOf<class UNavArea> AreaClass)
{
	EnabledAreaClass = AreaClass;
	if (IsNavigationRelevant() && bLinkEnabled)
	{
		UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
		NavSys->UpdateSmartLink(this);
	}
}

void USmartNavLinkComponent::SetDisabledArea(TSubclassOf<class UNavArea> AreaClass)
{
	DisabledAreaClass = AreaClass;
	if (IsNavigationRelevant() && !bLinkEnabled)
	{
		UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
		NavSys->UpdateSmartLink(this);
	}
}

void USmartNavLinkComponent::AddNavigationObstacle(TSubclassOf<class UNavArea> AreaClass, const FVector& BoxExtent, const FVector& BoxOffset)
{
	ObstacleOffset = BoxOffset;
	ObstacleExtent = BoxExtent;
	ObstacleAreaClass = AreaClass;
	bCreateBoxObstacle = true;

	RefreshNavigationModifiers();
}

void USmartNavLinkComponent::ClearNavigationObstacle()
{
	ObstacleAreaClass = NULL;
	bCreateBoxObstacle = false;

	RefreshNavigationModifiers();
}

void USmartNavLinkComponent::SetEnabled(bool bNewEnabled)
{
	if (bLinkEnabled != bNewEnabled)
	{
		bLinkEnabled = bNewEnabled;

		UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
		if (NavSys)
		{
			NavSys->UpdateSmartLink(this);
		}

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(this, &USmartNavLinkComponent::BroadcastStateChange);

			if ((bLinkEnabled && bNotifyWhenEnabled) || (!bLinkEnabled && bNotifyWhenDisabled))
			{
				BroadcastStateChange();
			}
		}
	}
}

void USmartNavLinkComponent::SetMoveReachedLink(FOnMoveReachedLink const& InDelegate)
{
	OnMoveReachedLink = InDelegate;
}

void USmartNavLinkComponent::ResumePathFollowing(class UPathFollowingComponent* PathComp)
{
	PathComp->ResumeMove();
}

bool USmartNavLinkComponent::HasMovingAgents() const
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

void USmartNavLinkComponent::OnOwnerRegistered()
{
	if (GetOwner() == NULL || GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL)
	{
		return;
	}

	UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
	if (NavLinkUserId == 0)
	{
		NavLinkUserId = NavSys->FindFreeSmartLinkId();
	}

	NavSys->RegisterSmartLink(this);
}

void USmartNavLinkComponent::OnOwnerUnregistered()
{
	if (GetOwner() == NULL || GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL)
	{
		return;
	}

	// always try to unregister, even if not relevant right now
	UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
	NavSys->UnregisterSmartLink(this);
}

void USmartNavLinkComponent::OnApplyModifiers(FCompositeNavModifier& Modifiers)
{
	FNavigationLink LinkMod = GetLink();
	Modifiers.Add(FSimpleLinkNavModifier(LinkMod, GetOwner()->GetTransform()));

	if (bCreateBoxObstacle)
	{
		Modifiers.Add(FAreaNavModifier(FBox::BuildAABB(ObstacleOffset, ObstacleExtent), GetOwner()->GetTransform(), ObstacleAreaClass));
	}
}

void USmartNavLinkComponent::NotifyLinkReached(class UPathFollowingComponent* PathComp, const FVector& DestPoint)
{
	TWeakObjectPtr<UPathFollowingComponent> WeakPathComp;
	MovingAgents.Add(WeakPathComp);

	if (OnMoveReachedLink.IsBound())
	{
		OnMoveReachedLink.Execute(this, PathComp, DestPoint);
	}
	else
	{
		ResumePathFollowing(PathComp);
	}
}

void USmartNavLinkComponent::NotifyLinkFinished(class UPathFollowingComponent* PathComp)
{
	TWeakObjectPtr<UPathFollowingComponent> WeakPathComp;
	MovingAgents.Remove(WeakPathComp);
}

void USmartNavLinkComponent::NotifyMoveAborted(class UPathFollowingComponent* PathComp)
{
	TWeakObjectPtr<UPathFollowingComponent> WeakPathComp;
	MovingAgents.Remove(WeakPathComp);
}

void USmartNavLinkComponent::SetBroadcastData(float Radius, ECollisionChannel TraceChannel, float Interval)
{
	BroadcastRadius = Radius;
	BroadcastChannel = TraceChannel;
	BroadcastInterval = Interval;
}

void USmartNavLinkComponent::SendBroadcastWhenEnabled(bool bEnabled)
{
	bNotifyWhenEnabled = bEnabled;
}

void USmartNavLinkComponent::SendBroadcastWhenDisabled(bool bEnabled)
{
	bNotifyWhenDisabled = bEnabled;
}

void USmartNavLinkComponent::CollectNearbyAgents(TArray<UNavigationComponent*>& NotifyList)
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

void USmartNavLinkComponent::BroadcastStateChange()
{
	TArray<UNavigationComponent*> NearbyAgents;

	CollectNearbyAgents(NearbyAgents);
	OnBroadcastFilter.ExecuteIfBound(this, NearbyAgents);

	for (int32 i = 0; i < NearbyAgents.Num(); i++)
	{
		NearbyAgents[i]->OnSmartLinkBroadcast(this);
	}

	if (BroadcastInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(this, &USmartNavLinkComponent::BroadcastStateChange, BroadcastInterval);
	}
}

FVector USmartNavLinkComponent::GetStartPoint() const
{
	return GetOwner()->GetTransform().TransformPosition(LinkRelativeStart);
}

FVector USmartNavLinkComponent::GetEndPoint() const
{
	return GetOwner()->GetTransform().TransformPosition(LinkRelativeEnd);
}
