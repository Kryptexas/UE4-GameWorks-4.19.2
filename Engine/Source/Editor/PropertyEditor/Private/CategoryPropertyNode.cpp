// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PropertyEditorPrivatePCH.h"
#include "PropertyNode.h"
#include "CategoryPropertyNode.h"
#include "ObjectPropertyNode.h"
#include "ItemPropertyNode.h"
#include "ObjectEditorUtils.h"


FCategoryPropertyNode::FCategoryPropertyNode(void)
	: FPropertyNode()
{
}

FCategoryPropertyNode::~FCategoryPropertyNode(void)
{
}

bool FCategoryPropertyNode::IsSubcategory() const
{
	return GetParentNode() != NULL && const_cast<FPropertyNode*>( GetParentNode() )->AsCategoryNode() != NULL;
}

FString FCategoryPropertyNode::GetDisplayName() const 
{
	FString SubcategoryName = GetSubcategoryName();
	//if in "readable display name mode" return that
	if ( FPropertySettings::Get().ShowFriendlyPropertyNames() )
	{
		FString DisplaySubcategoryName = EngineUtils::SanitizeDisplayName( SubcategoryName, false );

		//if there is a localized version, return that
		FText LocalizedName;
		if ( FText::FindText( TEXT("UObjectCategories"), *SubcategoryName, /*OUT*/LocalizedName ) )
		{
			return LocalizedName.ToString();
		}

		return DisplaySubcategoryName;
	}
	return SubcategoryName;
}	
/**
 * Overridden function for special setup
 */
void FCategoryPropertyNode::InitBeforeNodeFlags()
{

}
/**
 * Overridden function for Creating Child Nodes
 */
void FCategoryPropertyNode::InitChildNodes()
{
	const bool bShowHiddenProperties = !!HasNodeFlags( EPropertyNodeFlags::ShouldShowHiddenProperties );

	TArray<UProperty*> Properties;
	// The parent of a category window has to be an object window.
	FObjectPropertyNode* itemobj = FindObjectItemParent();
	// Get a list of properties that are in the same category
	for( TFieldIterator<UProperty> It(itemobj->GetObjectBaseClass()); It; ++It )
	{
		bool bMetaDataAllowVisible = true;
		FString MetaDataVisibilityCheckString = It->GetMetaData(TEXT("bShowOnlyWhenTrue"));
		if (MetaDataVisibilityCheckString.Len())
		{
			//ensure that the metadata visibility string is actually set to true in order to show this property
			// @todo Remove this
			GConfig->GetBool(TEXT("UnrealEd.PropertyFilters"), *MetaDataVisibilityCheckString, bMetaDataAllowVisible, GEditorUserSettingsIni);
		}

		if (bMetaDataAllowVisible)
		{
			// Add if we are showing non-editable props and this is the 'None' category, 
			// or if this is the right category, and we are either showing non-editable
			if ( FObjectEditorUtils::GetCategoryFName(*It) == CategoryName && (bShowHiddenProperties || (It->PropertyFlags & CPF_Edit)) )
			{
				Properties.Add( *It );
			}
		}
	}

	for( int32 PropertyIndex = 0 ; PropertyIndex < Properties.Num() ; ++PropertyIndex )
	{
		TSharedPtr<FItemPropertyNode> NewItemNode( new FItemPropertyNode );

		FPropertyNodeInitParams InitParams;
		InitParams.ParentNode = SharedThis(this);
		InitParams.Property = Properties[PropertyIndex];
		InitParams.ArrayOffset = 0;
		InitParams.ArrayIndex = INDEX_NONE;
		InitParams.bAllowChildren = true;
		InitParams.bForceHiddenPropertyVisibility = bShowHiddenProperties;

		NewItemNode->InitNode( InitParams );

		AddChildNode(NewItemNode);
	}
}


/**
 * Appends my path, including an array index (where appropriate)
 */
void FCategoryPropertyNode::GetQualifiedName( FString& PathPlusIndex, const bool bWithArrayIndex ) const
{
	if( ParentNode )
	{
		ParentNode->GetQualifiedName(PathPlusIndex, bWithArrayIndex);
		PathPlusIndex += TEXT(".");
	}
	
	GetCategoryName().AppendString(PathPlusIndex);
}


FString FCategoryPropertyNode::GetSubcategoryName() const 
{
	FString SubcategoryName;
	{
		// The category name may actually contain a path of categories.  When displaying this category
		// in the property window, we only want the leaf part of the path
		const FString& CategoryPath = GetCategoryName().ToString();
		FString CategoryDelimiterString;
		CategoryDelimiterString.AppendChar( FPropertyNodeConstants::CategoryDelimiterChar );  
		const int32 LastDelimiterCharIndex = CategoryPath.Find( CategoryDelimiterString );
		if( LastDelimiterCharIndex != INDEX_NONE )
		{
			// Grab the last sub-category from the path
			SubcategoryName = CategoryPath.Mid( LastDelimiterCharIndex + 1 );
		}
		else
		{
			// No sub-categories, so just return the original (clean) category path
			SubcategoryName = CategoryPath;
		}
	}
	return SubcategoryName;
}
