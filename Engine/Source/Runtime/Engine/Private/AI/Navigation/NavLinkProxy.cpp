// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ObjectEditorUtils.h"
#include "AI/NavigationModifier.h"

ANavLinkProxy::ANavLinkProxy(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("PositionComponent"));
	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	EdRenderComp = PCIP.CreateDefaultSubobject<UNavLinkRenderingComponent>(this, TEXT("EdRenderComp"));
	EdRenderComp->PostPhysicsComponentTick.bCanEverTick = false;
	EdRenderComp->AttachParent = RootComponent;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (!IsRunningCommandlet() && (SpriteComponent != NULL))
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/Engine/EditorResources/AI/S_NavLink"));

		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bVisible = true;

		SpriteComponent->AttachParent = RootComponent;
		SpriteComponent->SetAbsolute(false, false, true);
	}
#endif

	SmartLinkComp = PCIP.CreateDefaultSubobject<USmartNavLinkComponent>(this, TEXT("SmartLinkComp"));
	SmartLinkComp->SetNavigationRelevancy(false);
	SmartLinkComp->SetMoveReachedLink(this, &ANavLinkProxy::NotifySmartLinkReached);
	bSmartLinkIsRelevant = false;

	PointLinks.Add(FNavigationLink());
	SetActorEnableCollision(false);

	bCanBeDamaged = false;
}

#if WITH_EDITOR
void ANavLinkProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ANavLinkProxy,bSmartLinkIsRelevant))
	{
		SmartLinkComp->SetNavigationRelevancy(bSmartLinkIsRelevant);
	}

	UpdateNavigationRelevancy();

	if (PropertyChangedEvent.Property && 
		IsNavigationRelevant() && GetWorld() && GetWorld()->GetNavigationSystem())
	{
		const FName CategoryName = FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property);
		if (CategoryName == TEXT("SimpleLink") ||
			CategoryName == TEXT("SmartLink") ||
			CategoryName == TEXT("Obstacle"))
		{
			GetWorld()->GetNavigationSystem()->UpdateNavOctree(this);
		}
	}
}
#endif // WITH_EDITOR

void ANavLinkProxy::PostInitProperties()
{
	Super::PostInitProperties();

	// NavLinkProxy has no components so we need to update navigation relevancy manually
	UpdateNavigationRelevancy();
}

bool ANavLinkProxy::UpdateNavigationRelevancy()
{ 
	const bool bNewRelevancy = (PointLinks.Num() > 0) || (SegmentLinks.Num() > 0) || SmartLinkComp->IsNavigationRelevant();
	SetNavigationRelevancy(bNewRelevancy);
	return bNewRelevancy; 
}

bool ANavLinkProxy::GetNavigationRelevantData(struct FNavigationRelevantData& Data) const
{
	NavigationHelper::ProcessNavLinkAndAppend(&Data.Modifiers, this, PointLinks);
	NavigationHelper::ProcessNavLinkSegmentAndAppend(&Data.Modifiers, this, SegmentLinks);

	return false;
}

bool ANavLinkProxy::GetNavigationLinksClasses(TArray<TSubclassOf<class UNavLinkDefinition> >& OutClasses) const
{
	return false;
}

bool ANavLinkProxy::GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const
{
	OutLink.Append(PointLinks);
	OutSegments.Append(SegmentLinks);

	if (SmartLinkComp->IsNavigationRelevant())
	{
		OutLink.Add(SmartLinkComp->GetLink());
	}

	return (PointLinks.Num() > 0) || (SegmentLinks.Num() > 0) || SmartLinkComp->IsNavigationRelevant();
}

FBox ANavLinkProxy::GetComponentsBoundingBox(bool bNonColliding) const
{
	FBox LinksBB(0);
	LinksBB += FVector(0,0,-10);
	LinksBB += FVector(0,0,10);

	for (int32 i = 0; i < PointLinks.Num(); ++i)
	{
		const FNavigationLink& Link = PointLinks[i];
		LinksBB += Link.Left;
		LinksBB += Link.Right;
	}

	for (int32 i = 0; i < SegmentLinks.Num(); ++i)
	{
		const FNavigationSegmentLink& SegmentLink = SegmentLinks[i];
		LinksBB += SegmentLink.LeftStart;
		LinksBB += SegmentLink.LeftEnd;
		LinksBB += SegmentLink.RightStart;
		LinksBB += SegmentLink.RightEnd;
	}

	if (SmartLinkComp->IsNavigationRelevant())
	{
		FNavigationLink Link = SmartLinkComp->GetLink();
		LinksBB += Link.Left;
		LinksBB += Link.Right;
	}

	return LinksBB.TransformBy(RootComponent->ComponentToWorld);
}

void ANavLinkProxy::NotifySmartLinkReached(USmartNavLinkComponent* LinkComp, class UPathFollowingComponent* PathComp, const FVector& DestPoint)
{
	AActor* PathOwner = PathComp->GetOwner();
	AController* ControllerOwner = Cast<AController>(PathOwner);
	if (ControllerOwner)
	{
		PathOwner = ControllerOwner->GetPawn();
	}

	ReceiveSmartLinkReached(PathOwner, DestPoint);
	OnSmartLinkReached.Broadcast(PathOwner, DestPoint);
}

void ANavLinkProxy::ResumePathFollowing(AActor* Agent)
{
	if (Agent)
	{
		UPathFollowingComponent* PathComp = Agent->FindComponentByClass<UPathFollowingComponent>();
		if (PathComp == NULL)
		{
			APawn* PawnOwner = Cast<APawn>(Agent);
			if (PawnOwner && PawnOwner->GetController())
			{
				PathComp = PawnOwner->GetController()->FindComponentByClass<UPathFollowingComponent>();
			}
		}

		if (PathComp)
		{
			SmartLinkComp->ResumePathFollowing(PathComp);
		}
	}
}

bool ANavLinkProxy::IsSmartLinkEnabled() const
{
	return SmartLinkComp->IsEnabled();
}

void ANavLinkProxy::SetSmartLinkEnabled(bool bEnabled)
{
	SmartLinkComp->SetEnabled(bEnabled);
}

bool ANavLinkProxy::HasMovingAgents() const
{
	return SmartLinkComp->HasMovingAgents();
}
