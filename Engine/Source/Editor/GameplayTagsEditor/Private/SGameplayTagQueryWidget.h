// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagsManager.h"
#include "SlateBasics.h"

/** Widget allowing user to tag assets with gameplay tags */
class SGameplayTagQueryWidget : public SCompoundWidget
{
public:

	DECLARE_DELEGATE(FOnSaveAndClose)
	DECLARE_DELEGATE(FOnCancel)
	
	SLATE_BEGIN_ARGS( SGameplayTagQueryWidget )
	: _ReadOnly(false)
	{}
		SLATE_ARGUMENT( bool, ReadOnly ) // Flag to set if the list is read only
		SLATE_EVENT(FOnSaveAndClose, OnSaveAndClose) // Called when "Save and Close" button clicked
		SLATE_EVENT(FOnCancel, OnCancel) // Called when "Close Without Saving" button clicked
	SLATE_END_ARGS()

	/** Simple struct holding a tag query and its owner for generic re-use of the widget */
	struct FEditableGameplayTagQueryDatum
	{
		/** Constructor */
		FEditableGameplayTagQueryDatum(class UObject* InOwnerObj, struct FGameplayTagQuery* InTagQuery)
			: TagQueryOwner(InOwnerObj)
			, TagQuery(InTagQuery)
		{}

		/** Owning UObject of the query being edited */
		TWeakObjectPtr<class UObject> TagQueryOwner;

		/** Tag query to edit */
		struct FGameplayTagQuery* TagQuery; 
	};

	/** Construct the actual widget */
	void Construct(const FArguments& InArgs, const TArray<FEditableGameplayTagQueryDatum>& EditableTagQueries);

	~SGameplayTagQueryWidget();

private:

	/* Flag to set if the list is read only*/
	bool bReadOnly;

	/** Containers to modify */
	TArray<FEditableGameplayTagQueryDatum> TagQueries;

	/** Called when "save and close" is clicked */
	FOnSaveAndClose OnSaveAndClose;

	/** Called when "cancel" is clicked */
	FOnCancel OnCancel;

	/** Properties Tab */
	TSharedPtr<class IDetailsView> Details;

	class UEditableGameplayTagQuery* CreateEditableQuery(FGameplayTagQuery& Q);
	TWeakObjectPtr<class UEditableGameplayTagQuery> EditableQuery;

	/** Called when the user clicks the "Expand All" button; Expands the entire tag tree */
	FReply OnSaveAndCloseClicked();

	/** Called when the user clicks the "Expand All" button; Expands the entire tag tree */
	FReply OnCancelClicked();

};

