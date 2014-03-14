// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "InputStructCustomization.h"
#include "InputSettingsDetails.h"

#define LOCTEXT_NAMESPACE "InputStructCustomization"

///////////////////////////////////
// FInputAxisConfigCustomization //
///////////////////////////////////

TSharedRef<IStructCustomization> FInputAxisConfigCustomization::MakeInstance() 
{
	return MakeShareable( new FInputAxisConfigCustomization );
}

void FInputAxisConfigCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	FString AxisKeyName;
	InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputAxisConfigEntry, AxisKeyName))->GetValue(AxisKeyName);

	HeaderRow.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget(AxisKeyName)
	];
}

void FInputAxisConfigCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> AxisProperties = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputAxisConfigEntry, AxisProperties));

	uint32 NumChildren;
	AxisProperties->GetNumChildren( NumChildren );

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		StructBuilder.AddChildProperty( AxisProperties->GetChildHandle(ChildIndex).ToSharedRef() );
	}
}

//////////////////////////////////////
// FInputActionMappingCustomization //
//////////////////////////////////////

TSharedRef<IStructCustomization> FInputActionMappingCustomization::MakeInstance() 
{
	return MakeShareable( new FInputActionMappingCustomization );
}

void FInputActionMappingCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	ActionMappingHandle = InStructPropertyHandle;
}

void FInputActionMappingCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> KeyHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputActionKeyMapping, Key));
	TSharedPtr<IPropertyHandle> ShiftHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputActionKeyMapping, bShift));
	TSharedPtr<IPropertyHandle> CtrlHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputActionKeyMapping, bCtrl));
	TSharedPtr<IPropertyHandle> AltHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputActionKeyMapping, bAlt));

	TSharedRef<SWidget> RemoveButton = PropertyCustomizationHelpers::MakeDeleteButton(FSimpleDelegate::CreateSP(this, &FInputActionMappingCustomization::RemoveActionMappingButton_OnClick),
		LOCTEXT("RemoveActionMappingToolTip", "Removes Action Mapping"));

	StructBuilder.AddChildContent( LOCTEXT("KeySearchStr", "Key").ToString() )
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( InputConstants::TextBoxWidth )
			[
				StructBuilder.GenerateStructValueWidget( KeyHandle.ToSharedRef() )
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ShiftHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ShiftHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			CtrlHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			CtrlHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AltHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AltHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			RemoveButton
		]
	];
}

void FInputActionMappingCustomization::RemoveActionMappingButton_OnClick()
{
	if( ActionMappingHandle->IsValidHandle() )
	{
		const TSharedPtr<IPropertyHandle> ParentHandle = ActionMappingHandle->GetParentHandle();
		const TSharedPtr<IPropertyHandleArray> ParentArrayHandle = ParentHandle->AsArray();

		ParentArrayHandle->DeleteItem( ActionMappingHandle->GetIndexInArray() );
	}
}

//////////////////////////////////////
// FInputAxisMappingCustomization //
//////////////////////////////////////

TSharedRef<IStructCustomization> FInputAxisMappingCustomization::MakeInstance() 
{
	return MakeShareable( new FInputAxisMappingCustomization );
}

void FInputAxisMappingCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	AxisMappingHandle = InStructPropertyHandle;
}

void FInputAxisMappingCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	TSharedPtr<IPropertyHandle> KeyHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputAxisKeyMapping, Key));
	TSharedPtr<IPropertyHandle> ScaleHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInputAxisKeyMapping, Scale));

	TSharedRef<SWidget> RemoveButton = PropertyCustomizationHelpers::MakeDeleteButton( FSimpleDelegate::CreateSP( this, &FInputAxisMappingCustomization::RemoveAxisMappingButton_OnClick), 
		LOCTEXT("RemoveAxisMappingToolTip", "Removes Axis Mapping") );

	StructBuilder.AddChildContent( LOCTEXT("KeySearchStr", "Key").ToString() )
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( InputConstants::TextBoxWidth )
			[
				StructBuilder.GenerateStructValueWidget( KeyHandle.ToSharedRef() )
			]
		]
		+SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ScaleHandle->CreatePropertyNameWidget()
		]
		+SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(InputConstants::ScaleBoxWidth)
			[
				ScaleHandle->CreatePropertyValueWidget()
			]
		]
		+SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			RemoveButton
		]
	];
}

void FInputAxisMappingCustomization::RemoveAxisMappingButton_OnClick()
{
	if( AxisMappingHandle->IsValidHandle() )
	{
		const TSharedPtr<IPropertyHandle> ParentHandle = AxisMappingHandle->GetParentHandle();
		const TSharedPtr<IPropertyHandleArray> ParentArrayHandle = ParentHandle->AsArray();

		ParentArrayHandle->DeleteItem( AxisMappingHandle->GetIndexInArray() );
	}
}

#undef LOCTEXT_NAMESPACE
