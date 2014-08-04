// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGSequencePlayer.h"
#include "SceneViewport.h"

TSharedRef<FUMGDragDropOp> FUMGDragDropOp::New()
{
	TSharedRef<FUMGDragDropOp> Operation = MakeShareable(new FUMGDragDropOp);
	Operation->Construct();

	return Operation;
}

void FUMGDragDropOp::OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
}

void FUMGDragDropOp::OnDragged(const class FDragDropEvent& DragDropEvent)
{
	FDragDropOperation::OnDragged(DragDropEvent);
}

TSharedPtr<SWidget> FUMGDragDropOp::GetDefaultDecorator() const
{
	return DecoratorWidget;
}

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

	void Construct(const FArguments& InArgs, bool bInModal)
	{
		bModal = bInModal;

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
		if ( SupportsKeyboardFocus() )
		{
			return FReply::Handled().ReleaseMouseCapture().ReleaseJoystickCapture();// .LockMouseToWidget(SharedThis(this));
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
		return bModal ? FReply::Handled() : SCompoundWidget::OnKeyChar(MyGeometry, InCharacterEvent);
	}
			   
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override
	{
		return bModal ? FReply::Handled() : SCompoundWidget::OnKeyDown(MyGeometry, InKeyboardEvent);
	}
		   
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override
	{
		return bModal ? FReply::Handled() : SCompoundWidget::OnKeyUp(MyGeometry, InKeyboardEvent);
	}
		   
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled().CaptureMouse(AsShared()) : SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
	}
		   
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled().ReleaseMouseCapture() : SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : SCompoundWidget::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
	}

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bModal ? FReply::Handled() : SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);
	}

protected:
	bool bModal;
};


/////////////////////////////////////////////////////
// UUserWidget

UUserWidget::UUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	Visiblity = ESlateVisibility::SelfHitTestInvisible;
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

	// Only do this if this widget is of a blueprint class
	UWidgetBlueprintGeneratedClass* BGClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
	if ( BGClass != NULL )
	{
		BGClass->InitializeWidget(this);
	}

	//TODO UMG For non-BP versions how do we generate the Components list?
}

UWorld* UUserWidget::GetWorld() const
{
	if ( PlayerContext.IsValid() )
	{
		return PlayerContext.GetWorld();
	}

	return NULL;
}

void UUserWidget::PlayAnimation(FName AnimationName)
{
	UWidgetBlueprintGeneratedClass* BPClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
	if (BPClass)
	{
		const FWidgetAnimation* Animation = BPClass->FindAnimation(AnimationName);
		if( Animation )
		{
			// @todo UMG sequencer - Restart animations which have had Play called on them?
			UUMGSequencePlayer** FoundPlayer = ActiveSequencePlayers.FindByPredicate( [&](const UUMGSequencePlayer* Player) { return Player->GetMovieScene() == Animation->MovieScene; } );

			if( !FoundPlayer )
			{
				UUMGSequencePlayer* NewPlayer = ConstructObject<UUMGSequencePlayer>( UUMGSequencePlayer::StaticClass(), this );
				ActiveSequencePlayers.Add( NewPlayer );

				NewPlayer->OnSequenceFinishedPlaying().AddUObject( this, &UUserWidget::OnAnimationFinishedPlaying );

				NewPlayer->InitSequencePlayer( *Animation, *this );

				NewPlayer->Play();
			}
			else
			{
				(*FoundPlayer)->Play();
			}
		}
	}
}

void UUserWidget::StopAnimation(FName AnimationName)
{
	UWidgetBlueprintGeneratedClass* BPClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
	if(BPClass)
	{
		const FWidgetAnimation* Animation = BPClass->FindAnimation(AnimationName);
		if(Animation)
		{
			// @todo UMG sequencer - Restart animations which have had Play called on them?
			UUMGSequencePlayer** FoundPlayer = ActiveSequencePlayers.FindByPredicate([&](const UUMGSequencePlayer* Player) { return Player->GetMovieScene() == Animation->MovieScene; });
			if(FoundPlayer)
			{
				(*FoundPlayer)->Stop();
			}
		}
	}
}

void UUserWidget::OnAnimationFinishedPlaying( UUMGSequencePlayer& Player )
{
	ActiveSequencePlayers.Remove( &Player );
}

UWidget* UUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetTree->FindWidget(InWidget);
}

TSharedRef<SWidget> UUserWidget::RebuildWidget()
{
	TSharedPtr<SWidget> UserRootWidget;

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
	for ( UWidget* Widget : Components )
	{
		if ( Widget->GetName().Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Widget;
		}
	}

	return NULL;
}

void UUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime )
{
	// Update active movie scenes
	for( UUMGSequencePlayer* Player : ActiveSequencePlayers )
	{
		Player->Tick( InDeltaTime );
	}

	if ( !bDesignTime )
	{
		UWorld* World = GetWorld();
		if ( World )
		{
			// Update any latent actions we have for this actor
			World->GetLatentActionManager().ProcessLatentActions(this, InDeltaTime);
		}
	}

	Tick( MyGeometry, InDeltaTime );
}

TSharedRef<SWidget> UUserWidget::MakeViewportWidget(bool bAbsoluteLayout, bool bModal, bool bShowCursor)
{
	if ( bAbsoluteLayout )
	{
		//BIND_UOBJECT_ATTRIBUTE

		return SNew(SConstraintCanvas)

			+ SConstraintCanvas::Slot()
			.Offset(BIND_UOBJECT_ATTRIBUTE(FMargin, GetFullScreenOffset))
			.Alignment(BIND_UOBJECT_ATTRIBUTE(FVector2D, GetFullScreenAlignment))
			.ZOrder(BIND_UOBJECT_ATTRIBUTE(int32, GetFullScreenZOrder))
			[
				TakeWidget()
			];
	}
	else
	{
		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

		auto& NewSlot = VerticalBox->AddSlot()
			.Padding(Padding)
			.HAlign(HorizontalAlignment)
			.VAlign(VerticalAlignment)
			[
				TakeWidget()
			];

		NewSlot.SizeParam = UWidget::ConvertSerializedSizeParamToRuntime(Size);

		return VerticalBox;
	}
}

UWidget* UUserWidget::GetRootWidgetComponent()
{
	if ( Components.Num() > 0 )
	{
		return Components[0];
	}

	return NULL;
}

void UUserWidget::AddToViewport(bool bAbsoluteLayout, bool bModal, bool bShowCursor)
{
	if ( !FullScreenWidget.IsValid() )
	{
		TSharedRef<SWidget> RootWidget = MakeViewportWidget(bAbsoluteLayout, bModal, bShowCursor);

		TSharedRef<SViewportWidgetHost> WidgetHost = SNew(SViewportWidgetHost, (bool)bModal)
			[
				RootWidget
			];

		FullScreenWidget = WidgetHost;

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			UGameViewportClient* Viewport = World->GetGameViewport();
			Viewport->AddViewportWidgetContent(WidgetHost);

			TWeakPtr<SViewport> GameViewportWidget = Viewport->GetGameViewport()->GetViewportWidget();
			if ( GameViewportWidget.IsValid() )
			{
				GameViewportWidget.Pin()->SetWidgetToFocusOnActivate(RootWidget);
				FSlateApplication::Get().SetKeyboardFocus(RootWidget);
			}
		}
	}
}

void UUserWidget::RemoveFromViewport()
{
	if ( FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> RootWidget = FullScreenWidget.Pin();

		// If this is a game world add the widget to the current worlds viewport.
		UWorld* World = GetWorld();
		if ( World && World->IsGameWorld() )
		{
			UGameViewportClient* Viewport = World->GetGameViewport();
			Viewport->RemoveViewportWidgetContent(RootWidget.ToSharedRef());

			TWeakPtr<SViewport> GameViewportWidget = Viewport->GetGameViewport()->GetViewportWidget();
			if ( GameViewportWidget.IsValid() )
			{
				//TODO UMG this isn't what should manage focus, a higher level window controller, probably the viewport needs to understand
				// the Widget stack, and the dialog stack.
				GameViewportWidget.Pin()->ClearWidgetToFocusOnActivate();
				FSlateApplication::Get().SetKeyboardFocus(TSharedPtr<SWidget>());
			}
		}
	}
}

bool UUserWidget::GetIsVisible()
{
	return FullScreenWidget.IsValid();
}

TEnumAsByte<ESlateVisibility::Type> UUserWidget::GetVisiblity()
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

ULocalPlayer* UUserWidget::GetLocalPlayer() const
{
	APlayerController* PC = PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : NULL;
	return PC ? Cast<ULocalPlayer>(PC->Player) : NULL;
}

APlayerController* UUserWidget::GetPlayerController() const
{
	return PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : NULL;
}

FMargin UUserWidget::GetFullScreenOffset() const
{
	FVector2D FinalSize = FullScreenSize;
	if ( FinalSize.IsZero() )
	{
		TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
		if ( CachedWidget.IsValid() )
		{
			FinalSize = CachedWidget->GetDesiredSize();
		}
	}

	return FMargin(FullScreenPosition.X, FullScreenPosition.Y, FinalSize.X, FinalSize.Y);
}

FVector2D UUserWidget::GetFullScreenAlignment() const
{
	return FullScreenAlignment;
}

int32 UUserWidget::GetFullScreenZOrder() const
{
	return FullScreenZOrder;
}

#if WITH_EDITOR

const FSlateBrush* UUserWidget::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.UserWidget");
}

#endif