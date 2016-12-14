// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Components/WidgetInteractionComponent.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/ArrowComponent.h"
#include "Framework/Application/SlateApplication.h"

#include "Components/WidgetComponent.h"


#define LOCTEXT_NAMESPACE "WidgetInteraction"

UWidgetInteractionComponent::UWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, VirtualUserIndex(0)
	, PointerIndex(0)
	, InteractionDistance(500)
	, InteractionSource(EWidgetInteractionSource::World)
	, bEnableHitTesting(true)
	, bShowDebug(false)
	, DebugColor(FLinearColor::Red)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	TraceChannel = ECC_Visibility;
	bAutoActivate = true;

#if WITH_EDITORONLY_DATA
	ArrowComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("ArrowComponent0"));

	if ( ArrowComponent && !IsTemplate() )
	{
		ArrowComponent->ArrowColor = DebugColor.ToFColor(true);
		ArrowComponent->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	}
#endif
}

void UWidgetInteractionComponent::OnComponentCreated()
{
#if WITH_EDITORONLY_DATA
	if ( ArrowComponent )
	{
		ArrowComponent->ArrowColor = DebugColor.ToFColor(true);
		ArrowComponent->SetVisibility(bEnableHitTesting);
	}
#endif
}

void UWidgetInteractionComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	if ( FSlateApplication::IsInitialized() )
	{
		if ( !VirtualUser.IsValid() )
		{
			VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualUserIndex);
		}
	}
}

