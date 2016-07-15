// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "WidgetComponent.h"
#include "WidgetLayoutLibrary.h"
#include "WidgetInteractionComponent.h"

#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "WidgetInteraction"

UWidgetInteractionComponent::UWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, VirtualUserIndex(0)
	, PointerIndex(0)
	, InteractionDistance(500)
	, bAutomaticHitTesting(true)
	, bSimulatePointerMovement(true)
	, bShowDebug(false)
	, DebugColor(FLinearColor::Red)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWidgetInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if ( FSlateApplication::IsInitialized() )
	{
		if ( !VirtualUser.IsValid() )
		{
			VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualUserIndex);
		}
	}
}

void UWidgetInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if ( FSlateApplication::IsInitialized() )
	{
		if ( VirtualUser.IsValid() )
		{
			FSlateApplication::Get().UnregisterUser(VirtualUser->GetUserIndex());
			VirtualUser.Reset();
		}
	}
}

void UWidgetInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SimulatePointerMovement();
}

bool UWidgetInteractionComponent::CanSendInput()
{
	return FSlateApplication::IsInitialized() && VirtualUser.IsValid();
}

void UWidgetInteractionComponent::SetCustomHitResult(const FHitResult& HitResult)
{
	CustomHitResult = HitResult;
}

bool UWidgetInteractionComponent::PerformTrace(FHitResult& HitResult)
{
	if ( bAutomaticHitTesting )
	{
		const FVector WorldLocation = GetComponentLocation();
		const FTransform WorldTransform = GetComponentTransform();
		const FVector Direction = WorldTransform.GetUnitAxis(EAxis::X);

		TArray<UPrimitiveComponent*> PrimitiveChildren;
		GetRelatedComponentsToIgnoreInAutomaticHitTesting(PrimitiveChildren);

		FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
		Params.AddIgnoredComponents(PrimitiveChildren);

		FCollisionObjectQueryParams Everything(FCollisionObjectQueryParams::AllObjects);
		return GetWorld()->LineTraceSingleByObjectType(HitResult, WorldLocation, WorldLocation + (Direction * InteractionDistance), Everything, Params);
	}
	else
	{
		HitResult = CustomHitResult;
		return HitResult.bBlockingHit;
	}
}

void UWidgetInteractionComponent::GetRelatedComponentsToIgnoreInAutomaticHitTesting(TArray<UPrimitiveComponent*>& IgnorePrimitives)
{
	TArray<USceneComponent*> SceneChildren;
	if ( AActor* Owner = GetOwner() )
	{
		if ( USceneComponent* Root = Owner->GetRootComponent() )
		{
			Root = Root->GetAttachmentRoot();
			Root->GetChildrenComponents(true, SceneChildren);
			SceneChildren.Add(Root);
		}
	}

	for ( USceneComponent* SceneComponent : SceneChildren )
	{
		if ( UPrimitiveComponent* PrimtiveComponet = Cast<UPrimitiveComponent>(SceneComponent) )
		{
			// Don't ignore widget components that are siblings.
			if ( SceneComponent->IsA<UWidgetComponent>() )
			{
				continue;
			}

			IgnorePrimitives.Add(PrimtiveComponet);
		}
	}
}

