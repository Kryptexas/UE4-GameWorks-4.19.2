// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Slate.h"
#include "SComponentClassCombo.h"
#include "ComponentAssetBroker.h"
#include "ClassIconFinder.h"
#include "BlueprintGraphDefinitions.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "ComponentClassCombo"


void SComponentClassCombo::Construct(const FArguments& InArgs)
{
	OnComponentClassSelected = InArgs._OnComponentClassSelected;

	UpdateComponentClassList();
	GenerateFilteredComponentList(FString());

	// Create the Construct arguments for the parent class (SComboButton)
	SComboButton::FArguments Args;
	Args.ButtonContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("AddComponentButtonLabel", "Add Component") )
	]
	.MenuContent()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText( LOCTEXT( "BlueprintAddComponentSearchBoxHint", "Search Components" ) )
			.OnTextChanged( this, &SComponentClassCombo::OnSearchBoxTextChanged )
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
			.Orientation(EOrientation::Orient_Horizontal)
		]
		+SVerticalBox::Slot()
		.MaxHeight(400)
		[
			SAssignNew(ComponentClassListView, SListView<FComponentClassComboEntryPtr>)
			.ListItemsSource( &FilteredComponentClassList )
			.OnSelectionChanged( this, &SComponentClassCombo::OnAddComponentSelectionChanged )
			.OnGenerateRow( this, &SComponentClassCombo::GenerateAddComponentRow )
			.SelectionMode(ESelectionMode::Single)
		]
	]
	.IsFocusable(true)
	.ContentPadding(FMargin(2));

	SComboButton::Construct(Args);

	// The base class can automatically handle setting focus to a specified control when the combo button is opened
	SetMenuContentWidgetToFocus( SearchBox );
}

void SComponentClassCombo::ClearSelection()
{
	SearchBox->SetText(FText::GetEmpty());
	ComponentClassListView->ClearSelection();
}

void SComponentClassCombo::GenerateFilteredComponentList(const FString& InSearchText)
{
	if ( InSearchText.IsEmpty() )
	{
		FilteredComponentClassList = ComponentClassList;
	}
	else
	{
		FilteredComponentClassList.Empty();
		for ( int32 ComponentIndex = 0; ComponentIndex < ComponentClassList.Num(); ComponentIndex++ )
		{
			FComponentClassComboEntryPtr& CurrentEntry = ComponentClassList[ComponentIndex];

			if ( CurrentEntry->IsClass() )
			{
				FString FriendlyComponentName = GetSanitizedComponentName( CurrentEntry->GetComponentClass() );

				if ( FriendlyComponentName.Contains( InSearchText, ESearchCase::IgnoreCase ) )
				{
					FilteredComponentClassList.Add( CurrentEntry );
				}
			}
		}
	}
}

FText SComponentClassCombo::GetCurrentSearchString() const
{
	return CurrentSearchString;
}

void SComponentClassCombo::OnSearchBoxTextChanged( const FText& InSearchText )
{
	CurrentSearchString = InSearchText;

	// Generate a filtered list
	GenerateFilteredComponentList(CurrentSearchString.ToString());

	// Ask the combo to update its contents on next tick
	ComponentClassListView->RequestListRefresh();
}

void SComponentClassCombo::OnAddComponentSelectionChanged( FComponentClassComboEntryPtr InItem, ESelectInfo::Type SelectInfo )
{
	if ( InItem.IsValid() )
	{
		// We don't want the item to remain selected
		ClearSelection();

		if ( InItem->IsClass() )
		{
			OnComponentClassSelected.ExecuteIfBound(InItem->GetComponentClass());

			// Neither do we want the combo dropdown staying open once the user has clicked on a valid option
			SetIsOpen( false, false );
		}
	}
}

