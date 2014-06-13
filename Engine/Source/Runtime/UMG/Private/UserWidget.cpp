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

	virtual bool OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition) override
	{
		return true;
	}

	virtual bool SupportsKeyboardFocus() const override
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

	RebuildWidget();
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

UWidget* UUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetToComponent.FindRef(InWidget);
}

TSharedRef<SWidget> UUserWidget::RebuildWidget()
{
	WidgetToComponent.Reset();
	
	// Add the first component to the root of the widget surface.
	if ( Components.Num() > 0 )
	{
		UserRootWidget = Components[0]->GetWidget();
	}
	else
	{
		UserRootWidget = SNew(SSpacer);
	}

	// Place all of our top-level children Slate wrapped components into the overlay
	for ( int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ++ComponentIndex )
	{
		UWidget* Handle = Components[ComponentIndex];
		TSharedRef<SWidget> Widget = Handle->GetWidget();

		WidgetToComponent.Add(Widget, Handle);
	}
	
	// If this is a game world add the widget to the current worlds viewport.
	UWorld* World = GetWorld();
	if ( World && World->IsGameWorld() )
	{
		if ( Visiblity == ESlateVisibility::Visible )
		{
			Show();
		}
		else
		{
			Hide();
		}
	}

	return MakeWidget();
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

UWidget* UUserWidget::GetHandleFromName(const FString& Name) const
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

TSharedRef<SWidget> UUserWidget::MakeWidget()
{
	return SNew(SObjectWidget, this)
	[
		UserRootWidget.ToSharedRef()
	];
}

TSharedRef<SWidget> UUserWidget::MakeFullScreenWidget()
{
	if ( bAbsoluteLayout )
	{
		return SNew(SCanvas)

			+ SCanvas::Slot()
			.Position(AbsolutePosition)
			.Size(AbsoluteSize)
			.VAlign(VerticalAlignment)
			.HAlign(HorizontalAlignment)
			[
				MakeWidget()
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
				MakeWidget()
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

void UUserWidget::Show()
{
	if ( !FullScreenWidget.IsValid() )
	{
		TSharedRef<SWidget> RootWidget = MakeFullScreenWidget();

		TSharedRef<SViewportWidgetHost> WidgetHost = SNew(SViewportWidgetHost, (bool)bModal)
			[
				RootWidget
			];

		FullScreenWidget = WidgetHost;

		//WidgetHost->SetVisibility(EVisibility::Visible);
		//OnVisibilityChanged.Broadcast(ESlateVisibility::Visible);

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

void UUserWidget::Hide()
{
	if ( FullScreenWidget.IsValid() )
	{
		TSharedPtr<SWidget> RootWidget = FullScreenWidget.Pin();

		//RootWidget->SetVisibility(EVisibility::Hidden);
		//OnVisibilityChanged.Broadcast(ESlateVisibility::Hidden);

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