void UWidgetInteractionComponent::SimulatePointerMovement()
{
	if ( !bSimulatePointerMovement )
	{
		return;
	}

	if ( !CanSendInput() )
	{
		return;
	}
	
	LocalHitLocation = FVector2D(0, 0);
	FWidgetPath WidgetPathUnderFinger;
	
	FHitResult HitResult;
	const bool bHit = PerformTrace(HitResult);

	UWidgetComponent* OldHoveredWidget = HoveredWidget;
	
	HoveredWidget = nullptr;
	LastImpactPoint = FVector(0, 0, 0);

	if (bHit)
	{
		HoveredWidget = Cast<UWidgetComponent>(HitResult.GetComponent());
		if ( HoveredWidget )
		{
			LastImpactPoint = HitResult.ImpactPoint;
			
			HoveredWidget->GetLocalHitLocation( HitResult.ImpactPoint, LocalHitLocation );
			WidgetPathUnderFinger = FWidgetPath(HoveredWidget->GetHitWidgetPath(HitResult.ImpactPoint, /*bIgnoreEnabledStatus*/ false));
		}
	}

	if ( bShowDebug && bAutomaticHitTesting )
	{
		const FVector WorldLocation = GetComponentLocation();

		FTransform WorldTransform = GetComponentTransform();
		const FVector Direction = WorldTransform.GetUnitAxis(EAxis::X);

		if ( HoveredWidget )
		{
			UKismetSystemLibrary::DrawDebugSphere(this, LastImpactPoint, 5, 15, DebugColor, 0, 2);
			UKismetSystemLibrary::DrawDebugLine(this, WorldLocation, LastImpactPoint, DebugColor, 0, 2);
		}
		else
		{
			UKismetSystemLibrary::DrawDebugLine(this, WorldLocation, WorldLocation + ( Direction * InteractionDistance ), DebugColor, 0, 2);
		}
	}
	
	FPointerEvent PointerEvent(
		VirtualUser->GetUserIndex(),
		PointerIndex,
		LocalHitLocation,
		LastLocalHitLocation,
		PressedKeys,
		FKey(),
		0.0f,
		ModifierKeys);
	
	if (WidgetPathUnderFinger.IsValid())
	{
		check(HoveredWidget);
		LastWigetPath = WidgetPathUnderFinger;
		
		FSlateApplication::Get().RoutePointerMoveEvent(WidgetPathUnderFinger, PointerEvent, false);
	}
	else
	{
		FWidgetPath EmptyWidgetPath;
		FSlateApplication::Get().RoutePointerMoveEvent(EmptyWidgetPath, PointerEvent, false);

		LastWigetPath = FWeakWidgetPath();
	}

	if ( HoveredWidget )
	{
		HoveredWidget->RequestRedraw();
	}

	LastLocalHitLocation = LocalHitLocation;

	if ( HoveredWidget != OldHoveredWidget )
	{
		if ( OldHoveredWidget )
		{
			OldHoveredWidget->RequestRedraw();
		}

		OnHoveredWidgetChanged.Broadcast();
	}
}

void UWidgetInteractionComponent::PressPointerKey(FKey Key)
{
	if ( !CanSendInput() )
	{
		return;
	}

	if (PressedKeys.Contains(Key))
	{
		return;
	}
	
	PressedKeys.Add(Key);
	
	FWidgetPath WidgetPathUnderFinger = LastWigetPath.ToWidgetPath();
		
	FPointerEvent PointerEvent(
		VirtualUser->GetUserIndex(),
		PointerIndex,
		LocalHitLocation,
		LastLocalHitLocation,
		PressedKeys,
		Key,
		0.0f,
		ModifierKeys);
		
	FReply Reply = FSlateApplication::Get().RoutePointerDownEvent(WidgetPathUnderFinger, PointerEvent);
	
	// @TODO Something about double click, expose directly, or automatically do it if key press happens within
	// the double click timeframe?
	//Reply = FSlateApplication::Get().RoutePointerDoubleClickEvent( WidgetPathUnderFinger, PointerEvent );

	if ( HoveredWidget )
	{
		HoveredWidget->RequestRedraw();
	}
}

void UWidgetInteractionComponent::ReleasePointerKey(FKey Key)
{
	if ( !CanSendInput() )
	{
		return;
	}

	if (!PressedKeys.Contains(Key))
	{
		return;
	}
	
	PressedKeys.Remove(Key);
	
	FWidgetPath WidgetPathUnderFinger = LastWigetPath.ToWidgetPath();
		
	FPointerEvent PointerEvent(
		VirtualUser->GetUserIndex(),
		PointerIndex,
		LocalHitLocation,
		LastLocalHitLocation,
		PressedKeys,
		Key,
		0.0f,
		ModifierKeys);
		
	FReply Reply = FSlateApplication::Get().RoutePointerUpEvent(WidgetPathUnderFinger, PointerEvent);

	if ( HoveredWidget )
	{
		HoveredWidget->RequestRedraw();
	}
}

