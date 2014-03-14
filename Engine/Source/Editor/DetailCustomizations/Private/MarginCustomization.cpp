// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "MarginCustomization.h"

TSharedRef<IStructCustomization> FMarginStructCustomization::MakeInstance() 
{
	return MakeShareable( new FMarginStructCustomization() );
}

void FMarginStructCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	const FString UVSpaceString( StructPropertyHandle->GetProperty()->GetMetaData( TEXT( "UVSpace" ) ) );
	bIsMarginUsingUVSpace = UVSpaceString.Len() > 0 && UVSpaceString == TEXT( "true" );

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		ChildPropertyHandles.Add( StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef() );
	}

	TSharedPtr<SHorizontalBox> HorizontalBox;

	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth( 250.0f )
	.MaxDesiredWidth( 250.0f )
	[
		SAssignNew( HorizontalBox, SHorizontalBox )
	];

	HorizontalBox->AddSlot()
	[
		MakePropertyWidget()
	];

	UpdateMarginTextFromProperties();
}

void FMarginStructCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	FSimpleDelegate OnPropertyChangedDelegate = FSimpleDelegate::CreateSP( this, &FMarginStructCustomization::OnPropertyChanged );
	StructPropertyHandle->SetOnPropertyValueChanged( OnPropertyChangedDelegate );

	const uint32 NumChildren = ChildPropertyHandles.Num();

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = ChildPropertyHandles[ ChildIndex ];
		IDetailPropertyRow& PropertyRow = StructBuilder.AddChildProperty( ChildHandle );
		PropertyRow
		.CustomWidget()
		.NameContent()
		[
			ChildHandle->CreatePropertyNameWidget( ChildHandle->GetPropertyDisplayName() )
		]
		.ValueContent()
		[
			MakeChildPropertyWidget( ChildIndex, false )
		];

		ChildHandle->SetOnPropertyValueChanged( OnPropertyChangedDelegate );
	}
}

TSharedRef<SEditableTextBox> FMarginStructCustomization::MakePropertyWidget()
{
	return
		SAssignNew( MarginEditableTextBox,SEditableTextBox )
		.Text( this, &FMarginStructCustomization::GetMarginText )
		.ToolTipText( NSLOCTEXT( "UnrealEd", "MarginPropertyToolTip", "Margin values" ).ToString() )
		.OnTextCommitted( this, &FMarginStructCustomization::OnMarginTextCommitted )
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.SelectAllTextWhenFocused( true )
	;
}

TSharedRef<SWidget> FMarginStructCustomization::MakeChildPropertyWidget( int32 PropertyIndex, bool bDisplayLabel ) const
{
	return
		SNew( SNumericEntryBox<float> )
		.Value( this, &FMarginStructCustomization::OnGetValue, PropertyIndex )
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.UndeterminedString( NSLOCTEXT( "PropertyEditor", "MultipleValues", "Multiple Values").ToString() )
		.OnValueCommitted( this, &FMarginStructCustomization::OnValueCommitted, PropertyIndex )
		.OnValueChanged( this, &FMarginStructCustomization::OnValueChanged, PropertyIndex )
		.OnBeginSliderMovement( this, &FMarginStructCustomization::OnBeginSliderMovement )
		.OnEndSliderMovement( this, &FMarginStructCustomization::OnEndSliderMovement )
		.LabelVAlign( VAlign_Center )
		.AllowSpin( true )
		.MinValue( 0.0f )
		.MaxValue( bIsMarginUsingUVSpace ? 1.0f : 100.0f )
		.MinSliderValue( 0.0f )
		.MaxSliderValue( bIsMarginUsingUVSpace ? 1.0f : 100.0f )
		.Label()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( ChildPropertyHandles[ PropertyIndex ]->GetPropertyDisplayName() )
			.Visibility( bDisplayLabel ? EVisibility::Visible : EVisibility::Collapsed )
		];
}

FText FMarginStructCustomization::GetMarginText() const
{
	return FText::FromString( MarginText );
}

