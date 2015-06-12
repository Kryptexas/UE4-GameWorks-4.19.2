// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "GameplayTagContainer.h"

/** Main CollisionAnalyzer UI widget */
class SGameplayCueEditor : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SGameplayCueEditor) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SGameplayCueEditor();

	void OnNewGameplayCueTagCommited(const FText& InText, ETextCommit::Type InCommitType);
	void OnSearchTagCommited(const FText& InText, ETextCommit::Type InCommitType);

	FReply OnNewGameplayCueButtonPressed();
	
	static FString GetPathNameForGameplayCueTag(FString Tag);

private:

	TSharedPtr<SEditableTextBox>	NewGameplayCueTextBox;

	void CreateNewGameplayCueTag();

	void HandleShowAllCheckedStateChanged( ECheckBoxState NewValue );

	/** Callback for getting the checked state of the focus check box. */
	ECheckBoxState HandleShowAllCheckBoxIsChecked() const;

	bool bShowAll;

	struct FGameplayCueEditorHandlerItem
	{
		FStringAssetReference GameplayCueNotifyObj;

		TWeakObjectPtr<UFunction> FunctionPtr;
	};

	typedef SListView< TSharedPtr< FGameplayCueEditorHandlerItem > > SGameplayCueHandlerListView;
	typedef TArray< TSharedPtr< FGameplayCueEditorHandlerItem > > FGameplayCueHandlerArray;

	/** An item in the GameplayCue list */
	struct FGameplayCueEditorItem
	{
		FGameplayTag GameplayCueTagName;

		FGameplayCueHandlerArray HandlerItems;

		TSharedPtr< SGameplayCueHandlerListView > GameplayCueHandlerListView;

		FReply OnAddNewClicked();
		void OnNewClassPicked(FString SelectedPath);
	};

	typedef SListView< TSharedPtr< FGameplayCueEditorItem > > SGameplayCueListView;
	typedef TArray< TSharedPtr< FGameplayCueEditorItem > > FGameplayCueArray;
	

	TSharedRef<ITableRow> OnGenerateWidgetForGameplayCueListView(TSharedPtr< FGameplayCueEditorItem > InItem, const TSharedRef<STableViewBase>& OwnerTable);

	TSharedPtr< SGameplayCueListView > GameplayCueListView;
	FGameplayCueArray GameplayCueListItems;

	void UpdateGameplayCueListItems();

	FReply BuildEventMap();
	
	TMultiMap<FGameplayTag, UFunction*> EventMap;

	FString AutoSelectTag;
	FString SearchString;
};
