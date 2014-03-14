// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Asset editor common commands */
class FAssetEditorCommonCommands : public TCommands< FAssetEditorCommonCommands >
{

public:

	FAssetEditorCommonCommands()
		: TCommands< FAssetEditorCommonCommands >( TEXT("AssetEditor"), NSLOCTEXT("Contexts", "AssetEditor", "Asset Editor"), NAME_None, FEditorStyle::GetStyleSetName() )
	{
	}	

	virtual void RegisterCommands() OVERRIDE;

	TSharedPtr< FUICommandInfo > SaveAsset;
	TSharedPtr< FUICommandInfo > Reimport;
	TSharedPtr< FUICommandInfo > SwitchToStandaloneEditor;
	TSharedPtr< FUICommandInfo > SwitchToWorldCentricEditor;
};