void UWidgetInteractionComponent::Deactivate()
{
	Super::Deactivate();

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

UWidgetInteractionComponent::FWidgetTraceResult UWidgetInteractionComponent::PerformTrace() const
{
	FWidgetTraceResult TraceResult;

	switch( InteractionSource )
	{
		case EWidgetInteractionSource::World:
		{
			const FVector WorldLocation = GetComponentLocation();
			const FTransform WorldTransform = GetComponentTransform();
			const FVector Direction = WorldTransform.GetUnitAxis(EAxis::X);

			TArray<UPrimitiveComponent*> PrimitiveChildren;
			GetRelatedComponentsToIgnoreInAutomaticHitTesting(PrimitiveChildren);

			FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
			Params.AddIgnoredComponents(PrimitiveChildren);

			TraceResult.bWasHit = GetWorld()->LineTraceSingleByChannel(TraceResult.HitResult, WorldLocation, WorldLocation + ( Direction * InteractionDistance ), TraceChannel, Params);
			break;
		}
		case EWidgetInteractionSource::Mouse:
		case EWidgetInteractionSource::CenterScreen:
		{
			TArray<UPrimitiveComponent*> PrimitiveChildren;
			GetRelatedComponentsToIgnoreInAutomaticHitTesting(PrimitiveChildren);

			FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
			Params.AddIgnoredComponents(PrimitiveChildren);

			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

			ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
			TraceResult.bWasHit = false;
			if ( LocalPlayer && LocalPlayer->ViewportClient )
			{
				if ( InteractionSource == EWidgetInteractionSource::Mouse )
				{
					FVector2D MousePosition;
					if ( LocalPlayer->ViewportClient->GetMousePosition(MousePosition) )
					{
						TraceResult.bWasHit = PlayerController->GetHitResultAtScreenPosition(MousePosition, TraceChannel, Params, TraceResult.HitResult);
					}
				}
				else if ( InteractionSource == EWidgetInteractionSource::CenterScreen )
				{
					FVector2D ViewportSize;
					LocalPlayer->ViewportClient->GetViewportSize(ViewportSize);

					TraceResult.bWasHit = PlayerController->GetHitResultAtScreenPosition(ViewportSize * 0.5f, TraceChannel, Params, TraceResult.HitResult);
				}

				// Don't allow infinite distance hit testing.
				if ( TraceResult.bWasHit )
				{
					if ( TraceResult.HitResult.Distance > InteractionDistance )
					{
						TraceResult = FWidgetTraceResult();
					}
				}
			}
			break;
		}
		case EWidgetInteractionSource::Custom:
		{
			TraceResult.HitResult = CustomHitResult;
			TraceResult.bWasHit = CustomHitResult.bBlockingHit;
			break;
		}
	}

	// Resolve trace to location on widget.
	if (TraceResult.bWasHit)
	{
		TraceResult.HitWidgetComponent = Cast<UWidgetComponent>(TraceResult.HitResult.GetComponent());
		if (TraceResult.HitWidgetComponent)
		{
			// @todo WASTED WORK: GetLocalHitLocation() gets called in GetHitWidgetPath();

			if (TraceResult.HitWidgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder)
			{
				const FTransform WorldTransform = GetComponentTransform();
				const FVector Direction = WorldTransform.GetUnitAxis(EAxis::X);
				
				TTuple<FVector, FVector2D> CylinderHitLocation = TraceResult.HitWidgetComponent->GetCylinderHitLocation(TraceResult.HitResult.ImpactPoint, Direction);
				TraceResult.HitResult.ImpactPoint = CylinderHitLocation.Get<0>();
				TraceResult.LocalHitLocation = CylinderHitLocation.Get<1>();
			}
			else
			{
				ensure(TraceResult.HitWidgetComponent->GetGeometryMode() == EWidgetGeometryMode::Plane);
				TraceResult.HitWidgetComponent->GetLocalHitLocation(LastHitResult.ImpactPoint, TraceResult.LocalHitLocation);
			}

			TraceResult.HitWidgetPath = FWidgetPath(TraceResult.HitWidgetComponent->GetHitWidgetPath(TraceResult.LocalHitLocation, /*bIgnoreEnabledStatus*/ false));
		}
	}

	return TraceResult;
}

void UWidgetInteractionComponent::GetRelatedComponentsToIgnoreInAutomaticHitTesting(TArray<UPrimitiveComponent*>& IgnorePrimitives) const
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

bool UWidgetInteractionComponent::CanInteractWithComponent(UWidgetComponent* Component) const
{
	bool bCanInteract = false;

	if (Component)
	{
		bCanInteract = !GetWorld()->IsPaused() || Component->PrimaryComponentTick.bTickEvenWhenPaused;
	}

	return bCanInteract;
}

void UWidgetInteractionComponent::SimulatePointerMovement()
{
	bIsHoveredWidgetInteractable = false;
	bIsHoveredWidgetFocusable = false;
	bIsHoveredWidgetHitTestVisible = false;

	if ( !bEnableHitTesting )
	{
		return;
	}

	if ( !CanSendInput() )
	{
		return;
	}
	
	FWidgetPath WidgetPathUnderFinger;
	UWidgetComponent* OldHoveredWidget = HoveredWidgetComponent;

	FWidgetTraceResult TraceResult = PerformTrace();
	LastHitResult = TraceResult.HitResult;
	HoveredWidgetComponent = TraceResult.HitWidgetComponent;
	LocalHitLocation = TraceResult.bWasHit
		? TraceResult.LocalHitLocation
		: LastLocalHitLocation;
	WidgetPathUnderFinger = TraceResult.HitWidgetPath;

	if ( bShowDebug )
	{
		if ( HoveredWidgetComponent )
		{
			UKismetSystemLibrary::DrawDebugSphere(this, LastHitResult.ImpactPoint, 2.5f, 12, DebugColor, 0, 2);
		}

		if ( InteractionSource == EWidgetInteractionSource::World || InteractionSource == EWidgetInteractionSource::Custom )
		{
			if ( HoveredWidgetComponent )
			{
				UKismetSystemLibrary::DrawDebugLine(this, LastHitResult.TraceStart, LastHitResult.ImpactPoint, DebugColor, 0, 1);
			}
			else
			{
				UKismetSystemLibrary::DrawDebugLine(this, LastHitResult.TraceStart, LastHitResult.TraceEnd, DebugColor, 0, 1);
			}
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
		check(HoveredWidgetComponent);
		LastWigetPath = WidgetPathUnderFinger;
		
		FSlateApplication::Get().RoutePointerMoveEvent(WidgetPathUnderFinger, PointerEvent, false);
	}
	else
	{
		FWidgetPath EmptyWidgetPath;
		FSlateApplication::Get().RoutePointerMoveEvent(EmptyWidgetPath, PointerEvent, false);

		LastWigetPath = FWeakWidgetPath();
	}

	if ( HoveredWidgetComponent )
	{
		HoveredWidgetComponent->RequestRedraw();
	}

	LastLocalHitLocation = LocalHitLocation;

	if ( WidgetPathUnderFinger.IsValid() )
	{
		const FArrangedChildren::FArrangedWidgetArray& AllArrangedWidgets = WidgetPathUnderFinger.Widgets.GetInternalArray();
		for ( const FArrangedWidget& ArrangedWidget : AllArrangedWidgets )
		{
			const TSharedRef<SWidget>& Widget = ArrangedWidget.Widget;
			if ( Widget->IsInteractable() )
			{
				bIsHoveredWidgetInteractable = true;
			}

			if ( Widget->SupportsKeyboardFocus() )
			{
				bIsHoveredWidgetFocusable = true;
			}

			if ( Widget->GetVisibility().IsHitTestVisible() )
			{
				bIsHoveredWidgetHitTestVisible = true;
			}
		}
	}

	if ( HoveredWidgetComponent != OldHoveredWidget )
	{
		if ( OldHoveredWidget )
		{
			OldHoveredWidget->RequestRedraw();
		}

		OnHoveredWidgetChanged.Broadcast(HoveredWidgetComponent, OldHoveredWidget);
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

bool UWidgetInteractionComponent::SendKeyChar(FString Characters, bool bRepeat)
{
	if ( !CanSendInput() )
	{
		return false;
	}

	bool bProcessResult = false;

	for ( int32 CharIndex = 0; CharIndex < Characters.Len(); CharIndex++ )
	{
		TCHAR CharKey = Characters[CharIndex];

		FCharacterEvent CharacterEvent(CharKey, ModifierKeys, VirtualUser->GetUserIndex(), bRepeat);
		bProcessResult |= FSlateApplication::Get().ProcessKeyCharEvent(CharacterEvent);
	}

	return bProcessResult;
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
}

UWidgetComponent* UWidgetInteractionComponent::GetHoveredWidgetComponent() const
{
	return HoveredWidgetComponent;
}

bool UWidgetInteractionComponent::IsOverInteractableWidget() const
{
	return bIsHoveredWidgetInteractable;
}

bool UWidgetInteractionComponent::IsOverFocusableWidget() const
{
	return bIsHoveredWidgetFocusable;
}

bool UWidgetInteractionComponent::IsOverHitTestVisibleWidget() const
{
	return bIsHoveredWidgetHitTestVisible;
}

const FWeakWidgetPath& UWidgetInteractionComponent::GetHoveredWidgetPath() const
{
	return LastWigetPath;
}

const FHitResult& UWidgetInteractionComponent::GetLastHitResult() const
{
	return LastHitResult;
}

FVector2D UWidgetInteractionComponent::Get2DHitLocation() const
{
	return LocalHitLocation;
}

#undef LOCTEXT_NAMESPACE
