// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGSequencePlayer.h"
#include "SceneViewport.h"
#include "WidgetAnimation.h"

#include "WidgetBlueprintLibrary.h"
#include "WidgetLayoutLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UUserWidget

UUserWidget::UUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ViewportAnchors = FAnchors(0, 0, 1, 1);
	Visiblity = ESlateVisibility::SelfHitTestInvisible;

	bInitialized = false;
	CachedWorld = NULL;

	bSupportsKeyboardFocus = true;

#if WITH_EDITORONLY_DATA
	bUseDesignTimeSize = false;
	DesignTimeSize = FVector2D(100, 100);
	PaletteCategory = LOCTEXT("UserCreated", "User Created");
#endif
}

void UUserWidget::Initialize()
{
	// If it's not initialized initialize it, as long as it's not the CDO, we never initialize the CDO.
	if ( !bInitialized && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		bInitialized = true;

		// Only do this if this widget is of a blueprint class
		UWidgetBlueprintGeneratedClass* BGClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
		if ( BGClass != NULL )
		{
			BGClass->InitializeWidget(this);
		}

		for ( UWidget* Widget : Components )
		{
			if ( UNamedSlot* NamedWidet = Cast<UNamedSlot>(Widget) )
			{
				for ( FNamedSlotBinding& Binding : NamedSlotBindings )
				{
					if ( Binding.Content && Binding.Name == NamedWidet->GetFName() )
					{
						NamedWidet->ClearChildren();
						NamedWidet->AddChild(Binding.Content);
						continue;
					}
				}
			}
		}
	}
}

void UUserWidget::PostEditImport()
{
	Initialize();
}

void UUserWidget::PostDuplicate(bool bDuplicateForPIE)
{
	Initialize();
}

void UUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	UWidget* RootWidget = GetRootWidgetComponent();
	if ( RootWidget )
	{
		RootWidget->ReleaseSlateResources(bReleaseChildren);
	}
}

void UUserWidget::PostInitProperties()
{
	Super::PostInitProperties();

	Components.Reset();
	//TODO UMG For non-BP versions how do we generate the Components list?
}

UWorld* UUserWidget::GetWorld() const
{
	// Use the Player Context's world.
	if ( PlayerContext.IsValid() )
	{
		if ( UWorld* World = PlayerContext.GetWorld() )
		{
			return World;
		}
	}

	// If the current player context doesn't have a world or isn't valid, return the game instance's world.
	if (  UGameInstance* GameInstance = Cast<UGameInstance>(GetOuter()) )
	{
		if ( UWorld* World = GameInstance->GetWorld() )
		{
			return World;
		}
	}

	return nullptr;
}

void UUserWidget::SetIsDesignTime(bool bInDesignTime)
{
	Super::SetIsDesignTime(bInDesignTime);

	for ( UWidget* Widget : Components )
	{
		Widget->SetIsDesignTime(bInDesignTime);
	}
}

void UUserWidget::Construct_Implementation()
{

}

void UUserWidget::Tick_Implementation(FGeometry MyGeometry, float InDeltaTime)
{

}

void UUserWidget::OnPaint_Implementation(FPaintContext& Context) const
{

}

