// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditorModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "Editor/UnrealEd/Public/Kismet2/StructureEditorUtils.h"

class FUserDefinedStructureEditor : public IUserDefinedStructureEditor
{
	/** App Identifier.*/
	static const FName UserDefinedStructureEditorAppIdentifier;

	/**	The tab ids for all the tabs used */
	static const FName MemberVariablesTabId;
	
	/** Property viewing widget */
	TSharedPtr<class IDetailsView> PropertyView;

public:
	/**
	 * Edits the specified enum
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	EnumToEdit				The user defined enum to edit
	 */
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UUserDefinedStruct* EnumToEdit);

	/** Destructor */
	virtual ~FUserDefinedStructureEditor();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

protected:
	TSharedRef<SDockTab> SpawnStructureTab(const FSpawnTabArgs& Args);
};

class FUserDefinedStructureDetails : public IDetailCustomization, FStructureEditorUtils::INotifyOnStructChanged
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FUserDefinedStructureDetails);
	}

	~FUserDefinedStructureDetails();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

	UUserDefinedStruct* GetUserDefinedStruct();

	struct FStructVariableDescription* FindStructureFieldByGuid(FGuid Guid);

	/** FStructureEditorUtils::INotifyOnStructChanged */
	virtual void OnChanged(const class UUserDefinedStruct* Struct) OVERRIDE;

private:
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	TSharedPtr<class FUserDefinedStructureLayout> Layout;
};