TSharedRef<ITableRow> SComponentClassCombo::GenerateAddComponentRow( FComponentClassComboEntryPtr Entry, const TSharedRef<STableViewBase> &OwnerTable ) const
{
	check( Entry->IsHeading() || Entry->IsSeparator() || Entry->IsClass() );

	if ( Entry->IsHeading() )
	{
		return 
			SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
			[
				SNew(STextBlock)
				.Text(Entry->GetHeadingText())
			];
	}
	else if ( Entry->IsSeparator() )
	{
		return 
			SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
			[
				SNew(SSeparator)
			];
	}
	else
	{
		// Get a user friendly string from the component name
		FString FriendlyComponentName = GetSanitizedComponentName( Entry->GetComponentClass() );

		// Search the selected assets and look for any that can be used as a source asset for this type of component
		// If there is one we append the asset name to the component name, if there are many we append "Multiple Assets"
		FString AssetName;
		UObject* PreviousMatchingAsset = NULL;

		FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
		USelection* Selection = GEditor->GetSelectedObjects();
		for ( FSelectionIterator ObjectIter(*Selection); ObjectIter; ++ObjectIter )
		{
			UObject* Object = *ObjectIter;
			UClass* Class = Object->GetClass();

			TArray<TSubclassOf<UActorComponent> > ComponentClasses = FComponentAssetBrokerage::GetComponentsForAsset(Object);
			for ( int32 ComponentIndex = 0; ComponentIndex < ComponentClasses.Num(); ComponentIndex++ )
			{
				if ( ComponentClasses[ComponentIndex]->IsChildOf( Entry->GetComponentClass() ) )
				{
					if ( AssetName.IsEmpty() )
					{
						// If there is no previous asset then we just accept the name
						AssetName = Object->GetName();
						PreviousMatchingAsset = Object;
					}
					else
					{
						// if there is a previous asset then check that we didn't just find multiple appropriate components
						// in a single asset - if the asset differs then we don't display the name, just "Multiple Assets"
						if ( PreviousMatchingAsset != Object )
						{
							AssetName = LOCTEXT("MultipleAssetsForComponentAnnotation", "Multiple Assets").ToString();
							PreviousMatchingAsset = Object;
						}
					}
				}
			}
		}

		if ( !AssetName.IsEmpty() )
		{
			FriendlyComponentName += FString(" (") + AssetName + FString(")");
		}

		// Get info for rich tooltip
		FString DocLink = TEXT("Shared/Editor/Blueprint/VariableTypes");
		FString DocExcerptName = FString::Printf(TEXT("object'%s'"), *Entry->GetComponentClass()->GetName());

		return
			SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
			.ToolTip( IDocumentation::Get()->CreateToolTip(Entry->GetComponentClass()->GetToolTipText(), NULL, DocLink, DocExcerptName) )
			.Padding(1.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SSpacer)
					.Size(FVector2D(8.0f,1.0f))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SImage)
					.Image( FClassIconFinder::FindIconForClass( Entry->GetComponentClass() ) )
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SSpacer)
					.Size(FVector2D(3.0f,1.0f))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.HighlightText(this, &SComponentClassCombo::GetCurrentSearchString)
					.Text( FriendlyComponentName )
				]
			];
	}
}

struct SortComboEntry
{
	static const FString CommonClassGroup;

	bool operator () (const FComponentClassComboEntryPtr& A, const FComponentClassComboEntryPtr& B) const
	{
		bool bResult = false;

		// check headings first, if they are the same compare the individual entries
		int32 HeadingCompareResult = FCString::Stricmp( *A->GetHeadingText(), *B->GetHeadingText() );
		if ( HeadingCompareResult == 0 )
		{
			check(A->GetComponentClass()); check(B->GetComponentClass()); 
			bResult = FCString::Stricmp(*A->GetComponentClass()->GetName(), *B->GetComponentClass()->GetName()) < 0;
		}
		else if (CommonClassGroup == A->GetHeadingText())
		{
			bResult = true;
		}
		else if (CommonClassGroup == B->GetHeadingText())
		{
			bResult = false;
		}
		else
		{
			bResult = HeadingCompareResult < 0;
		}

		return bResult;
	}
};
const FString SortComboEntry::CommonClassGroup(TEXT("Common"));

void SComponentClassCombo::UpdateComponentClassList()
{
	ComponentClassList.Empty();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	TArray<FComponentClassComboEntryPtr> SortedClassList;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		// If this is a subclass of SceneComponent, not abstract, and tagged as spawnable from Kismet
		if (Class->IsChildOf(UActorComponent::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract) && Class->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent)) //@TODO: Fold this logic together with the one in UEdGraphSchema_K2::GetAddComponentClasses
		{
			TArray<FString> ClassGroupNames;
			Class->GetClassGroupNames( ClassGroupNames );

			FString ClassGroup;
			if (ClassGroupNames.Contains(SortComboEntry::CommonClassGroup))
			{
				ClassGroup = SortComboEntry::CommonClassGroup;
			}
			else if (ClassGroupNames.Num())
			{
				ClassGroup = ClassGroupNames[0];
			}

			FComponentClassComboEntryPtr NewEntry(new FComponentClassComboEntry(ClassGroup,	Class));
			SortedClassList.Add(NewEntry);
		}
	}

	if (SortedClassList.Num() > 0)
	{
		Sort(SortedClassList.GetTypedData(),SortedClassList.Num(), SortComboEntry());

		FString PreviousHeading;
		for ( int32 ClassIndex = 0; ClassIndex < SortedClassList.Num(); ClassIndex++ )
		{
			FComponentClassComboEntryPtr& CurrentEntry = SortedClassList[ClassIndex];

			const FString& CurrentHeadingText = CurrentEntry->GetHeadingText();

			if ( CurrentHeadingText != PreviousHeading )
			{
				// This avoids a redundant separator being added to the very top of the list
				if ( ClassIndex > 0 )
				{
					FComponentClassComboEntryPtr NewSeparator(new FComponentClassComboEntry());
					ComponentClassList.Add( NewSeparator );
				}
				FComponentClassComboEntryPtr NewHeading(new FComponentClassComboEntry( CurrentHeadingText ));
				ComponentClassList.Add( NewHeading );

				PreviousHeading = CurrentHeadingText;
			}

			ComponentClassList.Add( CurrentEntry );
		}
	}
}

FString SComponentClassCombo::GetSanitizedComponentName( UClass* ComponentClass )
{
	FString OriginalName = ComponentClass->GetName();
	FString ComponentNameWithoutComponent = OriginalName.Replace( TEXT("Component"), TEXT(""), ESearchCase::IgnoreCase );
	return EngineUtils::SanitizeDisplayName( ComponentNameWithoutComponent, false );
}

#undef LOCTEXT_NAMESPACE
