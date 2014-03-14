// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationTypes.h"
#include "SmartNavLinkComponent.generated.h"

/**
 *  Navigation smart object
 *
 *  Creates and manage navigation link (point to point type) that:
 *  - supports custom path following
 *  - can be toggled
 */

UENUM()
namespace ESmartNavLinkDir
{
	enum Type
	{
		OneWay,
		BothWays,
	};
}

UCLASS(HeaderGroup=Navigation)
class ENGINE_API USmartNavLinkComponent : public UNavRelevantComponent
{
	GENERATED_UCLASS_BODY()

	DECLARE_DELEGATE_ThreeParams(FOnMoveReachedLink, USmartNavLinkComponent* /*ThisComp*/, class UPathFollowingComponent* /*PathComp*/, const FVector& /*DestPoint*/); 
	DECLARE_DELEGATE_TwoParams(FBroadcastFilter, USmartNavLinkComponent* /*ThisComp*/, TArray<class UNavigationComponent*>& /*NotifyList*/); 

	/** set basic link data: end points and direction */
	void SetLinkData(const FVector& RelativeStart, const FVector& RelativeEnd, ESmartNavLinkDir::Type Direction);
	FNavigationLink GetLink() const;

	/** set area class to use when link is enabled */
	void SetEnabledArea(TSubclassOf<class UNavArea> AreaClass);
	TSubclassOf<UNavArea> GetEnabledArea() const { return EnabledAreaClass; }

	/** set area class to use when link is disabled */
	void SetDisabledArea(TSubclassOf<class UNavArea> AreaClass);
	TSubclassOf<UNavArea> GetDisabledArea() const { return DisabledAreaClass; }

	/** add box obstacle during generation of navigation data
	  * this can be used to create empty area under doors */
	void AddNavigationObstacle(TSubclassOf<class UNavArea> AreaClass, const FVector& BoxExtent, const FVector& BoxOffset = FVector::ZeroVector);

	/** removes simple obstacle */
	void ClearNavigationObstacle();

	/** set properties of trigger around link entry point(s), that will notify nearby agents about link state change */
	void SetBroadcastData(float Radius, ECollisionChannel TraceChannel = ECC_Pawn, float Interval = 0.0f);

	void SendBroadcastWhenEnabled(bool bEnabled);
	void SendBroadcastWhenDisabled(bool bEnabled);

	/** set delegate to filter  */
	void SetBroadcastFilter(FBroadcastFilter const& InDelegate);

	/** change state of smart link (used area class) */
	void SetEnabled(bool bNewEnabled);
	bool IsEnabled() const { return bLinkEnabled; }
	
	/** get stored link ID */
	uint32 GetLinkId() const { return NavLinkUserId; }

	/** set delegate to notify about reaching this link during path following */
	void SetMoveReachedLink(FOnMoveReachedLink const& InDelegate);

	/** pass movement control back to path following component and travel in straight line to other end of navigation link */
	void ResumePathFollowing(class UPathFollowingComponent* PathComp);

	/** check is any agent is currently moving though this link */
	bool HasMovingAgents() const;

	/** get link start point in world space */
	FVector GetStartPoint() const;

	/** get link end point in world space */
	FVector GetEndPoint() const;

	//////////////////////////////////////////////////////////////////////////
	// notifies from navigation system and path following

	virtual void OnOwnerRegistered() OVERRIDE;
	virtual void OnOwnerUnregistered() OVERRIDE;
	virtual void OnApplyModifiers(struct FCompositeNavModifier& Modifiers) OVERRIDE;

	/** called by path following when agent needs to go through this link
	  * will trigger OnMoveReachedLink delegate, or call ResumePathFollowing() when it's not bound */
	void NotifyLinkReached(class UPathFollowingComponent* PathComp, const FVector& DestPoint);

	/** called by path following when agent finished moving though this link */
	void NotifyLinkFinished(class UPathFollowingComponent* PathComp);

	/** called by path following when movement gets aborted while agent is moving though this link */
	void NotifyMoveAborted(class UPathFollowingComponent* PathComp);

