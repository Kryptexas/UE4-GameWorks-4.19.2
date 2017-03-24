// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "PropertyEditorDelegates.h"

class FAdvancedPreviewScene;
class IDetailsView;
class UAssetViewerSettings;
class UEditorPerProjectUserSettings;

class UNREALED_API SAdvancedPreviewDetailsTab : public SCompoundWidget
{
public:
	/** Info about a per-instance details customization */
	struct FDetailCustomizationInfo
	{
		UStruct* Struct;
		FOnGetDetailCustomizationInstance OnGetDetailCustomizationInstance;
	};

	/** Info about a per-instance property type customization */
	struct FPropertyTypeCustomizationInfo
	{
		FName StructName;
		FOnGetPropertyTypeCustomizationInstance OnGetPropertyTypeCustomizationInstance;
	};

	SLATE_BEGIN_ARGS(SAdvancedPreviewDetailsTab)
		: _AdditionalSettings(nullptr)
	{}

	/** Additional settings object to display in the view */
	SLATE_ARGUMENT(UObject*, AdditionalSettings)

	/** Customizations to use for this details tab */
	SLATE_ARGUMENT(TArray<FDetailCustomizationInfo>, DetailCustomizations)

	/** Customizations to use for this details tab */
	SLATE_ARGUMENT(TArray<FPropertyTypeCustomizationInfo>, PropertyTypeCustomizations)

	SLATE_END_ARGS()

public:
	/** **/
	SAdvancedPreviewDetailsTab();
	~SAdvancedPreviewDetailsTab();

	/** SWidget functions */
	void Construct(const FArguments& InArgs, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene);

	void Refresh();

protected:
	void CreateSettingsView();
	void ComboBoxSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/);
	void UpdateSettingsView();
	void UpdateProfileNames();
	FReply AddProfileButtonClick();
	FReply RemoveProfileButtonClick();
protected:
	void OnAssetViewerSettingsRefresh(const FName& InPropertyName);
	void OnAssetViewerSettingsPostUndo();
protected:
	/** Property viewing widget */
	TSharedPtr<IDetailsView> SettingsView;
	TSharedPtr<STextComboBox> ProfileComboBox;
	TWeakPtr<FAdvancedPreviewScene> PreviewScenePtr;
	UAssetViewerSettings* DefaultSettings;
	UObject* AdditionalSettings;

	TArray<TSharedPtr<FString>> ProfileNames;
	int32 ProfileIndex;

	FDelegateHandle RefreshDelegate;
	FDelegateHandle AddRemoveProfileDelegate;
	FDelegateHandle PostUndoDelegate;

	UEditorPerProjectUserSettings* PerProjectSettings;

	TArray<FDetailCustomizationInfo> DetailCustomizations;

	TArray<FPropertyTypeCustomizationInfo> PropertyTypeCustomizations;
};
