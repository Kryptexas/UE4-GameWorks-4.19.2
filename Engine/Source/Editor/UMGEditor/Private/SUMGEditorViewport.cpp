// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorViewport.h"
#include "UMGEditor.h"
#include "UMGEditorViewportClient.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

void SUMGEditorViewport::Construct(const FArguments& InArgs, TWeakPtr<IUMGEditor> UMGEditor, AUserWidget* ObjectToEdit)
{
	UMGEditorPtr = UMGEditor;
	Page = ObjectToEdit;

	CurrentViewMode = VMI_Lit;

	SEditorViewport::Construct( SEditorViewport::FArguments() );

	SetPreviewPage(Page);

	FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorViewport::OnObjectPropertyChanged) );
}

SUMGEditorViewport::~SUMGEditorViewport()
{
	FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorViewport::OnObjectPropertyChanged) );
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = NULL;
	}
}

void SUMGEditorViewport::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(Page);
}

void SUMGEditorViewport::RefreshViewport()
{
	// Invalidate the viewport's display.
	SceneViewport->Invalidate();
}

void SUMGEditorViewport::OnObjectPropertyChanged(UObject* ObjectBeingModified)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}
}

void SUMGEditorViewport::SetPreviewPage(AUserWidget* InPage)
{
	EditorViewportClient->SetPreviewPage(InPage);
}

bool SUMGEditorViewport::IsVisible() const
{
	return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground());
}

FUMGEditorViewportClient& SUMGEditorViewport::GetViewportClient()
{
	return *EditorViewportClient;
}

TSharedRef<FEditorViewportClient> SUMGEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable( new FUMGEditorViewportClient(UMGEditorPtr, PreviewScene, Page) );

	EditorViewportClient->bSetListenerPosition = false;

	EditorViewportClient->SetRealtime( false );
	EditorViewportClient->VisibilityDelegate.BindSP( this, &SUMGEditorViewport::IsVisible );

	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SUMGEditorViewport::MakeViewportToolbar()
{
	return NULL; 
}

EVisibility SUMGEditorViewport::OnGetViewportContentVisibility() const
{
	return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SUMGEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	//const FUMGEditorCommands& Commands = FUMGEditorCommands::Get();

	//TODO Commands
}

void SUMGEditorViewport::OnFocusViewportToSelection()
{
	//EditorViewportClient->FocusViewportOnBox(...);
}