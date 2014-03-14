// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorManager.h"
#include "ITranslationEditor.h"
#include "CustomFontColumn.h"

class TRANSLATIONEDITOR_API FTranslationEditor :  public ITranslationEditor
{
public:

	
	/**
	 *	Creates a new FTranslationEditor and calls Initialize
	 */
	static TSharedRef< FTranslationEditor > Create( TSharedRef< FTranslationDataManager > DataManager, FName ProjectName, FName TranslationTargetLanguage )
	{
		TSharedRef< FTranslationEditor > TranslationEditor = MakeShareable( new FTranslationEditor( DataManager, ProjectName, TranslationTargetLanguage ) );
		TranslationEditor->Initialize();
		return TranslationEditor;
	}
	
	virtual ~FTranslationEditor() {}

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The object to edit
	 */
	void InitTranslationEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UTranslationDataObject* TranslationDataToEdit );

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() OVERRIDE;

private:

	FTranslationEditor(TSharedRef< FTranslationDataManager > InDataManager, FName InProjectName, FName InTranslationTargetLanguage )
	: TranslationData(NULL)
	, DataManager(InDataManager)
	, SourceFont(FEditorStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle ))
	, TranslationTargetFont(FEditorStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle ))
	, SourceColumn(MakeShareable(new FCustomFontColumn(SourceFont)))
	, TranslationColumn(MakeShareable(new FCustomFontColumn(TranslationTargetFont)))
	, PreviewTextBlock(SNew(STextBlock)
				.Text(FText::FromString(""))
				.Font(TranslationTargetFont))
	, NamespaceTextBlock(SNew(STextBlock)
				.Text(FText::FromString("")))
	, ProjectName(InProjectName)
	, TranslationTargetLanguage(InTranslationTargetLanguage)
	{}

	/** Does some things we can't do in the constructor because we can't get a SharedRef to "this" there */ 
	void Initialize();

	/**	Spawns the untranslated tab */
	TSharedRef<SDockTab> SpawnTab_Untranslated( const FSpawnTabArgs& Args );
	/**	Spawns the review tab */
	TSharedRef<SDockTab> SpawnTab_Review( const FSpawnTabArgs& Args );
	/**	Spawns the completed tab */
	TSharedRef<SDockTab> SpawnTab_Completed( const FSpawnTabArgs& Args );
	/**	Spawns the preview tab */
	TSharedRef<SDockTab> SpawnTab_Preview( const FSpawnTabArgs& Args );
	/**	Spawns the context tab */
	TSharedRef<SDockTab> SpawnTab_Context( const FSpawnTabArgs& Args );
	/**	Spawns the history tab */
	TSharedRef<SDockTab> SpawnTab_History( const FSpawnTabArgs& Args );

	/** Map actions for the UI_COMMANDS */
	void MapActions();

	/** Change the font for the source language */
	void ChangeSourceFont();

	/** For button delegate */
	FReply ChangeSourceFont_FReply()
	{
		ChangeSourceFont();

		return FReply::Handled();
	}

	/** Change the font for the target translation language */
	void ChangeTranslationTargetFont();

	/** For button delegate */
	FReply ChangeTranslationTargetFont_FReply()
	{
		ChangeTranslationTargetFont();

		return FReply::Handled();
	}

	/** Called on SpinBox OnValueCommitted */
	void OnSourceFontSizeCommitt(int32 NewFontSize, ETextCommit::Type)
	{
		SourceFont.Size = NewFontSize;
		RefreshUI();
	}

	void OnTranslationTargetFontSizeCommitt(int32 NewFontSize, ETextCommit::Type)
	{
		TranslationTargetFont.Size = NewFontSize;
		RefreshUI();
	}

	/** Open the file dialog prompt (at the DefaultFile location) to allow the user to pick a font, then return the user's selection, and a boolean of whether something was selected */
	bool OpenFontPicker(const FString DefaultFile, FString& OutFile);

	/** Reset all of the UI after a new font is chosen */
	void RefreshUI();

	/** Update content when a new translation unit selection is made */
	void UpdateTranslationUnitSelection();

	/** Update content when a new context selection is made */
	void UpdateContextSelection();

	/**	The tab id for the untranslated tab */
	static const FName UntranslatedTabId;
	/**	The tab id for the review tab */
	static const FName ReviewTabId;
	/**	The tab id for the completed tab */
	static const FName CompletedTabId;
	/**	The tab id for the preview tab */
	static const FName PreviewTabId;
	/**	The tab id for the context tab */
	static const FName ContextTabId;
	/**	The tab id for the history tab */
	static const FName HistoryTabId;

	/** The Untranslated Tab */
	TSharedPtr<SDockTab> UntranslatedTab;
	/** The Review Tab */
	TSharedPtr<SDockTab> ReviewTab;
	/** The Review Tab */
	TSharedPtr<SDockTab> CompletedTab;

	/** UObject containing our translation information */
	UTranslationDataObject* TranslationData;

	/** Manages the reading and writing of data to file */
	TSharedRef< FTranslationDataManager > DataManager;

	/** Array of Objects for the Property Table */
	TArray<UObject*>  PropertyTableObjects;

	/** The table of untranslated items */
	TSharedPtr< class IPropertyTable > UntranslatedPropertyTable;
	/** The table of translations to review */
	TSharedPtr< class IPropertyTable > ReviewPropertyTable;
	/** The table of completed translations */
	TSharedPtr< class IPropertyTable > CompletedPropertyTable;
	/** The table of context information */
	TSharedPtr< class IPropertyTable > ContextPropertyTable;
	/** The table of previous revision information */
	TSharedPtr< class IPropertyTable > HistoryPropertyTable;

	/** The slate widget table of untranslated items */
	TSharedPtr< class IPropertyTableWidgetHandle > UntranslatedPropertyTableWidgetHandle;
	/** The slate widget table of translations to review */
	TSharedPtr< class IPropertyTableWidgetHandle > ReviewPropertyTableWidgetHandle;
	/** The slate widget table of completed items */
	TSharedPtr< class IPropertyTableWidgetHandle > CompletedPropertyTableWidgetHandle;
	/** The slate widget table of contexts for this item */
	TSharedPtr< class IPropertyTableWidgetHandle > ContextPropertyTableWidgetHandle;
	/** The slate widget table of previous revision information */
	TSharedPtr< class IPropertyTableWidgetHandle > HistoryPropertyTableWidgetHandle;

	/** Font to use for the source language */
	FSlateFontInfo SourceFont;
	/** Font to use for the translation target language */
	FSlateFontInfo TranslationTargetFont;

	/** Custom FontColumn for columns that display source text */
	TSharedRef<FCustomFontColumn> SourceColumn;
	/** Custom FontColumn for columns that display source text */
	TSharedRef<FCustomFontColumn> TranslationColumn;

	/** Text block for previewing the currently selected translation */
	TSharedRef<STextBlock> PreviewTextBlock;
	/** Text block displaying the namespace of the currently selected translation unit */
	TSharedRef<STextBlock> NamespaceTextBlock;

	/** Name of the project we are translating for */
	FName ProjectName;
	/** Name of the language we are translating to */
	FName TranslationTargetLanguage;
};
