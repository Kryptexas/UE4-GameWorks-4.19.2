// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "WidgetInteractionComponent.generated.h"

class UPrimitiveComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoveredWidgetChanged);

/**
 * This is a highly experimental component to allow interaction with the Widget Component.  Not
 * everything should be expected to work correctly.  This class allows you to simulate a sort of
 * laser pointer device, when it hovers over widgets it will send the basic signals to show as if the
 * mouse were moving on top of it.  You'll then tell the component to simulate key presses, like
 * Left Mouse, down and up, to simulate a mouse click.
 */
UCLASS(ClassGroup=Experimental, meta=(BlueprintSpawnableComponent, DevelopmentStatus=Experimental) )
class UMG_API UWidgetInteractionComponent : public UArrowComponent
{
	GENERATED_BODY()

public:
	/**  */
	UPROPERTY(BlueprintAssignable, Category="Interaction|Event")
	FOnHoveredWidgetChanged OnHoveredWidgetChanged;

public:
	UWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin ActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UActorComponent
	
	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void PressPointerKey(FKey Key);
	
	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void ReleasePointerKey(FKey Key);

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual bool PressKey(FKey Key, bool bRepeat = false);
	
	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual bool ReleaseKey(FKey Key);

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual bool PressAndReleaseKey(FKey Key);

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual bool SendKeyChar(FString Character, bool bRepeat = false);
	
	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void ScrollWheel(float ScrollDelta);
	
	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	UWidgetComponent* GetHoveredWidget();

	/**
	 * Gets the last position in world space that the interaction component successfully hit a widget component.
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	FVector GetLastImpactPoint();

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetCustomHitResult(const FHitResult& HitResult);

private:
	/**
	 * Represents the virtual user in slate.  When this component is registered, it gets a handle to the 
	 * virtual slate user it will be, so virtual slate user 0, is probably real slate user 8, as that's the first
	 * index by default that virtual users begin - the goal is to never have them overlap with real input
	 * hardware as that will likely conflict with focus states you don't actually want to change - like where
	 * the mouse and keyboard focus input (the viewport), so that things like the player controller recieve
	 * standard hardware input.
	 */
	TSharedPtr<FSlateVirtualUser> VirtualUser;

public:

	/**
	 * Represents the Virtual User Index.  Each virtual user should be represented by a different 
	 * index number, this will maintain separate capture and focus states for them.  Each
	 * controller or finger-tip should get a unique PointerIndex.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=( ClampMin = "0", ExposeOnSpawn = true ))
	int32 VirtualUserIndex;

	/**
	 * Each user virtual controller or virtual finger tips being simulated should use a different pointer index.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=( UIMin = "0", UIMax = "9", ClampMin = "0", ExposeOnSpawn = true ))
	float PointerIndex;

public:

	/**
	 * The distance in game units the component should be able to interact with a widget component.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	float InteractionDistance;

	/**
	 * Should we project from the world location of the component?  If you set this to false, you'll
	 * need to call SetCustomHitResult(), and provide the result of a custom hit test form whatever
	 * location you wish.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bAutomaticHitTesting;

	/**
	 * Should the interaction component perform hit testing (automatic or custom) and attempt to simulate hover - if you were going
	 * to emulate a keyboard you would want to turn this option off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bSimulatePointerMovement;

	/**
	 * Shows some debugging lines and a hit sphere to help you debug interactions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debugging")
	bool bShowDebug;

	/**
	 * Determines the color of the debug lines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debugging")
	FLinearColor DebugColor;

protected:

	/**  */
	bool CanSendInput();

	/**  */
	void SimulatePointerMovement();

	/**  */
	virtual bool PerformTrace(FHitResult& HitResult);
	
	/**  */
	void GetRelatedComponentsToIgnoreInAutomaticHitTesting(TArray<UPrimitiveComponent*>& IgnorePrimitives);

protected:

	/** */
	FHitResult CustomHitResult;

	/**  */
	FWeakWidgetPath LastWigetPath;

	/** */
	FModifierKeysState ModifierKeys;
	
	/**  */
	TSet<FKey> PressedKeys;
	
	/**  */
	UPROPERTY()
	FVector2D LocalHitLocation;
	
	/**  */
	UPROPERTY()
	FVector2D LastLocalHitLocation;

	/**  */
	UPROPERTY(Transient)
	UWidgetComponent* HoveredWidget;

	/**  */
	UPROPERTY(Transient)
	FVector LastImpactPoint;
};
