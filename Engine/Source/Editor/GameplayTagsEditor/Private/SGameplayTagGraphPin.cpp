// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "SGameplayTagGraphPin.h"
#include "GameplayTags.h"

#define LOCTEXT_NAMESPACE "GameplayTagGraphPin"

void SGameplayTagGraphPin::Construct( const FArguments& InArgs, UEdGraphPin* InGraphPinObj )
{
	TagContainer = MakeShareable( new FGameplayTagContainer() );
	SGraphPin::Construct( SGraphPin::FArguments(), InGraphPinObj );
}

TSharedRef<SWidget>	SGameplayTagGraphPin::GetDefaultValueWidget()
{
	ParseDefaultValueData();

	//Create widget
	return SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew( ComboButton, SComboButton )
			.OnGetMenuContent( this, &SGameplayTagGraphPin::GetListContent )
			.ContentPadding( FMargin( 2.0f, 2.0f ) )
			.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
			.ButtonContent()
			[
				SNew( STextBlock )
				.Text( LOCTEXT("GameplayTagWidget_Edit", "Edit") )
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SelectedTags()
		];
}

void SGameplayTagGraphPin::ParseDefaultValueData()
{
	FString TagString = GraphPinObj->GetDefaultAsString();

	if( TagString.StartsWith( TEXT("(") ) && TagString.EndsWith( TEXT(")") ) )
	{
		TagString = TagString.LeftChop(1);
		TagString = TagString.RightChop(1);
		
		FString ReadTag;
		FString Remainder;
		while( TagString.Split( TEXT(","), &ReadTag, &Remainder ) )
		{
			ReadTag.Split( "=", NULL, &ReadTag );
			TagString = Remainder;
			TagContainer->AddTag( FName( *ReadTag ) );
		}
		if( !Remainder.IsEmpty() )
		{
			Remainder.Split( "=", NULL, &Remainder );
			TagContainer->AddTag( FName( *Remainder ) );
		}
	}
}

TSharedRef<SWidget> SGameplayTagGraphPin::GetListContent()
{
	EditableContainers.Empty();
	EditableContainers.Add( SGameplayTagWidget::FEditableGameplayTagContainerDatum( GraphPinObj, TagContainer.Get() ) );

	return SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight( 400 )
		[
			SNew( SGameplayTagWidget, EditableContainers )
			.OnTagChanged( this, &SGameplayTagGraphPin::RefreshTagList )
			.TagContainerName( TEXT("SGameplayTagGraphPin") )
			.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
		];
}

TSharedRef<SWidget> SGameplayTagGraphPin::SelectedTags()
{
	RefreshTagList();

	SAssignNew( TagListView, SListView<TSharedPtr<FString>> )
		.ListItemsSource(&TagNames)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SGameplayTagGraphPin::OnGenerateRow);

	return TagListView->AsShared();
}

TSharedRef<ITableRow> SGameplayTagGraphPin::OnGenerateRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		[
			SNew(STextBlock) .Text( *Item.Get() )
		];
}

void SGameplayTagGraphPin::RefreshTagList()
{	
	// Clear the list
	TagNames.Empty();

	// Add tags to list
	if (TagContainer.IsValid())
	{
		TSet<FName> AssetTags;
		TagContainer->GetTags(AssetTags);

		for (TSet<FName>::TConstIterator TagIter(AssetTags); TagIter; ++TagIter)
		{
			FString TagName = *TagIter->ToString();
			TagNames.Add( MakeShareable( new FString( TagName ) ) );
		}
	}

	// Refresh the slate list
	if( TagListView.IsValid() )
	{
		TagListView->RequestListRefresh();
	}

	// Set Pin Data
	FString TagContainerString = TEXT("(");;

 	for( int32 iName = 0; iName < TagNames.Num(); ++iName )
 	{
		TagContainerString += FString::Printf( TEXT("Tags(%d)="), iName );
 		TagContainerString += *TagNames[iName];
 
 		if( iName < TagNames.Num()-1 )
 		{
 			TagContainerString += TEXT(",");
 		}
 	}

	TagContainerString += TEXT(")");

	FString CurrentDefaultValue = GraphPinObj->GetDefaultAsString();
	if (CurrentDefaultValue.IsEmpty())
	{
		CurrentDefaultValue = FString(TEXT("()"));
	}
	if (!CurrentDefaultValue.Equals(TagContainerString))
	{
		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, TagContainerString);
	}
}

#undef LOCTEXT_NAMESPACE
