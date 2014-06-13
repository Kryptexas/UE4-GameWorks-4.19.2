// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationTypes.h"
#include "Ai/Navigation/NavRelevantActorInterface.h"
#include "NavLinkProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FSmartLinkReachedSignature, class AActor*, MovingActor, const FVector&, DestinationPoint );

UCLASS(Blueprintable)
class ENGINE_API ANavLinkProxy : public AActor, public INavLinkHostInterface, public INavRelevantActorInterface
{
	GENERATED_UCLASS_BODY()

	/** Navigation links (point to point) added to navigation data */
	UPROPERTY(EditAnywhere, Category=SimpleLink)
	TArray<FNavigationLink> PointLinks;
	
	/** Navigation links (segment to segment) added to navigation data */
	UPROPERTY(EditAnywhere, Category=SimpleLink)
	TArray<FNavigationSegmentLink> SegmentLinks;

	/** Smart link: can affect path following */
	UPROPERTY(VisibleAnywhere, Category=SmartLink)
	TSubobjectPtr<class UNavLinkCustomComponent> SmartLinkComp;

	/** Smart link: toggle relevancy */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	bool bSmartLinkIsRelevant;

#if WITH_EDITORONLY_DATA
	/** Editor Preview */
	UPROPERTY()
	TSubobjectPtr<class UNavLinkRenderingComponent> EdRenderComp;

	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;
#endif // WITH_EDITORONLY_DATA

	// BEGIN INavRelevantActorInterface
	virtual bool UpdateNavigationRelevancy() override;
	virtual bool GetNavigationRelevantData(struct FNavigationRelevantData& Data) const override;
	// END INavRelevantActorInterface

	// BEGIN INavLinkHostInterface
	virtual bool GetNavigationLinksClasses(TArray<TSubclassOf<class UNavLinkDefinition> >& OutClasses) const override;
	virtual bool GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const override;
	// END INavLinkHostInterface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	virtual void PostInitProperties() override;
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false) const override;

	//////////////////////////////////////////////////////////////////////////
	// Blueprint interface for smart links

	/** called when agent reaches smart link during path following, use ResumePathFollowing() to give control back */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveSmartLinkReached(AActor* Agent, const FVector& Destination);

	/** resume normal path following */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	void ResumePathFollowing(AActor* Agent);

	/** check if smart link is enabled */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	bool IsSmartLinkEnabled() const;

	/** change state of smart link */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	void SetSmartLinkEnabled(bool bEnabled);

	/** check if any agent is moving through smart link right now */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	bool HasMovingAgents() const;

protected:

	UPROPERTY(BlueprintAssignable)
	FSmartLinkReachedSignature OnSmartLinkReached;

	void NotifySmartLinkReached(UNavLinkCustomComponent* LinkComp, class UPathFollowingComponent* PathComp, const FVector& DestPoint);
};
