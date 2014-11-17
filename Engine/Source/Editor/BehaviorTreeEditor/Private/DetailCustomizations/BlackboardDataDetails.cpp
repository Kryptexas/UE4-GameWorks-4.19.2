// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BlackboardDataDetails.h"


TSharedRef<IDetailCustomization> FBlackboardDataDetails::MakeInstance(FOnGetSelectedBlackboardItemIndex InOnGetSelectedBlackboardItemIndex)
{
	return MakeShareable( new FBlackboardDataDetails(InOnGetSelectedBlackboardItemIndex) );
}

void FBlackboardDataDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	// First hide all keys
	DetailLayout.HideProperty(TEXT("Keys"));
	DetailLayout.HideProperty(TEXT("ParentKeys"));

	// Now show only the currently selected key
	bool bIsInherited = false;
	int32 CurrentSelection = INDEX_NONE;
	if(OnGetSelectedBlackboardItemIndex.IsBound())
	{
		CurrentSelection = OnGetSelectedBlackboardItemIndex.Execute(bIsInherited);
	}

	if(CurrentSelection >= 0)
	{
		TSharedPtr<IPropertyHandle> KeysHandle = bIsInherited ? DetailLayout.GetProperty(TEXT("ParentKeys")) : DetailLayout.GetProperty(TEXT("Keys"));
		check(KeysHandle.IsValid());
		uint32 NumChildKeys = 0;
		KeysHandle->GetNumChildren(NumChildKeys);
		if((uint32)CurrentSelection < NumChildKeys)
		{
			TSharedPtr<IPropertyHandle> KeyHandle = KeysHandle->GetChildHandle((uint32)CurrentSelection);

			IDetailCategoryBuilder& DetailCategoryBuilder = DetailLayout.EditCategory("Key");
			DetailCategoryBuilder.AddProperty(KeyHandle);
		}	
	}
}
