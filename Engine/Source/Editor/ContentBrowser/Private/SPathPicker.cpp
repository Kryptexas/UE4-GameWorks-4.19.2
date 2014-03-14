// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "ContentBrowserPCH.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"


void SPathPicker::Construct( const FArguments& InArgs )
{
	TSharedPtr<SPathView> PathViewPtr;
	ChildSlot
	[
		SAssignNew(PathViewPtr, SPathView)
		.OnPathSelected(InArgs._PathPickerConfig.OnPathSelected)
		.OnGetPathContextMenuExtender(InArgs._PathPickerConfig.OnGetPathContextMenuExtender)
		.FocusSearchBoxWhenOpened(InArgs._PathPickerConfig.bFocusSearchBoxWhenOpened)
		.AllowContextMenu(InArgs._PathPickerConfig.bAllowContextMenu)
		.AllowClassesFolder(false)
		.SelectionMode(ESelectionMode::Single)
	];

	const FString& DefaultPath = InArgs._PathPickerConfig.DefaultPath;
	if ( !DefaultPath.IsEmpty() )
	{
		if (InArgs._PathPickerConfig.bAddDefaultPath)
		{
			PathViewPtr->AddPath(DefaultPath, false);
		}

		TArray<FString> SelectedPaths;
		SelectedPaths.Add(DefaultPath);
		PathViewPtr->SetSelectedPaths(SelectedPaths);
	}
}


#undef LOCTEXT_NAMESPACE