// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ModuleManager.h"

#include "WorkspaceMenuStructureModule.h"

#include "WorkspaceMenuStructure.h"

#include "Slate.h"
#include "EditorStyle.h"

IMPLEMENT_MODULE( FWorkspaceMenuStructureModule, WorkspaceMenuStructure );

#define LOCTEXT_NAMESPACE "UnrealEditor"

class FWorkspaceMenuStructure : public IWorkspaceMenuStructure
{
public:
	virtual TSharedRef<FWorkspaceItem> GetStructureRoot() const OVERRIDE
	{
		return MenuRoot.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetAssetEditorCategory() const OVERRIDE
	{
		return AssetEditorCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetLevelEditorCategory() const OVERRIDE
	{
		return LevelEditorCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetLevelEditorViewportsCategory() const OVERRIDE
	{
		return LevelEditorViewportsCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetLevelEditorDetailsCategory() const OVERRIDE
	{
		return LevelEditorDetailsCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetLevelEditorModesCategory() const OVERRIDE
	{
		return LevelEditorModesCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetToolsCategory() const OVERRIDE
	{
		return ToolsCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetDeveloperToolsCategory() const OVERRIDE
	{
		return DeveloperToolsCategory.ToSharedRef();
	}

	virtual TSharedRef<FWorkspaceItem> GetEditOptions() const OVERRIDE
	{
		return EditOptions.ToSharedRef();
	}

	void ResetAssetEditorCategory()
	{
		AssetEditorCategory->ClearItems();
	}

	void ResetLevelEditorCategory()
	{
		LevelEditorCategory->ClearItems();
		LevelEditorViewportsCategory = LevelEditorCategory->AddGroup(LOCTEXT( "WorkspaceMenu_LevelEditorViewportCategory", "Viewports" ), FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"), true);
		LevelEditorDetailsCategory = LevelEditorCategory->AddGroup(LOCTEXT("WorkspaceMenu_LevelEditorDetailCategory", "Details" ), FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"), true );
		LevelEditorModesCategory = LevelEditorCategory->AddGroup(LOCTEXT("WorkspaceMenu_LevelEditorToolsCategory", "Tools" ), FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.EditorModes"), true );
	}

	void ResetToolsCategory()
	{
		ToolsCategory->ClearItems();
		DeveloperToolsCategory = ToolsCategory->AddGroup(LOCTEXT( "WorkspaceMenu_DeveloperToolsCategory", "Developer Tools" ), FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"), true);
	}

public:
	FWorkspaceMenuStructure()
		: MenuRoot ( FWorkspaceItem::NewGroup(LOCTEXT( "WorkspaceMenu_Root", "Menu Root" )) )
		, AssetEditorCategory ( MenuRoot->AddGroup(LOCTEXT( "WorkspaceMenu_AssetEditorCategory", "Asset Editor Tabs" )) )
		, LevelEditorCategory ( MenuRoot->AddGroup(LOCTEXT( "WorkspaceMenu_LevelEditorCategory", "Level Editor Tabs" ), FSlateIcon(), true) )
		, ToolsCategory ( MenuRoot->AddGroup(LOCTEXT( "WorkspaceMenu_ToolsCategory", "Application Windows" ), FSlateIcon(), true) )
		, EditOptions ( FWorkspaceItem::NewGroup(LOCTEXT( "WorkspaceEdit_Options", "Edit Options" )) )
	{
		ResetAssetEditorCategory();
		ResetLevelEditorCategory();
		ResetToolsCategory();
	}

private:
	TSharedPtr<FWorkspaceItem> MenuRoot;
	TSharedPtr<FWorkspaceItem> AssetEditorCategory;
	TSharedPtr<FWorkspaceItem> LevelEditorCategory;
	TSharedPtr<FWorkspaceItem> LevelEditorViewportsCategory;
	TSharedPtr<FWorkspaceItem> LevelEditorDetailsCategory;
	TSharedPtr<FWorkspaceItem> LevelEditorModesCategory;
	TSharedPtr<FWorkspaceItem> ToolsCategory;
	TSharedPtr<FWorkspaceItem> DeveloperToolsCategory;
	TSharedPtr<FWorkspaceItem> EditOptions;
};

void FWorkspaceMenuStructureModule::StartupModule()
{
	WorkspaceMenuStructure = MakeShareable(new FWorkspaceMenuStructure);
}

void FWorkspaceMenuStructureModule::ShutdownModule()
{
	WorkspaceMenuStructure.Reset();
}

const IWorkspaceMenuStructure& FWorkspaceMenuStructureModule::GetWorkspaceMenuStructure() const
{
	check(WorkspaceMenuStructure.IsValid());
	return *WorkspaceMenuStructure;
}

void FWorkspaceMenuStructureModule::ResetAssetEditorCategory()
{
	check(WorkspaceMenuStructure.IsValid());
	WorkspaceMenuStructure->ResetAssetEditorCategory();
}

void FWorkspaceMenuStructureModule::ResetLevelEditorCategory()
{
	check(WorkspaceMenuStructure.IsValid());
	WorkspaceMenuStructure->ResetLevelEditorCategory();
}

void FWorkspaceMenuStructureModule::ResetToolsCategory()
{
	check(WorkspaceMenuStructure.IsValid());
	WorkspaceMenuStructure->ResetToolsCategory();
}

#undef LOCTEXT_NAMESPACE