	//////////////////////////////////////////////////////////////////////////
	// helper functions for setting delegates

	template< class UserClass >	
	FORCEINLINE void SetMoveReachedLink(UserClass* TargetOb, typename FOnMoveReachedLink::TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc)
	{
		SetMoveReachedLink(FOnMoveReachedLink::CreateUObject(TargetOb, InFunc));
	}
	template< class UserClass >	
	FORCEINLINE void SetMoveReachedLink(UserClass* TargetOb, typename FOnMoveReachedLink::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc)
	{
		SetMoveReachedLink(FOnMoveReachedLink::CreateUObject(TargetOb, InFunc));
	}

	template< class UserClass >	
	FORCEINLINE void SetBroadcastFilter(UserClass* TargetOb, typename FBroadcastFilter::TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc)
	{
		SetBroadcastFilter(FBroadcastFilter::CreateUObject(TargetOb, InFunc));
	}
	template< class UserClass >	
	FORCEINLINE void SetBroadcastFilter(UserClass* TargetOb, typename FBroadcastFilter::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc)
	{
		SetBroadcastFilter(FBroadcastFilter::CreateUObject(TargetOb, InFunc));
	}
	
protected:

	/** link Id assigned by navigation system */
	UPROPERTY()
	uint32 NavLinkUserId;

	/** area class to use when link is enabled */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	TSubclassOf<class UNavArea> EnabledAreaClass;

	/** area class to use when link is disabled */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	TSubclassOf<class UNavArea> DisabledAreaClass;

	/** start point, relative to owner */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	FVector LinkRelativeStart;

	/** end point, relative to owner */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	FVector LinkRelativeEnd;

	/** direction of link */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	TEnumAsByte<ESmartNavLinkDir::Type> LinkDirection;

	/** is link currently in enabled state? (area class) */
	UPROPERTY(EditAnywhere, Category=SmartLink)
	uint32 bLinkEnabled : 1;

	/** should link notify nearby agents when it changes state to enabled */
	UPROPERTY(EditAnywhere, Category=Broadcast)
	uint32 bNotifyWhenEnabled : 1;

	/** should link notify nearby agents when it changes state to disabled */
	UPROPERTY(EditAnywhere, Category=Broadcast)
	uint32 bNotifyWhenDisabled : 1;

	/** if set, box obstacle area will be added to generation */
	UPROPERTY(EditAnywhere, Category=Obstacle)
	uint32 bCreateBoxObstacle : 1;

	/** offset of simple box obstacle */
	UPROPERTY(EditAnywhere, Category=Obstacle)
	FVector ObstacleOffset;

	/** extent of simple box obstacle */
	UPROPERTY(EditAnywhere, Category=Obstacle)
	FVector ObstacleExtent;

	/** area class for simple box obstacle */
	UPROPERTY(EditAnywhere, Category=Obstacle)
	TSubclassOf<class UNavArea> ObstacleAreaClass;

	/** radius of state change broadcast */
	UPROPERTY(EditAnywhere, Category=Broadcast)
	float BroadcastRadius;

	/** interval for state change broadcast (0 = single broadcast) */
	UPROPERTY(EditAnywhere, Category=Broadcast, Meta=(UIMin="0.0", ClampMin="0.0"))
	float BroadcastInterval;

	/** trace channel for state change broadcast */
	UPROPERTY(EditAnywhere, Category=Broadcast)
	TEnumAsByte<ECollisionChannel> BroadcastChannel;

	/** delegate to call when link is reached */
	FBroadcastFilter OnBroadcastFilter;

	/** list of agents moving though this link */
	TArray<TWeakObjectPtr<class UPathFollowingComponent> > MovingAgents;

	/** delegate to call when link is reached */
	FOnMoveReachedLink OnMoveReachedLink;

	/** notify nearby agents about link changing state */
	void BroadcastStateChange();
	
	/** gather agents to notify about state change */
	void CollectNearbyAgents(TArray<UNavigationComponent*>& NotifyList);
};
