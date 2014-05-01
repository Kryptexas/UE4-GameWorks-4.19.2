// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

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
	if ( bShowCursorWhenVisible && GetIsVisible() )
	{
		GetWorld()->GetFirstLocalPlayerFromController()->PlayerController->bShowMouseCursor = true;
	}
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
		RootWidget = SlateWrapperComponents[0]->GetWidget();
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
		UGameViewportClient* Viewport = World->GetGameViewport();
		Viewport->AddViewportWidgetContent(RootWidget.ToSharedRef());
		RootWidget->SetVisibility(USlateWrapperComponent::ConvertSerializedVisibilityToRuntime(Visiblity));
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
}

void AUserWidget::Hide()
{
	RootWidget->SetVisibility(EVisibility::Hidden);
	OnVisibilityChanged.Broadcast(ESlateVisibility::Hidden);
}

bool AUserWidget::GetIsVisible()
{
	return RootWidget->GetVisibility().IsVisible();
}

TEnumAsByte<ESlateVisibility::Type> AUserWidget::GetVisiblity()
{
	return USlateWrapperComponent::ConvertRuntimeToSerializedVisiblity(RootWidget->GetVisibility());
}
