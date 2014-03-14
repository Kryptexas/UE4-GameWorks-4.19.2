// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SlateColorCustomization.h"

TSharedRef<IStructCustomization> FSlateColorCustomization::MakeInstance() 
{
	return MakeShareable( new FSlateColorCustomization() );
}

void FSlateColorCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		const TSharedRef< IPropertyHandle > ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();

		if ( ChildHandle->GetProperty()->GetName() == TEXT("ColorUseRule") )
		{
			ColorRuleHandle = ChildHandle;
		}
		else if ( ChildHandle->GetProperty()->GetName() == TEXT("SpecifiedColor") )
		{
			SpecifiedColorHandle = ChildHandle;
		}
	}

	check( ColorRuleHandle.IsValid() );
	check( SpecifiedColorHandle.IsValid() );
}

void FSlateColorCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	SpecifiedColorHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP( SharedThis( this ), &FSlateColorCustomization::OnValueChanged ) );
	StructBuilder.AddChildProperty( SpecifiedColorHandle.ToSharedRef() )
		.DisplayName( StructPropertyHandle->GetPropertyDisplayName() )
		.ToolTip( StructPropertyHandle->GetToolTipText() );
}

void FSlateColorCustomization::OnValueChanged()
{
	ColorRuleHandle->SetValueFromFormattedString(TEXT("UseColor_Specified"));
}
