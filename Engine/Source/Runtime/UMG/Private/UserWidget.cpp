// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGSequencePlayer.h"
#include "SceneViewport.h"
#include "WidgetAnimation.h"

#include "WidgetBlueprintLibrary.h"
#include "WidgetLayoutLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/**
 * This class holds onto the widget when it's placed into the viewport.
 */
class SViewportWidgetHost : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SViewportWidgetHost)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}

		SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<SWidget> InUserWidget, bool bInModal)
	{
		bModal = bInModal;
		UserWidget = InUserWidget;

		SetVisibility(bModal ? EVisibility::Visible : EVisibility::SelfHitTestInvisible);

		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual bool SupportsKeyboardFocus() const override
	{
		return bModal ? true : false;
	}

	FReply OnKeyboardFocusReceived(const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent) override
	{
		// if we support focus, release the focus captures and lock the focus to this widget 
		if ( bModal )
		{
			return FReply::Handled();// .ReleaseMouseCapture().ReleaseJoystickCapture();// .LockMouseToWidget(SharedThis(this));
		}
		else
		{
			return FReply::Unhandled();
		}
	}

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override
	{
		// If we support focus, show the default mouse cursor 
		if ( SupportsKeyboardFocus() )
		{
			return FCursorReply::Cursor(EMouseCursor::Default);
		}
		else
		{
			return SCompoundWidget::OnCursorQuery(MyGeometry, CursorEvent);
		}
	}

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}
	
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled().CaptureMouse(AsShared()).CaptureJoystick(AsShared()) : FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled().ReleaseMouseCapture().ReleaseJoystickCapture() : FReply::Unhandled();
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : FReply::Unhandled();
	}

	bool IsModal() const { return bModal; }

protected:
	bool bModal;

	TSharedPtr<SWidget> UserWidget;
};


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

void UUserWidget::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	UWidget* RootWidget = GetRootWidgetComponent();
	if ( RootWidget )
	{
		RootWidget->ReleaseNativeWidget();
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

UWidget* UUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetTree->FindWidget(InWidget);
}

TSharedRef<SWidget> UUserWidget::RebuildWidget()
{
	TSharedPtr<SWidget> UserRootWidget;

	check(bInitialized);

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

TSharedPtr<SWidget> UUserWidget::GetWidgetFromName(const FString& Name) const
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
	//return WidgetTree->FindWidget(Name);

	for ( UWidget* Widget : Components )
	{
		if ( Widget->GetName().Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Widget;
		}
	}

	return nullptr;
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
		.ZOrder(BIND_UOBJECT_ATTRIBUTE(int32, GetFullScreenZOrder))
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

	return NULL;
}

void UUserWidget::AddToViewport(bool bModal)
{
	if ( !FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> OutUserSlateWidget;
		TSharedRef<SWidget> RootWidget = MakeViewportWidget(OutUserSlateWidget);

		TSharedRef<SViewportWidgetHost> WidgetHost = SNew(SViewportWidgetHost, OutUserSlateWidget, bModal)
			[
				RootWidget
			];

		FullScreenWidget = WidgetHost;

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
			{
				ViewportClient->AddViewportWidgetContent(WidgetHost);

				//TODO UMG this isn't what should manage focus, a higher level window controller, probably the viewport needs to understand
				// the Widget stack, and the dialog stack.
				if ( bModal )
				{
					if ( FSceneViewport* SceneViewport = ViewportClient->GetGameViewport() )
					{
						TWeakPtr<SViewport> GameViewportWidget = SceneViewport->GetViewportWidget();

						if ( GameViewportWidget.IsValid() )
						{
							GameViewportWidget.Pin()->SetWidgetToFocusOnActivate(OutUserSlateWidget);

							FSlateApplication::Get().SetFocusToGameViewport();
							FSlateApplication::Get().SetJoystickCaptorToGameViewport();
						}
					}
				}
			}
		}
	}
}

void UUserWidget::RemoveFromViewport()
{
	if ( FullScreenWidget.IsValid() )
	{
		TSharedPtr<SViewportWidgetHost> WidgetHost = FullScreenWidget.Pin();

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			if ( UGameViewportClient* ViewportClient = World->GetGameViewport() )
			{
				ViewportClient->RemoveViewportWidgetContent(WidgetHost.ToSharedRef());

				if ( WidgetHost->IsModal() )
				{
					if ( FSceneViewport* SceneViewport = ViewportClient->GetGameViewport() )
					{
						TWeakPtr<SViewport> GameViewportWidget = SceneViewport->GetViewportWidget();

						if ( GameViewportWidget.IsValid() )
						{
							//TODO UMG this isn't what should manage focus, a higher level window controller, probably the viewport needs to understand
							// the Widget stack, and the dialog stack.
							GameViewportWidget.Pin()->ClearWidgetToFocusOnActivate();
							FSlateApplication::Get().ClearKeyboardFocus(EKeyboardFocusCause::SetDirectly);

							FSlateApplication::Get().SetFocusToGameViewport();
							FSlateApplication::Get().SetJoystickCaptorToGameViewport();
						}
					}
				}
			}
		}
	}
}

bool UUserWidget::GetIsVisible() const
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

void UUserWidget::SetZOrderInViewport(int32 ZOrder)
{
	ViewportZOrder = ZOrder;
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

int32 UUserWidget::GetFullScreenZOrder() const
{
	return ViewportZOrder;
}

#if WITH_EDITOR

const FSlateBrush* UUserWidget::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.UserWidget");
}

const FText UUserWidget::GetPaletteCategory()
{
	return LOCTEXT("UserCreated", "User Created");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
