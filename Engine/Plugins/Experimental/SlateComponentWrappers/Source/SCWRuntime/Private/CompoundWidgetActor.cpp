// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

/////////////////////////////////////////////////////
// ACompoundWidgetActor

ACompoundWidgetActor::ACompoundWidgetActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void ACompoundWidgetActor::PostActorCreated()
{
	EnsureWidgetExists();
	Super::PostActorCreated();
}

void ACompoundWidgetActor::Destroyed()
{
	MyWrapperWidget = NULL;
	Super::Destroyed();
}

void ACompoundWidgetActor::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();

	RebuildWrapperWidget();
}

void ACompoundWidgetActor::EnsureWidgetExists()
{
	if (!MyWrapperWidget.IsValid())
	{
		MyWrapperWidget = SNew(SOverlay);
	}
}

void ACompoundWidgetActor::RebuildWrapperWidget()
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

TSharedRef<SWidget> ACompoundWidgetActor::GetWidget()
{
	EnsureWidgetExists();
	return MyWrapperWidget.ToSharedRef();
}
