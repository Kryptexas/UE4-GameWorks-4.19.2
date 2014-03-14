// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "MathStructProxyCustomizations.h"

void FMathStructProxyCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{

}

void FMathStructProxyCustomization::MakeHeaderRow( TSharedRef<class IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row )
{

}

template<typename ProxyType, typename NumericType>
TSharedRef<SWidget> FMathStructProxyCustomization::MakeNumericProxyWidget(TSharedRef<IPropertyHandle>& StructPropertyHandle, TSharedRef< TProxyProperty<ProxyType, NumericType> >& ProxyValue, const FText& Label, const FLinearColor& LabelColor, const FLinearColor& LabelBackgroundColor)
{
	TWeakPtr<IPropertyHandle> WeakHandlePtr = StructPropertyHandle;

	return 
		SNew( SNumericEntryBox<NumericType> )
		.Value( this, &FMathStructProxyCustomization::OnGetValue<ProxyType, NumericType>, WeakHandlePtr, ProxyValue )
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.UndeterminedString( NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values").ToString() )
		.OnValueCommitted( this, &FMathStructProxyCustomization::OnValueCommitted<ProxyType, NumericType>, WeakHandlePtr, ProxyValue )
		.OnValueChanged( this, &FMathStructProxyCustomization::OnValueChanged<ProxyType, NumericType>, WeakHandlePtr, ProxyValue )
		.OnBeginSliderMovement( this, &FMathStructProxyCustomization::OnBeginSliderMovement )
		.OnEndSliderMovement( this, &FMathStructProxyCustomization::OnEndSliderMovement<NumericType> )
		.LabelVAlign(VAlign_Fill)
		.LabelPadding(0)
		// Only allow spin on handles with one object.  Otherwise it is not clear what value to spin
		.AllowSpin( StructPropertyHandle->GetNumOuterObjects() == 1 )
		.MinValue(TOptional<NumericType>())
		.MaxValue(TOptional<NumericType>())
		.MaxSliderValue(TOptional<NumericType>())
		.MinSliderValue(TOptional<NumericType>())
		.Label()
		[
			SNumericEntryBox<float>::BuildLabel( Label, LabelColor, LabelBackgroundColor )
		];
}


template<typename ProxyType, typename NumericType>
TOptional<NumericType> FMathStructProxyCustomization::OnGetValue( TWeakPtr<IPropertyHandle> WeakHandlePtr, TSharedRef< TProxyProperty<ProxyType, NumericType> > ProxyValue ) const
{
	if(CacheValues(WeakHandlePtr))
	{
		return ProxyValue->Get();
	}
	return TOptional<NumericType>();
}

template<typename ProxyType, typename NumericType>
void FMathStructProxyCustomization::OnValueCommitted( NumericType NewValue, ETextCommit::Type CommitType, TWeakPtr<IPropertyHandle> WeakHandlePtr, TSharedRef< TProxyProperty<ProxyType, NumericType> > ProxyValue )
{
	ProxyValue->Set(NewValue);
	FlushValues(WeakHandlePtr);
}	

template<typename ProxyType, typename NumericType>
void FMathStructProxyCustomization::OnValueChanged( NumericType NewValue, TWeakPtr<IPropertyHandle> WeakHandlePtr, TSharedRef< TProxyProperty<ProxyType, NumericType> > ProxyValue )
{
	if( bIsUsingSlider )
	{
		ProxyValue->Set(NewValue);
		FlushValues(WeakHandlePtr);
	}
}

void FMathStructProxyCustomization::OnBeginSliderMovement()
{
	bIsUsingSlider = true;

	GEditor->BeginTransaction( NSLOCTEXT("FMathStructCustomization", "SetVectorProperty", "Set Vector Property") );
}

template<typename NumericType>
void FMathStructProxyCustomization::OnEndSliderMovement( NumericType NewValue )
{
	bIsUsingSlider = false;

	GEditor->EndTransaction();
}


TSharedRef<IStructCustomization> FMatrixStructCustomization::MakeInstance() 
{
	return MakeShareable( new FMatrixStructCustomization );
}

void FMatrixStructCustomization::MakeHeaderRow( TSharedRef<class IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row )
{
#define LOCTEXT_NAMESPACE "MatrixStructCustomization"

	// We'll set up reset to default ourselves
	const bool bDisplayResetToDefault = false;

	Row.NameContent()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
		[
			StructPropertyHandle->CreatePropertyNameWidget( LOCTEXT("LocationLabel", "Location").ToString(), bDisplayResetToDefault )
		]
		+SVerticalBox::Slot()
		.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
		[
			StructPropertyHandle->CreatePropertyNameWidget( LOCTEXT("RotationLabel", "Rotation").ToString(), bDisplayResetToDefault )
		]
		+SVerticalBox::Slot()
		.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
		[
			StructPropertyHandle->CreatePropertyNameWidget( LOCTEXT("ScaleLabel", "Scale").ToString(), bDisplayResetToDefault )
		]
	]
	.ValueContent()
	// Make enough space for each child handle
	.MinDesiredWidth(125.0f * SortedChildHandles.Num() )
	.MaxDesiredWidth(125.0f * SortedChildHandles.Num() )
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedTranslationX, LOCTEXT("TranslationX", "X"), FLinearColor::White, SNumericEntryBox<float>::RedLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedTranslationY, LOCTEXT("TranslationY", "Y"), FLinearColor::White, SNumericEntryBox<float>::GreenLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedTranslationZ, LOCTEXT("TranslationZ", "Z"), FLinearColor::White, SNumericEntryBox<float>::BlueLabelBackgroundColor)
			]
		]
		+SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FRotator, float>(StructPropertyHandle, CachedRotationYaw, LOCTEXT("RotationYaw", "X"), FLinearColor::White, SNumericEntryBox<float>::RedLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FRotator, float>(StructPropertyHandle, CachedRotationPitch, LOCTEXT("RotationPitch", "Y"), FLinearColor::White, SNumericEntryBox<float>::GreenLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FRotator, float>(StructPropertyHandle, CachedRotationRoll, LOCTEXT("RotationRoll", "Z"), FLinearColor::White, SNumericEntryBox<float>::BlueLabelBackgroundColor)
			]
		]
		+SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedScaleX, LOCTEXT("ScaleX", "X"), FLinearColor::White, SNumericEntryBox<float>::RedLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 3.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedScaleY, LOCTEXT("ScaleY", "Y"), FLinearColor::White, SNumericEntryBox<float>::GreenLabelBackgroundColor)
			]
			+SHorizontalBox::Slot()
			.Padding( FMargin(0.0f, 2.0f, 0.0f, 2.0f ) )
			[
				MakeNumericProxyWidget<FVector, float>(StructPropertyHandle, CachedScaleZ, LOCTEXT("ScaleZ", "Z"), FLinearColor::White, SNumericEntryBox<float>::BlueLabelBackgroundColor)
			]
		]
	];
