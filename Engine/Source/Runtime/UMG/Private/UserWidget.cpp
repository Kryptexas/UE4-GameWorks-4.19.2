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
// UUserWidget

UUserWidget::UUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Visiblity = ESlateVisibility::Visible;
	/*PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;*/
	bShowCursorWhenVisible = false;
	bAbsoluteLayout = false;
	bModal = false;
	AbsoluteSize = FVector2D(100, 100);
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
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

	RebuildWrapperWidget();
}

UWorld* UUserWidget::GetWorld() const
{
	UObject* Outer = GetOuter();
	if ( Outer == NULL )
	{
		return NULL;
	}

	// TODO UMG Global UI elements should go where?  who will be their outer.  Currently CreateWidget node makes the level their owner.
	if ( ULevel* Level = Cast<ULevel>(Outer) )
	{
		return Level->OwningWorld;
	}

	return NULL;
}

USlateWrapperComponent* UUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetToComponent.FindRef(InWidget);
}

void UUserWidget::RebuildWrapperWidget()
{
	WidgetToComponent.Reset();
	RootWidget = SNew(SSpacer);
	
	// Add the first component to the root of the widget surface.
	if ( Components.Num() > 0 )
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
					Components[0]->GetWidget()
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
					Components[0]->GetWidget()
				];

			NewSlot.SizeParam = USlateWrapperComponent::ConvertSerializedSizeParamToRuntime(Size);

			RootWidget = VerticalBox;
		}
	}

	// Place all of our top-level children Slate wrapped components into the overlay
	for ( int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ++ComponentIndex )
	{
		USlateWrapperComponent* Handle = Components[ComponentIndex];
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

TSharedPtr<SWidget> UUserWidget::GetWidgetFromName(const FString& Name) const
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

USlateWrapperComponent* UUserWidget::GetHandleFromName(const FString& Name) const
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

TSharedRef<SWidget> UUserWidget::GetRootWidget()
{
	return RootWidget.ToSharedRef();
}

USlateWrapperComponent* UUserWidget::GetRootWidgetComponent()
{
	if ( Components.Num() > 0 )
	{
		return Components[0];
	}

	return NULL;
}

void UUserWidget::Show()
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

void UUserWidget::Hide()
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

bool UUserWidget::GetIsVisible()
{
	return RootWidget->GetVisibility().IsVisible();
}

TEnumAsByte<ESlateVisibility::Type> UUserWidget::GetVisiblity()
{
	return USlateWrapperComponent::ConvertRuntimeToSerializedVisiblity(RootWidget->GetVisibility());
}
