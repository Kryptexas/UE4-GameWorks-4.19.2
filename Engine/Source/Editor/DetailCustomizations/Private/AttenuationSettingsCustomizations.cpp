// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "AttenuationSettingsCustomizations.h"

TSharedRef<IStructCustomization> FAttenuationSettingsCustomization::MakeInstance() 
{
	return MakeShareable( new FAttenuationSettingsCustomization );
}

void FAttenuationSettingsCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	// We'll set up reset to default ourselves
	const bool bDisplayResetToDefault = false;
	const FString DisplayNameOverride = TEXT("");

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget( DisplayNameOverride, bDisplayResetToDefault )
		];
}

void FAttenuationSettingsCustomization::CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// We'll set up reset to default ourselves
	const bool bDisplayResetToDefault = false;
	const FString DisplayNameOverride = TEXT("");

	AttenuationShapeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, AttenuationShape));
	DistanceAlgorithmHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, DistanceAlgorithm));

	TSharedRef<IPropertyHandle> AttenuationExtentsHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, AttenuationShapeExtents)).ToSharedRef();

	uint32 NumExtentChildren;
	AttenuationExtentsHandle->GetNumChildren( NumExtentChildren );

	TSharedPtr< IPropertyHandle > ExtentXHandle;
	TSharedPtr< IPropertyHandle > ExtentYHandle;
	TSharedPtr< IPropertyHandle > ExtentZHandle;

	for( uint32 ExtentChildIndex = 0; ExtentChildIndex < NumExtentChildren; ++ExtentChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = AttenuationExtentsHandle->GetChildHandle( ExtentChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, X))
		{
			ExtentXHandle = ChildHandle;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Y))
		{
			ExtentYHandle = ChildHandle;
		}
		else
		{
			check(PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Z));
			ExtentZHandle = ChildHandle;
		}
	}

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, bAttenuate)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, bSpatialize)).ToSharedRef());
	ChildBuilder.AddChildProperty( DistanceAlgorithmHandle.ToSharedRef() );
	
	IDetailPropertyRow& dbAttenuationRow = ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, dBAttenuationAtMax)).ToSharedRef());
	dbAttenuationRow.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsNaturalSoundSelected));

	IDetailPropertyRow& AttenuationShapeRow = ChildBuilder.AddChildProperty( AttenuationShapeHandle.ToSharedRef() );
	
	ChildBuilder.AddChildProperty( AttenuationExtentsHandle )
		.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsBoxSelected))
		.DisplayName(TEXT("Extents"));

	ChildBuilder.AddChildContent(TEXT("Radius"))
		.NameContent()
		[
			SNew(STextBlock)
				.Text(NSLOCTEXT("AttenuationSettings", "RadiusLabel", "Radius"))
				.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsSphereSelected));

	ChildBuilder.AddChildContent(TEXT("CapsuleHalfHeight"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("AttenuationSettings", "CapsuleHalfHeightLabel", "Capsule Half Height"))
				.Font(StructCustomizationUtils.GetRegularFont())
			]
		.ValueContent()
			[
				ExtentXHandle->CreatePropertyValueWidget()
			]
		.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsCapsuleSelected));

	ChildBuilder.AddChildContent(TEXT("CapsuleRadius"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "CapsuleRadiusLabel", "Capsule Radius"))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentYHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsCapsuleSelected));

	ChildBuilder.AddChildContent(TEXT("ConeRadius"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeRadiusLabel", "Cone Radius"))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildContent(TEXT("ConeAngle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeAngleLabel", "Cone Angle"))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentYHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildContent(TEXT("ConeFalloffAngle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeFalloffAngleLabel", "Cone Falloff Angle"))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentZHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsConeSelected));

	IDetailPropertyRow& ConeOffsetRow = ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, ConeOffset)).ToSharedRef());
	ConeOffsetRow.Visibility(TAttribute<EVisibility>(this, &FAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, FalloffDistance)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, OmniRadius)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, bAttenuateWithLPF)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, LPFRadiusMin)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAttenuationSettings, LPFRadiusMax)).ToSharedRef());

	if (PropertyHandles.Num() != 12)
	{
		FString PropertyList;
		for (auto It(PropertyHandles.CreateConstIterator()); It; ++It)
		{
			PropertyList += It.Key().ToString() + TEXT(", ");
		}
		ensureMsgf(false, TEXT("Unexpected property handle(s) customizing FAttenuationSettings: %s"), *PropertyList);
	}
}

EVisibility FAttenuationSettingsCustomization::IsConeSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Cone ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAttenuationSettingsCustomization::IsSphereSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Sphere ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAttenuationSettingsCustomization::IsBoxSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Box ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAttenuationSettingsCustomization::IsCapsuleSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Capsule ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAttenuationSettingsCustomization::IsNaturalSoundSelected() const
{
	uint8 DistanceAlgorithmValue;
	DistanceAlgorithmHandle->GetValue(DistanceAlgorithmValue);

	const ESoundDistanceModel DistanceAlgorithm = (ESoundDistanceModel)DistanceAlgorithmValue;

	return (DistanceAlgorithm == ATTENUATION_NaturalSound ? EVisibility::Visible : EVisibility::Hidden);
}