#undef LOCTEXT_NAMESPACE
}

bool FMatrixStructCustomization::CacheValues( TWeakPtr<IPropertyHandle> WeakHandlePtr ) const
{
	check(WeakHandlePtr.IsValid());

	TArray<void*> RawData;
	WeakHandlePtr.Pin()->AccessRawData(RawData);

	if (RawData.Num() == 1)
	{
		FMatrix* MatrixValue = reinterpret_cast<FMatrix*>(RawData[0]);
		if(MatrixValue != NULL)
		{
			CachedTranslation->Set(MatrixValue->GetOrigin());
			CachedRotation->Set(MatrixValue->Rotator());
			CachedScale->Set(MatrixValue->GetScaleVector());
			return true;
		}
	}

	return false;
}

bool FMatrixStructCustomization::FlushValues( TWeakPtr<IPropertyHandle> WeakHandlePtr ) const
{
	check(WeakHandlePtr.IsValid());

	TArray<void*> RawData;
	WeakHandlePtr.Pin()->AccessRawData(RawData);

	WeakHandlePtr.Pin()->NotifyPreChange();
	for(int32 ValueIndex = 0; ValueIndex < RawData.Num(); ValueIndex++)
	{
		FMatrix* MatrixValue = reinterpret_cast<FMatrix*>(RawData[ValueIndex]);
		if(MatrixValue != NULL)
		{
			const FRotator CurrentRotation = MatrixValue->Rotator();
			const FVector CurrentTranslation = MatrixValue->GetOrigin();
			const FVector CurrentScale = MatrixValue->GetScaleVector();

			FRotator Rotation(
				CachedRotationPitch->IsSet() ? CachedRotationPitch->Get() : CurrentRotation.Pitch,
				CachedRotationYaw->IsSet() ? CachedRotationYaw->Get() : CurrentRotation.Yaw,
				CachedRotationRoll->IsSet() ? CachedRotationRoll->Get() : CurrentRotation.Roll
				);
			FVector Translation(
				CachedTranslationX->IsSet() ? CachedTranslationX->Get() : CurrentTranslation.X,
				CachedTranslationY->IsSet() ? CachedTranslationY->Get() : CurrentTranslation.Y,
				CachedTranslationZ->IsSet() ? CachedTranslationZ->Get() : CurrentTranslation.Z
				);
			FVector Scale(
				CachedScaleX->IsSet() ? CachedScaleX->Get() : CurrentScale.X,
				CachedScaleY->IsSet() ? CachedScaleY->Get() : CurrentScale.Y,
				CachedScaleZ->IsSet() ? CachedScaleZ->Get() : CurrentScale.Z
				);
			*MatrixValue = FScaleRotationTranslationMatrix(Scale, Rotation, Translation);
		}
	}
	WeakHandlePtr.Pin()->NotifyPostChange();

	return true;
}