FEventReply UUserWidget::OnKeyboardFocusReceived_Implementation(FGeometry MyGeometry, FKeyboardFocusEvent InKeyboardFocusEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

void UUserWidget::OnKeyboardFocusLost_Implementation(FKeyboardFocusEvent InKeyboardFocusEvent)
{

}

FEventReply UUserWidget::OnKeyChar_Implementation(FGeometry MyGeometry, FCharacterEvent InCharacterEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnPreviewKeyDown_Implementation(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnKeyDown_Implementation(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnKeyUp_Implementation(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnMouseButtonDown_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnPreviewMouseButtonDown_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnMouseButtonUp_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnMouseMove_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

void UUserWidget::OnMouseEnter_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{

}

void UUserWidget::OnMouseLeave_Implementation(const FPointerEvent& MouseEvent)
{

}

FEventReply UUserWidget::OnMouseWheel_Implementation(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnMouseButtonDoubleClick_Implementation(FGeometry InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

void UUserWidget::OnDragDetected_Implementation(FGeometry MyGeometry, const FPointerEvent& PointerEvent, UDragDropOperation*& Operation)
{

}

void UUserWidget::OnDragCancelled_Implementation(const FPointerEvent& PointerEvent, UDragDropOperation* Operation)
{

}

void UUserWidget::OnDragEnter_Implementation(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation)
{

}

void UUserWidget::OnDragLeave_Implementation(FPointerEvent PointerEvent, UDragDropOperation* Operation)
{

}

bool UUserWidget::OnDragOver_Implementation(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation)
{
	return false;
}

bool UUserWidget::OnDrop_Implementation(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation* Operation)
{
	return false;
}

FEventReply UUserWidget::OnControllerButtonPressed_Implementation(FGeometry MyGeometry, FControllerEvent ControllerEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnControllerButtonReleased_Implementation(FGeometry MyGeometry, FControllerEvent ControllerEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnControllerAnalogValueChanged_Implementation(FGeometry MyGeometry, FControllerEvent ControllerEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnTouchGesture_Implementation(FGeometry MyGeometry, const FPointerEvent& GestureEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnTouchStarted_Implementation(FGeometry MyGeometry, const FPointerEvent& InTouchEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnTouchMoved_Implementation(FGeometry MyGeometry, const FPointerEvent& InTouchEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnTouchEnded_Implementation(FGeometry MyGeometry, const FPointerEvent& InTouchEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UUserWidget::OnMotionDetected_Implementation(FGeometry MyGeometry, FMotionEvent InMotionEvent)
{
	return UWidgetBlueprintLibrary::Unhandled();
}

void UUserWidget::PlayAnimation(const UWidgetAnimation* InAnimation)
{
	if( InAnimation )
	{
		// @todo UMG sequencer - Restart animations which have had Play called on them?
		UUMGSequencePlayer** FoundPlayer = ActiveSequencePlayers.FindByPredicate( [&](const UUMGSequencePlayer* Player) { return Player->GetAnimation() == InAnimation; } );

		if( !FoundPlayer )
		{
			UUMGSequencePlayer* NewPlayer = ConstructObject<UUMGSequencePlayer>( UUMGSequencePlayer::StaticClass(), this );
			ActiveSequencePlayers.Add( NewPlayer );

			NewPlayer->OnSequenceFinishedPlaying().AddUObject( this, &UUserWidget::OnAnimationFinishedPlaying );

			NewPlayer->InitSequencePlayer( *InAnimation, *this );

			NewPlayer->Play();
		}
		else
		{
			(*FoundPlayer)->Play();
		}
	}
}

void UUserWidget::StopAnimation(const UWidgetAnimation* InAnimation)
{
	if(InAnimation)
	{
		// @todo UMG sequencer - Restart animations which have had Play called on them?
		UUMGSequencePlayer** FoundPlayer = ActiveSequencePlayers.FindByPredicate([&](const UUMGSequencePlayer* Player) { return Player->GetAnimation() == InAnimation; } );

		if(FoundPlayer)
		{
			(*FoundPlayer)->Stop();
		}
	}
}

void UUserWidget::OnAnimationFinishedPlaying( UUMGSequencePlayer& Player )
{
	StoppedSequencePlayers.Add( &Player );
}

void UUserWidget::PlaySound(USoundBase* SoundToPlay)
{
	if (SoundToPlay)
	{
		FSlateSound NewSound;
		NewSound.SetResourceObject(SoundToPlay);
		FSlateApplication::Get().PlaySound(NewSound);
	}
}

UWidget* UUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetTree->FindWidget(InWidget);
}

TSharedRef<SWidget> UUserWidget::RebuildWidget()
{
	// In the event this widget is replaced in memory by the blueprint compiler update
	// the widget won't be properly initialized, so we ensure it's initialized and initialize
	// it if it hasn't been.
	if ( !bInitialized )
	{
		Initialize();
	}

	TSharedPtr<SWidget> UserRootWidget;

	//setup the player context on sub user widgets, if we have a valid context
	if (PlayerContext.IsValid())
	{
		for (UWidget* Widget : Components)
		{
			UUserWidget* UserWidget = Cast<UUserWidget>(Widget);
			if (UserWidget)
			{
				UserWidget->SetPlayerContext(PlayerContext);
			}
		}
	}

	// Add the first component to the root of the widget surface.
	if ( Components.Num() > 0 && Components[0] != NULL )
	{
		UserRootWidget = Components[0]->TakeWidget();
	}
	else
	{
		UserRootWidget = SNew(SSpacer);
	}

	if ( !IsDesignTime() )
	{
		// Notify the widget that it has been constructed.
		Construct();
	}

	return UserRootWidget.ToSharedRef();
}

TSharedPtr<SWidget> UUserWidget::GetWidgetFromName(const FName& Name) const
{
	UWidget* WidgetObject = WidgetTree->FindWidget(Name);
	if ( WidgetObject )
	{
		return WidgetObject->GetCachedWidget();
	}

	return TSharedPtr<SWidget>();
}

UWidget* UUserWidget::GetHandleFromName(const FString& Name) const
{
	for ( UWidget* Widget : Components )
	{
		if ( Widget->GetName().Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Widget;
		}
	}

	return nullptr;
}

void UUserWidget::GetSlotNames(TArray<FName>& SlotNames) const
{
	//TODO UMG This should be static data computed and stored on the class at compile time.

	// Add all the names of the named slot widgets to the slot names structure.
	for ( UWidget* Widget : Components )
	{
		if ( Widget && Widget->IsA<UNamedSlot>() )
		{
			SlotNames.Add( Widget->GetFName() );
		}
	}

	// Also add any existing bindings uniquely, templates don't have any components
	// yet because they're never initialized because of their CDO status.
	for ( const FNamedSlotBinding& Binding : NamedSlotBindings )
	{
		SlotNames.AddUnique( Binding.Name );
	}
}

UWidget* UUserWidget::GetContentForSlot(FName SlotName) const
{
	for ( const FNamedSlotBinding& Binding : NamedSlotBindings )
	{
		if ( Binding.Name == SlotName )
		{
			return Binding.Content;
		}
	}

	return nullptr;
}

void UUserWidget::SetContentForSlot(FName SlotName, UWidget* Content)
{
	// Find the binding in the existing set and replace the content for that binding.
	for ( int32 BindingIndex = 0; BindingIndex < NamedSlotBindings.Num(); BindingIndex++)
	{
		FNamedSlotBinding& Binding = NamedSlotBindings[BindingIndex];

		if ( Binding.Name == SlotName )
		{
			if ( Content )
			{
				Binding.Content = Content;
			}
			else
			{
				NamedSlotBindings.RemoveAt(BindingIndex);
			}

			return;
		}
	}

	if ( Content )
	{
		// Add the new binding to the list of bindings.
		FNamedSlotBinding NewBinding;
		NewBinding.Name = SlotName;
		NewBinding.Content = Content;

		NamedSlotBindings.Add(NewBinding);
	}
}

void UUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime )
{
	// Update active movie scenes
	for( UUMGSequencePlayer* Player : ActiveSequencePlayers )
	{
		Player->Tick( InDeltaTime );
	}

	// The process of ticking the players above can stop them so we remove them after all players have ticked
	for( UUMGSequencePlayer* StoppedPlayer : StoppedSequencePlayers )
	{
		ActiveSequencePlayers.Remove( StoppedPlayer );	
	}

	StoppedSequencePlayers.Empty();

	if ( !bDesignTime )
	{
		UWorld* World = GetWorld();
		if ( World )
		{
			// Update any latent actions we have for this actor
			FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
			LatentActionManager.ProcessLatentActions(this, InDeltaTime);
		}
	}

	Tick( MyGeometry, InDeltaTime );
}

TSharedRef<SWidget> UUserWidget::MakeViewportWidget(TSharedPtr<SWidget>& UserSlateWidget)
{
	UserSlateWidget = TakeWidget();

	return SNew(SConstraintCanvas)

		+ SConstraintCanvas::Slot()
		.Offset(BIND_UOBJECT_ATTRIBUTE(FMargin, GetFullScreenOffset))
		.Anchors(BIND_UOBJECT_ATTRIBUTE(FAnchors, GetViewportAnchors))
		.Alignment(BIND_UOBJECT_ATTRIBUTE(FVector2D, GetFullScreenAlignment))
		[
			UserSlateWidget.ToSharedRef()
		];
}

UWidget* UUserWidget::GetRootWidgetComponent()
{
	if ( Components.Num() > 0 )
	{
		return Components[0];
	}

	return nullptr;
}

void UUserWidget::AddToViewport()
{
	if ( !FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> OutUserSlateWidget;
		TSharedRef<SWidget> RootWidget = MakeViewportWidget(OutUserSlateWidget);

		FullScreenWidget = RootWidget;

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
			{
				ViewportClient->AddViewportWidgetContent(RootWidget);
			}
		}
	}
}

void UUserWidget::RemoveFromViewport()
{
	RemoveFromParent();
}

void UUserWidget::RemoveFromParent()
{
	if ( FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> WidgetHost = FullScreenWidget.Pin();

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
			{
				ViewportClient->RemoveViewportWidgetContent(WidgetHost.ToSharedRef());
			}
		}
	}
	else
	{
		Super::RemoveFromParent();
	}
}

bool UUserWidget::GetIsVisible() const
{
	return FullScreenWidget.IsValid();
}

bool UUserWidget::IsInViewport() const
{
	return FullScreenWidget.IsValid();
}

TEnumAsByte<ESlateVisibility::Type> UUserWidget::GetVisiblity() const
{
	if ( FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> RootWidget = FullScreenWidget.Pin();

		return UWidget::ConvertRuntimeToSerializedVisiblity(RootWidget->GetVisibility());
	}

	return ESlateVisibility::Collapsed;
}

void UUserWidget::SetPlayerContext(FLocalPlayerContext InPlayerContext)
{
	PlayerContext = InPlayerContext;
}

const FLocalPlayerContext& UUserWidget::GetPlayerContext() const
{
	return PlayerContext;
}

ULocalPlayer* UUserWidget::GetOwningLocalPlayer() const
{
	APlayerController* PC = PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : nullptr;
	return PC ? Cast<ULocalPlayer>(PC->Player) : nullptr;
}

APlayerController* UUserWidget::GetOwningPlayer() const
{
	return PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : nullptr;
}

class APawn* UUserWidget::GetOwningPlayerPawn() const
{
	if ( APlayerController* PC = GetOwningPlayer() )
	{
		return PC->GetPawn();
	}

	return nullptr;
}

void UUserWidget::SetPositionInViewport(FVector2D Position)
{
	float Scale = UWidgetLayoutLibrary::GetViewportScale(this);

	ViewportOffsets.Left = Position.X * ( 1.0f / Scale );
	ViewportOffsets.Top = Position.Y * ( 1.0f / Scale );

	ViewportAnchors = FAnchors(0, 0);
}

void UUserWidget::SetDesiredSizeInViewport(FVector2D DesiredSize)
{
	ViewportOffsets.Right = DesiredSize.X;
	ViewportOffsets.Bottom = DesiredSize.Y;

	ViewportAnchors = FAnchors(0, 0);
}

void UUserWidget::SetAnchorsInViewport(FAnchors Anchors)
{
	ViewportAnchors = Anchors;
}

void UUserWidget::SetAlignmentInViewport(FVector2D Alignment)
{
	ViewportAlignment = Alignment;
}

FMargin UUserWidget::GetFullScreenOffset() const
{
	// If the size is zero, and we're not stretched, then use the desired size.
	FVector2D FinalSize = FVector2D(ViewportOffsets.Right, ViewportOffsets.Bottom);
	if ( FinalSize.IsZero() && !ViewportAnchors.IsStretchedVertical() && !ViewportAnchors.IsStretchedHorizontal() )
	{
		TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
		if ( CachedWidget.IsValid() )
		{
			FinalSize = CachedWidget->GetDesiredSize();
		}
	}

	return FMargin(ViewportOffsets.Left, ViewportOffsets.Top, FinalSize.X, FinalSize.Y);
}

FAnchors UUserWidget::GetViewportAnchors() const
{
	return ViewportAnchors;
}

FVector2D UUserWidget::GetFullScreenAlignment() const
{
	return ViewportAlignment;
}

#if WITH_EDITOR

const FSlateBrush* UUserWidget::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.UserWidget");
}

const FText UUserWidget::GetPaletteCategory()
{
	return PaletteCategory;
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