void FMarginStructCustomization::OnMarginTextCommitted( const FText& InText, ETextCommit::Type InCommitType )
{
	FString InString( InText.ToString() );

	bool bError = false;

	if( InCommitType != ETextCommit::OnCleared )
	{
		TArray<float> PropertyValues;

		while( InString.Len() > 0 && !bError )
		{
			FString LeftString;
			const bool bSuccess = InString.Split( TEXT( "," ), &LeftString, &InString );

			if( !InString.IsEmpty() && ((bSuccess && !LeftString.IsEmpty()) || !bSuccess) )
			{
				if( !bSuccess )
				{
					LeftString = InString;
					InString.Empty();
				}

				LeftString = LeftString.Trim().TrimTrailing();

				if( LeftString.IsNumeric() )
				{
					float Value;
					TTypeFromString<float>::FromString( Value, *LeftString );
					PropertyValues.Add( bIsMarginUsingUVSpace ? FMath::Clamp( Value, 0.0f, 1.0f ) : FMath::Max( Value, 0.0f ) );
				}
				else
				{
					bError = true;
				}
			}
			else
			{
				bError = true;
			}
		}

		if( !bError )
		{
			// Update the property values
			if( PropertyValues.Num() == 1 )
			{
				const float Value = PropertyValues[ 0 ];
				PropertyValues.Add( Value );
				PropertyValues.Add( Value );
				PropertyValues.Add( Value );
			}
			else if( PropertyValues.Num() == 2 )
			{
				float Value = PropertyValues[ 0 ];
				PropertyValues.Add( Value );
				Value = PropertyValues[ 1 ];
				PropertyValues.Add( Value );
			}
			else if( PropertyValues.Num() != 4 )
			{
				bError = true;
			}

			if( !bError )
			{
				for( int32 PropertyIndex = 0; PropertyIndex < 4; ++PropertyIndex )
				{
					TWeakPtr<IPropertyHandle> WeakHandlePtr = ChildPropertyHandles[ PropertyIndex ];
					WeakHandlePtr.Pin()->SetValue( PropertyValues[ PropertyIndex ] );
				}

				if( bIsMarginUsingUVSpace )
				{
					// Using UV space so constrain the paired margin values to be between 0.0 and 1.0
					for( int32 PropertyIndex = 0; PropertyIndex < 4; ++PropertyIndex )
					{
						ConstrainPropertyValues( PropertyIndex );
					}
				}

				UpdateMarginTextFromProperties();

				MarginEditableTextBox->SetError( FString() );
			}
		}

		if( bError )
		{
			MarginEditableTextBox->SetError( NSLOCTEXT( "UnrealEd", "InvalidMarginText", "Valid Margin formats are:\nUniform Margin; eg. 0.5\nHorizontal / Vertical Margins; eg. 2, 3\nLeft / Top / Right / Bottom Margins; eg. 0.2, 1, 1.5, 3" ) );
		}
	}
}

void FMarginStructCustomization::UpdateMarginTextFromProperties()
{
	float PropertyValues[ 4 ];
	bool bMultipleValues = false;

	for( int32 PropertyIndex = 0; PropertyIndex < 4 && !bMultipleValues; ++PropertyIndex )
	{
		const FPropertyAccess::Result Result = ChildPropertyHandles[ PropertyIndex ]->GetValue( PropertyValues[ PropertyIndex ] );
		if( Result == FPropertyAccess::MultipleValues )
		{
			bMultipleValues = true;
		}
	}

	if( bMultipleValues )
	{
		MarginText = NSLOCTEXT( "PropertyEditor", "MultipleValues", "Multiple Values" ).ToString();
	}
	else
	{
		if( PropertyValues[ 0 ] == PropertyValues[ 1 ] && PropertyValues[ 1 ] == PropertyValues[ 2 ] && PropertyValues[ 2 ] == PropertyValues[ 3 ] )
		{
			// Uniform margin
			MarginText = FString::SanitizeFloat( PropertyValues[ 0 ] );
		}
		else if( PropertyValues[ 0 ] == PropertyValues[ 2 ] && PropertyValues[ 1 ] == PropertyValues[ 3 ] )
		{
			// Horizontal, Vertical margins
			MarginText = FString::SanitizeFloat( PropertyValues[ 0 ] ) + FString( ", " ) + FString::SanitizeFloat( PropertyValues[ 1 ] );
		}
		else
		{
			// Left, Top, Right, Bottom margins
			MarginText = FString::SanitizeFloat( PropertyValues[ 0 ] ) + FString( ", " ) + FString::SanitizeFloat( PropertyValues[ 1 ] ) + FString( ", " ) +
				FString::SanitizeFloat( PropertyValues[ 2 ] ) + FString( ", " ) + FString::SanitizeFloat( PropertyValues[ 3 ] );
		}
	}
}