TSharedRef<IStructCustomization> FTransformStructCustomization::MakeInstance() 
{
	return MakeShareable( new FTransformStructCustomization );
}

bool FTransformStructCustomization::CacheValues( TWeakPtr<IPropertyHandle> WeakHandlePtr ) const
{
	check(WeakHandlePtr.IsValid());

	TArray<void*> RawData;
	WeakHandlePtr.Pin()->AccessRawData(RawData);

	if (RawData.Num() == 1)
	{
		FTransform* TransformValue = reinterpret_cast<FTransform*>(RawData[0]);
		if(TransformValue != NULL)
		{
			CachedTranslation->Set(TransformValue->GetTranslation());
			CachedRotation->Set(TransformValue->GetRotation().Rotator());
			CachedScale->Set(TransformValue->GetScale3D());
			return true;
		}
	}

	return false;
}

bool FTransformStructCustomization::FlushValues( TWeakPtr<IPropertyHandle> WeakHandlePtr ) const
{
	check(WeakHandlePtr.IsValid());

	TArray<void*> RawData;
	WeakHandlePtr.Pin()->AccessRawData(RawData);

	WeakHandlePtr.Pin()->NotifyPreChange();
	for(int32 ValueIndex = 0; ValueIndex < RawData.Num(); ValueIndex++)
	{
		FTransform* TransformValue = reinterpret_cast<FTransform*>(RawData[0]);
		if(TransformValue != NULL)
		{
			const FRotator CurrentRotation = TransformValue->GetRotation().Rotator();
			const FVector CurrentTranslation = TransformValue->GetTranslation();
			const FVector CurrentScale = TransformValue->GetScale3D();

			FRotator Rotation(
				CachedRotationPitch->IsSet() ? CachedRotationPitch->Get() : CurrentRotation.Pitch,
				CachedRotationYaw->IsSet() ? CachedRotationYaw->Get() : CurrentRotation.Yaw,
				CachedRotationRoll->IsSet() ? CachedRotationRoll->Get() : CurrentRotation.Roll
				);
			FVector Translation(
				CachedTranslationX->IsSet() ? CachedTranslationX->Get() : CurrentTranslation.X,
				CachedTranslationY->IsSet() ? CachedTranslationY->Get() : CurrentTranslation.Y,
				CachedTranslationZ->IsSet() ? CachedTranslationZ->Get() : CurrentTranslation.Z
				);
			FVector Scale(
				CachedScaleX->IsSet() ? CachedScaleX->Get() : CurrentScale.X,
				CachedScaleY->IsSet() ? CachedScaleY->Get() : CurrentScale.Y,
				CachedScaleZ->IsSet() ? CachedScaleZ->Get() : CurrentScale.Z
				);
			TransformValue->SetComponents(Rotation.Quaternion(), Translation, Scale);
		}
	}
	WeakHandlePtr.Pin()->NotifyPostChange();

	return true;
}