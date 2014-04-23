// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// AUserWidget

AUserWidget::AUserWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void AUserWidget::PostActorCreated()
{
	EnsureWidgetExists();
	Super::PostActorCreated();
}

void AUserWidget::Destroyed()
{
	MyWrapperWidget = NULL;
	Super::Destroyed();
}

void AUserWidget::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();

	RebuildWrapperWidget();
}

void AUserWidget::EnsureWidgetExists()
{
	if (!MyWrapperWidget.IsValid())
	{
		MyWrapperWidget = SNew(SOverlay);
	}
}

void AUserWidget::RebuildWrapperWidget()
{
	EnsureWidgetExists();
	MyWrapperWidget->ClearChildren();
	
	TArray<USlateWrapperComponent*> SlateWrapperComponents;
	GetComponents(SlateWrapperComponents);
	// Place all of our top-level children Slate wrapped components into the overlay
	for (int32 ComponentIndex = 0; ComponentIndex < SlateWrapperComponents.Num(); ++ComponentIndex)
	{
		if (SlateWrapperComponents[ComponentIndex]->IsRegistered() && (SlateWrapperComponents[ComponentIndex]->AttachParent == NULL))
		{
			MyWrapperWidget->AddSlot()
			[
				SlateWrapperComponents[ComponentIndex]->GetWidget()
			];
		}
	}
	
	if (GEngine->GameViewport != NULL)
	{
		GEngine->GameViewport->AddViewportWidgetContent(MyWrapperWidget.ToSharedRef());
	}
//@TODO: Figure out how hot-reloading/etc... should work with slate wrapped components
	//SAssignNew(ScoreboardWidgetContainer,SWeakWidget)
	//.PossiblyNullContent(ScoreboardWidgetOverlay));
}

TSharedRef<SWidget> AUserWidget::GetWidget()
{
	EnsureWidgetExists();
	return MyWrapperWidget.ToSharedRef();
}
