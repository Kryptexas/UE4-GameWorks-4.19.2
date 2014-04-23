// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTree.h"
#include "UMGEditor.h"
#include "UMGEditorViewportClient.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

void SUMGEditorTree::Construct(const FArguments& InArgs, TWeakPtr<IUMGEditor> UMGEditor, UGUIPage* ObjectToEdit)
{
	UMGEditorPtr = UMGEditor;
	Page = ObjectToEdit;

	FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SAssignNew(PageTreeView, STreeView<TSharedPtr<FString>>)
			.ItemHeight(20.0f)
			//.OnGenerateRow(this, &SDeviceProcesses::HandleProcessTreeViewGenerateRow)
			//.OnGetChildren(this, &SDeviceProcesses::HandleProcessTreeViewGetChildren)
			//.SelectionMode(ESelectionMode::Multi)
			//.TreeItemsSource(&ProcessList)
		]
	];
}

SUMGEditorTree::~SUMGEditorTree()
{
	FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );
}

void SUMGEditorTree::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(Page);
}

void SUMGEditorTree::OnObjectPropertyChanged(UObject* ObjectBeingModified)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}
}
