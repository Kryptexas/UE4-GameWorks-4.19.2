// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

typedef TSharedPtr<class FComponentClassComboEntry> FComponentClassComboEntryPtr;

//////////////////////////////////////////////////////////////////////////

namespace EComponentCreateAction
{
	enum Type
	{
		/** Create a new C++ class based off the specified ActorComponent class and then add it to the tree */
		CreateNewCPPClass,
		/** Create a new blueprint class based off the specified ActorComponent class and then add it to the tree */
		CreateNewBlueprintClass,
		/** Spawn a new instance of the specified ActorComponent class and then add it to the tree */
		SpawnExistingClass,
	};
}

DECLARE_DELEGATE_TwoParams(FComponentClassSelected, TSubclassOf<UActorComponent>, EComponentCreateAction::Type );

class FComponentClassComboEntry: public TSharedFromThis<FComponentClassComboEntry>
{
public:
	FComponentClassComboEntry( const FString& InHeadingText, TSubclassOf<UActorComponent> InComponentClass, bool InIncludedInFilter, EComponentCreateAction::Type InComponentCreateAction )
		: ComponentClass(InComponentClass)
		, HeadingText(InHeadingText)
		, bIncludedInFilter(InIncludedInFilter)
		, ComponentCreateAction(InComponentCreateAction)
	{}

	FComponentClassComboEntry( const FString& InHeadingText )
		: ComponentClass(NULL)
		, HeadingText(InHeadingText)
		, bIncludedInFilter(false)
	{}

	FComponentClassComboEntry()
		: ComponentClass(NULL)
	{}


	TSubclassOf<UActorComponent> GetComponentClass() const { return ComponentClass; }

	const FString& GetHeadingText() const { return HeadingText; }

	bool IsHeading() const
	{
		return (ComponentClass==NULL && !HeadingText.IsEmpty());
	}
	bool IsSeparator() const
	{
		return (ComponentClass==NULL && HeadingText.IsEmpty());
	}
	
	bool IsClass() const
	{
		return (ComponentClass!=NULL);
	}
	
	bool IsIncludedInFilter() const
	{
		return bIncludedInFilter;
	}
	
	EComponentCreateAction::Type GetComponentCreateAction() const
	{
		return ComponentCreateAction;
	}

private:
	TSubclassOf<UActorComponent> ComponentClass;
	FString HeadingText;
	bool bIncludedInFilter;
	EComponentCreateAction::Type ComponentCreateAction;
};

//////////////////////////////////////////////////////////////////////////

class UNREALED_API SComponentClassCombo : public SComboButton
{
public:
	SLATE_BEGIN_ARGS( SComponentClassCombo )
		: _IncludeText(true)
	{}

		SLATE_ATTRIBUTE(bool, IncludeText)
		SLATE_EVENT( FComponentClassSelected, OnComponentClassSelected )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Clear the current combo list selection */
	void ClearSelection();

	/**
	 * Updates the filtered list of component names.
	 * @param InSearchText The search text from the search control.
	 */
	void GenerateFilteredComponentList(const FString& InSearchText);

	FText GetCurrentSearchString() const;

	/**
	 * Called when the user changes the text in the search box.
	 * @param InSearchText The new search string.
	 */
	void OnSearchBoxTextChanged( const FText& InSearchText );

	/** Callback when the user commits the text in the searchbox */
	void OnSearchBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo);

	void OnAddComponentSelectionChanged( FComponentClassComboEntryPtr InItem, ESelectInfo::Type SelectInfo );

	TSharedRef<ITableRow> GenerateAddComponentRow( FComponentClassComboEntryPtr Entry, const TSharedRef<STableViewBase> &OwnerTable ) const;

	/** Update list of component classes */
	void UpdateComponentClassList();

	/** Returns a component name without the substring "Component" and sanitized for display */
	static FString GetSanitizedComponentName( UClass* ComponentClass );

protected:
	virtual FReply OnButtonClicked();

	/** Called when a project is hot reloaded to refresh the components list */
	void OnProjectHotReloaded( bool bWasTriggeredAutomatically );
private:

	FText GetFriendlyComponentName(FComponentClassComboEntryPtr Entry) const;

	FComponentClassSelected OnComponentClassSelected;

	/** List of component class names used by combo box */
	TArray<FComponentClassComboEntryPtr> ComponentClassList;

	/** List of component class names, filtered by the current search string */
	TArray<FComponentClassComboEntryPtr> FilteredComponentClassList;

	/** The current search string */
	FText CurrentSearchString;

	/** The search box control - part of the combo drop down */
	TSharedPtr<SSearchBox> SearchBox;

	/** The component list control - part of the combo drop down */
	TSharedPtr< SListView<FComponentClassComboEntryPtr> > ComponentClassListView;

	/** Cached selection index used to skip over unselectable items */
	int32 PrevSelectedIndex;
};
