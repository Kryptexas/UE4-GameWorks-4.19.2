// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SceneViewport.h"

class SViewportWidgetHost : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SViewportWidgetHost)
	{ }
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

	virtual bool OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition) OVERRIDE
	{
		return true;
	}

	virtual bool SupportsKeyboardFocus() const OVERRIDE
	{
		return true;
	}

	FReply OnKeyboardFocusReceived(const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent)
	{
		// if we support focus, release the focus captures and lock the focus to this widget 
		if ( SupportsKeyboardFocus() )
		{
			return FReply::Handled().ReleaseMouseCapture().ReleaseJoystickCapture().LockMouseToWidget(SharedThis(this));
		}
		else
		{
			return FReply::Unhandled();
		}
	}

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
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

	//FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
	//{
	//	return FReply::Handled();
	//}

	//FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
	//{
	//	return FReply::Handled();
	//}

	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if ( bModal )
		{
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if ( bModal )
		{
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if ( bModal )
		{
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

protected:
	bool bModal;
};


/////////////////////////////////////////////////////
// AUserWidget

AUserWidget::AUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RootWidget = SNullWidget::NullWidget;
	Visiblity = ESlateVisibility::Visible;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShowCursorWhenVisible = false;
	bAbsoluteLayout = false;
	bModal = false;
	AbsoluteSize = FVector2D(100, 100);
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
}

void AUserWidget::PostActorCreated()
{
	Super::PostActorCreated();
}

void AUserWidget::Destroyed()
{
	RootWidget.Reset();
	Super::Destroyed();
}

void AUserWidget::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();

	RebuildWrapperWidget();
}

void AUserWidget::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	//UWorld* World = GetWorld();

	//if ( World && World->IsGameWorld() )
	//{
	//	if ( bShowCursorWhenVisible && GetIsVisible() )
	//	{
	//		World->GetFirstLocalPlayerFromController()->PlayerController->bShowMouseCursor = true;

	//		UGameViewportClient* Viewport = World->GetGameViewport();
	//	}
	//}
}

USlateWrapperComponent* AUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetToComponent.FindRef(InWidget);
}

void AUserWidget::RebuildWrapperWidget()
{
	WidgetToComponent.Reset();
	RootWidget = SNullWidget::NullWidget;
	
	TArray<USlateWrapperComponent*> SlateWrapperComponents;
	GetComponents(SlateWrapperComponents);

	// Add the first component to the root of the widget surface.
	if ( SlateWrapperComponents.Num() > 0 )
	{
		if ( bAbsoluteLayout )
		{
			RootWidget =
				SNew(SCanvas)

				+ SCanvas::Slot()
				.Position(AbsolutePosition)
				.Size(AbsoluteSize)
				.VAlign(VerticalAlignment)
				.HAlign(HorizontalAlignment)
				[
					SlateWrapperComponents[0]->GetWidget()
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
					SlateWrapperComponents[0]->GetWidget()
				];

			NewSlot.SizeParam = USlateWrapperComponent::ConvertSerializedSizeParamToRuntime(Size);

			RootWidget = VerticalBox;
		}
	}

	// Place all of our top-level children Slate wrapped components into the overlay
	for (int32 ComponentIndex = 0; ComponentIndex < SlateWrapperComponents.Num(); ++ComponentIndex)
	{
		USlateWrapperComponent* Handle = SlateWrapperComponents[ComponentIndex];
		TSharedRef<SWidget> Widget = Handle->GetWidget();

		WidgetToComponent.Add(Widget, Handle);
	}
	
	// If this is a game world add the widget to the current worlds viewport.
	UWorld* World = GetWorld();
	if ( World && World->IsGameWorld() )
	{
		TSharedRef<SViewportWidgetHost> WidgetHost = SNew(SViewportWidgetHost, (bool)bModal)
		[
			RootWidget.ToSharedRef()
		];

		RootWidget = WidgetHost;

		UGameViewportClient* Viewport = World->GetGameViewport();
		Viewport->AddViewportWidgetContent(RootWidget.ToSharedRef());

		if ( Visiblity == ESlateVisibility::Visible )
		{
			Show();
		}
		else
		{
			Hide();
		}
	}
}

TSharedPtr<SWidget> AUserWidget::GetWidgetFromName(const FString& Name) const
{
	for ( auto& Entry : WidgetToComponent )
	{
		if ( Entry.Value->GetName().Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Entry.Key.Pin();
		}
	}

	return TSharedPtr<SWidget>();
}

USlateWrapperComponent* AUserWidget::GetHandleFromName(const FString& Name) const
{
	for ( auto& Entry : WidgetToComponent )
	{
		if ( Entry.Value->GetName().Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Entry.Value;
		}
	}

	return NULL;
}

TSharedRef<SWidget> AUserWidget::GetRootWidget()
{
	return RootWidget.ToSharedRef();
}

void AUserWidget::Show()
{
	RootWidget->SetVisibility(EVisibility::Visible);
	OnVisibilityChanged.Broadcast(ESlateVisibility::Visible);

	// If this is a game world add the widget to the current worlds viewport.
	UWorld* World = GetWorld();
	if ( World && World->IsGameWorld() )
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
		TWeakPtr<SViewport> GameViewportWidget = Viewport->GetGameViewport()->GetViewportWidget();
		if ( GameViewportWidget.IsValid() )
		{
			GameViewportWidget.Pin()->SetWidgetToFocusOnActivate(RootWidget);
			FSlateApplication::Get().SetKeyboardFocus(RootWidget);
		}
	}
}

void AUserWidget::Hide()
{
	RootWidget->SetVisibility(EVisibility::Hidden);
	OnVisibilityChanged.Broadcast(ESlateVisibility::Hidden);

	// If this is a game world add the widget to the current worlds viewport.
	UWorld* World = GetWorld();
	if ( World && World->IsGameWorld() )
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
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

bool AUserWidget::GetIsVisible()
{
	return RootWidget->GetVisibility().IsVisible();
}

TEnumAsByte<ESlateVisibility::Type> AUserWidget::GetVisiblity()
{
	return USlateWrapperComponent::ConvertRuntimeToSerializedVisiblity(RootWidget->GetVisibility());
}
