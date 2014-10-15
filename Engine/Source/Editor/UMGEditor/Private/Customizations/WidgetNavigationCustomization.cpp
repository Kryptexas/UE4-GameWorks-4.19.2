// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"
#include "Blueprint/WidgetNavigation.h"
#include "WidgetNavigationCustomization.h"

#define LOCTEXT_NAMESPACE "UMG"

// FWidgetNavigationCustomization
////////////////////////////////////////////////////////////////////////////////

void FWidgetNavigationCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	//HeaderRow
	//.WholeRowContent()
	//[
	//	SNew(SButton)
	//	.HAlign(HAlign_Center)
	//	.OnClicked(this, &FWidgetNavigationCustomization::OnCustomizeNavigation, TWeakPtr<IPropertyHandle>(PropertyHandle))
	//	[
	//		SNew(STextBlock)
	//		.Text(LOCTEXT("CustomizeNavigation", "Customize Navigation"))
	//	]
	//];
}

void FWidgetNavigationCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	//FillOutChildren(PropertyHandle, ChildBuilder, CustomizationUtils);
}

void FWidgetNavigationCustomization::FillOutChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Generate all the other children
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	for ( uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++ )
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		if ( ChildHandle->IsCustomized() )
		{
			continue;
		}

		if ( ChildHandle->GetProperty() == NULL )
		{
			FillOutChildren(ChildHandle, ChildBuilder, CustomizationUtils);
		}
		else
		{
			ChildBuilder.AddChildProperty(ChildHandle);
		}
	}
}

FReply FWidgetNavigationCustomization::OnCustomizeNavigation(TWeakPtr<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle.Pin()->GetOuterObjects(OuterObjects);

	const FScopedTransaction Transaction(LOCTEXT("InitializeNavigation", "Initialize Navigation"));

	for ( UObject* OuterObject : OuterObjects )
	{
		if ( UWidget* Widget = Cast<UWidget>(OuterObject) )
		{
			if ( !Widget->Navigation )
			{
				Widget->Modify();
				Widget->Navigation = ConstructObject<UWidgetNavigation>(UWidgetNavigation::StaticClass(), Widget);
			}
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
