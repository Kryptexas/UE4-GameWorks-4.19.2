// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// AUserWidget

AUserWidget::AUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RootWidget = SNullWidget::NullWidget;
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

USlateWrapperComponent* AUserWidget::GetWidgetHandle(TSharedRef<SWidget> InWidget)
{
	return WidgetToComponent.FindRef(&InWidget.Get());
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

		WidgetToComponent.Add(&Widget.Get(), Handle);
	}
	
	// If this is a game world add the widget to the current worlds viewport.
	UWorld* World = GetWorld();
	if ( World && World->IsGameWorld() )
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
		Viewport->AddViewportWidgetContent(RootWidget.ToSharedRef());
	}
}

TSharedRef<SWidget> AUserWidget::GetRootWidget()
{
	return RootWidget.ToSharedRef();
}