void FMarginStructCustomization::OnPropertyChanged()
{
	UpdateMarginTextFromProperties();
}

TOptional<float> FMarginStructCustomization::OnGetValue( int32 PropertyIndex ) const
{
	TWeakPtr<IPropertyHandle> WeakHandlePtr = ChildPropertyHandles[ PropertyIndex ];
	float FloatVal = 0.0f;
	if( WeakHandlePtr.Pin()->GetValue( FloatVal ) == FPropertyAccess::Success )
	{
		return TOptional<float>( FloatVal );
	}

	return TOptional<float>();
}

void FMarginStructCustomization::OnBeginSliderMovement()
{
	bIsUsingSlider = true;

	GEditor->BeginTransaction( NSLOCTEXT("FMarginStructCustomization", "SetMarginProperty", "Set Margin Property") );
}

void FMarginStructCustomization::OnEndSliderMovement( float NewValue )
{
	bIsUsingSlider = false;

	GEditor->EndTransaction();
}

void FMarginStructCustomization::OnValueCommitted( float NewValue, ETextCommit::Type CommitType, int32 PropertyIndex )
{
	TWeakPtr<IPropertyHandle> WeakHandlePtr = ChildPropertyHandles[ PropertyIndex ];
	WeakHandlePtr.Pin()->SetValue( NewValue );

	if( bIsMarginUsingUVSpace )
	{
		ConstrainPropertyValues( PropertyIndex );
	}

	UpdateMarginTextFromProperties();
}	

void FMarginStructCustomization::OnValueChanged( float NewValue, int32 PropertyIndex )
{
	if( bIsUsingSlider )
	{
		TWeakPtr<IPropertyHandle> WeakHandlePtr = ChildPropertyHandles[ PropertyIndex ];
		EPropertyValueSetFlags::Type Flags = EPropertyValueSetFlags::InteractiveChange;
		WeakHandlePtr.Pin()->SetValue( NewValue, Flags );

		if( bIsMarginUsingUVSpace )
		{
			ConstrainPropertyValues( PropertyIndex );
		}

		UpdateMarginTextFromProperties();
	}
}

void FMarginStructCustomization::ConstrainPropertyValues( int32 PropertyIndex )
{
	check( bIsMarginUsingUVSpace );
	const int32 PropertyIndex2 = PropertyIndex ^ 2;

	float PropertyValues[ 2 ];
	ChildPropertyHandles[ PropertyIndex ]->GetValue( PropertyValues[ 0 ] );
	TArray<FString> ObjectValues;
	ChildPropertyHandles[ PropertyIndex2 ]->GetPerObjectValues( ObjectValues );

	for( int32 OutputIndex = 0; OutputIndex < ObjectValues.Num(); ++OutputIndex )
	{
		TTypeFromString<float>::FromString( PropertyValues[ 1 ], *ObjectValues[ OutputIndex ] );

		if( PropertyValues[ 0 ] + PropertyValues[ 1 ] > 1.0f )
		{
			PropertyValues[ 1 ] = 1.0f - PropertyValues[ 0 ];
		}

		ChildPropertyHandles[ PropertyIndex ]->SetValue( PropertyValues[ 0 ] );
		ObjectValues[ OutputIndex ] = FString::SanitizeFloat( PropertyValues[ 1 ] );
	}

	ChildPropertyHandles[ PropertyIndex2 ]->SetPerObjectValues( ObjectValues );
}