bool UWidgetInteractionComponent::PressKey(FKey Key, bool bRepeat)
{
	if ( !CanSendInput() )
	{
		return false;
	}

	const uint32* KeyCodePtr;
	const uint32* CharCodePtr;
	FInputKeyManager::Get().GetCodesFromKey(Key, KeyCodePtr, CharCodePtr);

	uint32 KeyCode = KeyCodePtr ? *KeyCodePtr : 0;
	uint32 CharCode = CharCodePtr ? *CharCodePtr : 0;

	FKeyEvent KeyEvent(Key, ModifierKeys, VirtualUser->GetUserIndex(), bRepeat, KeyCode, CharCode);
	bool DownResult = FSlateApplication::Get().ProcessKeyDownEvent(KeyEvent);

	if ( CharCodePtr )
	{
		FCharacterEvent CharacterEvent(CharCode, ModifierKeys, VirtualUser->GetUserIndex(), bRepeat);
		return FSlateApplication::Get().ProcessKeyCharEvent(CharacterEvent);
	}

	return DownResult;
}

bool UWidgetInteractionComponent::ReleaseKey(FKey Key)
{
	if ( !CanSendInput() )
	{
		return false;
	}

	const uint32* KeyCodePtr;
	const uint32* CharCodePtr;
	FInputKeyManager::Get().GetCodesFromKey(Key, KeyCodePtr, CharCodePtr);

	uint32 KeyCode = KeyCodePtr ? *KeyCodePtr : 0;
	uint32 CharCode = CharCodePtr ? *CharCodePtr : 0;

	FKeyEvent KeyEvent(Key, ModifierKeys, VirtualUser->GetUserIndex(), false, KeyCode, CharCode);
	return FSlateApplication::Get().ProcessKeyUpEvent(KeyEvent);
}

bool UWidgetInteractionComponent::PressAndReleaseKey(FKey Key)
{
	const bool PressResult = PressKey(Key, false);
	const bool ReleaseResult = ReleaseKey(Key);

	return PressResult || ReleaseResult;
}

bool UWidgetInteractionComponent::SendKeyChar(FString Character, bool bRepeat)
{
	if ( !CanSendInput() )
	{
		return false;
	}

	if ( Character.Len() == 0 )
	{
		return false;
	}

	TCHAR CharKey = Character[0];

	FCharacterEvent CharacterEvent(CharKey, ModifierKeys, VirtualUser->GetUserIndex(), bRepeat);
	return FSlateApplication::Get().ProcessKeyCharEvent(CharacterEvent);
}

void UWidgetInteractionComponent::ScrollWheel(float ScrollDelta)
{
	if ( !CanSendInput() )
	{
		return;
	}

	FWidgetPath WidgetPathUnderFinger = LastWigetPath.ToWidgetPath();
	
	FPointerEvent MouseWheelEvent(
		VirtualUser->GetUserIndex(),
		PointerIndex,
		LocalHitLocation,
		LastLocalHitLocation,
		PressedKeys,
		EKeys::MouseWheelAxis,
		ScrollDelta,
		ModifierKeys);

	FSlateApplication::Get().RouteMouseWheelOrGestureEvent(WidgetPathUnderFinger, MouseWheelEvent, nullptr);

	if ( HoveredWidget )
	{
		HoveredWidget->RequestRedraw();
	}
}

UWidgetComponent* UWidgetInteractionComponent::GetHoveredWidget()
{
	return HoveredWidget;
}

FVector UWidgetInteractionComponent::GetLastImpactPoint()
{
	return LastImpactPoint;
}

#undef LOCTEXT_NAMESPACE
