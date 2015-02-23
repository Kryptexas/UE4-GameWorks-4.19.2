// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "FoliageEditModule.h"
#include "PropertyEditing.h"
#include "FoliageType.h"
#include "FoliageEdMode.h"
#include "FoliageTypePaintingCustomization.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

/////////////////////////////////////////////////////
// FFoliageTypePaintingCustomization 
TSharedRef<IDetailCustomization> FFoliageTypePaintingCustomization::MakeInstance(FEdModeFoliage* InFoliageEditMode)
{
	TSharedRef<FFoliageTypePaintingCustomization> Instance = MakeShareable(new FFoliageTypePaintingCustomization(InFoliageEditMode));
	return Instance;
}

FFoliageTypePaintingCustomization::FFoliageTypePaintingCustomization(FEdModeFoliage* InFoliageEditMode)
{
	FoliageEditMode = InFoliageEditMode;
}

void FFoliageTypePaintingCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayoutBuilder)
{
	// Hide categories we are not going to customize
	const static FName PaintingName("Painting");
	const static FName PlacementName("Placement");
	const static FName ProceduralName("Procedural");
	HideFoliageCategory(DetailLayoutBuilder, PaintingName);
	HideFoliageCategory(DetailLayoutBuilder, PlacementName);
	HideFoliageCategory(DetailLayoutBuilder, ProceduralName);
	
	//
	// Re-add customized properties 
	//
		
	// Get the properties we need
	TSharedPtr<IPropertyHandle> InvalidProperty;
	IDetailCategoryBuilder& PaintingCategory = DetailLayoutBuilder.EditCategory("Painting");
	
	Density = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, Density));
	Radius = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, Radius));
	AlignToNormal = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, AlignToNormal));
	AlignMaxAngle = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, AlignMaxAngle));
	RandomYaw = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, RandomYaw));
	Scaling = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, Scaling));
	ScaleX = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleX));
	ScaleY = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleY));
	ScaleZ = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleZ));
	ZOffset = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ZOffset));
	RandomPitchAngle = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, RandomPitchAngle));
	GroundSlopeAngle = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, GroundSlopeAngle));
	Height = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, Height));
	LandscapeLayers = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, LandscapeLayers));
	MinimumLayerWeight = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, MinimumLayerWeight));
	CollisionWithWorld = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, CollisionWithWorld));
	CollisionScale = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, CollisionScale));
	VertexColorMask = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, VertexColorMask));
	VertexColorMaskInvert = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, VertexColorMaskInvert));
	VertexColorMaskThreshold = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, VertexColorMaskThreshold));
	
	//
	ReapplyDensityAmount = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyDensityAmount));
	ReapplyDensity = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyDensity));
	ReapplyRadius = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyRadius));
	ReapplyAlignToNormal = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyAlignToNormal));
	ReapplyRandomYaw = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyRandomYaw));
	ReapplyScaleX = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyScaleX));
	ReapplyScaleY = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyScaleY));
	ReapplyScaleZ = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyScaleZ));
	ReapplyRandomPitchAngle = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyRandomPitchAngle));
	ReapplyGroundSlopeAngle = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyGroundSlope));
	ReapplyHeight = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyHeight));
	ReapplyLandscapeLayer = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyLandscapeLayer));
	ReapplyZOffset = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyZOffset));
	ReapplyCollisionWithWorld = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyCollisionWithWorld));
	ReapplyVertexColorMask = DetailLayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, ReapplyVertexColorMask));
	
	// Density
	// TODO: Reapply density
	AddFoliageProperty(PaintingCategory, Density, InvalidProperty, 
		TAttribute<EVisibility>(), 
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyDensity)));

	// Radius
	AddFoliageProperty(PaintingCategory, Radius, ReapplyRadius, TAttribute<EVisibility>(), TAttribute<bool>());

	// Align to normal
	AddFoliageProperty(PaintingCategory, AlignToNormal, ReapplyAlignToNormal, TAttribute<EVisibility>(), TAttribute<bool>());
	AddFoliageProperty(PaintingCategory, AlignMaxAngle, InvalidProperty,
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetAlignMaxAngleVisibility)),
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyAlignToNormal))); 
		
	// Random Yaw
	AddFoliageProperty(PaintingCategory, RandomYaw, ReapplyRandomYaw, TAttribute<EVisibility>(), TAttribute<bool>());
		
	// Scaling
	AddFoliageProperty(PaintingCategory, Scaling, InvalidProperty, TAttribute<EVisibility>(), TAttribute<bool>());
	
	AddFoliageProperty(PaintingCategory, ScaleX, ReapplyScaleX, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetScaleXVisibility)), TAttribute<bool>());
	
	AddFoliageProperty(PaintingCategory, ScaleY, ReapplyScaleY, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetScaleYVisibility)), TAttribute<bool>());
	
	AddFoliageProperty(PaintingCategory, ScaleZ, ReapplyScaleZ, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetScaleZVisibility)), TAttribute<bool>());

	// ZOffset
	AddFoliageProperty(PaintingCategory, ZOffset, ReapplyZOffset, TAttribute<EVisibility>(), TAttribute<bool>());
	
	// Random pitch
	AddFoliageProperty(PaintingCategory, RandomPitchAngle, ReapplyRandomPitchAngle, TAttribute<EVisibility>(), TAttribute<bool>());

	// Ground slope
	AddFoliageProperty(PaintingCategory, GroundSlopeAngle, ReapplyGroundSlopeAngle, TAttribute<EVisibility>(), TAttribute<bool>());

	// Height
	AddFoliageProperty(PaintingCategory, Height, ReapplyHeight, TAttribute<EVisibility>(), TAttribute<bool>());

	// Landscape layers
	AddFoliageProperty(PaintingCategory, LandscapeLayers, ReapplyLandscapeLayer, TAttribute<EVisibility>(), TAttribute<bool>());
	AddFoliageProperty(PaintingCategory, MinimumLayerWeight, InvalidProperty, 
		TAttribute<EVisibility>(), 
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyLandscapeLayer))); 

	// Collision with world
	AddFoliageProperty(PaintingCategory, CollisionWithWorld, ReapplyCollisionWithWorld, TAttribute<EVisibility>(), TAttribute<bool>());
	AddFoliageProperty(PaintingCategory, CollisionScale, InvalidProperty, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetCollisionScaleVisibility)),
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyCollisionWithWorld))); 

	// Vertex color mask
	AddFoliageProperty(PaintingCategory, VertexColorMask, ReapplyVertexColorMask, TAttribute<EVisibility>(), TAttribute<bool>());
	
	AddFoliageProperty(PaintingCategory, VertexColorMaskInvert, InvalidProperty, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetVertexColorMaskDetailsVisibility)),
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyVertexColorMask))); 
	
	AddFoliageProperty(PaintingCategory, VertexColorMaskThreshold, InvalidProperty, 
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::GetVertexColorMaskDetailsVisibility)), 
		TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyVertexColorMask))); 
}

IDetailPropertyRow& FFoliageTypePaintingCustomization::AddFoliageProperty(
		IDetailCategoryBuilder& Category,
		TSharedPtr<IPropertyHandle>& Property, 
		TSharedPtr<IPropertyHandle>& ReapplyProperty,
		const TAttribute<EVisibility>& InVisibility,
		const TAttribute<bool>& InEnabled)
{
	IDetailPropertyRow& PropertyRow = Category.AddProperty(Property);

	if (ReapplyProperty.IsValid())
	{
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FDetailWidgetRow	Row;
		PropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

		auto IsEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled, ReapplyProperty));
		ValueWidget->SetEnabled(IsEnabled);
		NameWidget->SetEnabled(IsEnabled);

		PropertyRow.CustomWidget()
		.NameContent()
		.MinDesiredWidth(Row.NameWidget.MinWidth)
		.MaxDesiredWidth(Row.NameWidget.MaxWidth)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FFoliageTypePaintingCustomization::GetReapplyPropertyState, ReapplyProperty)
				.OnCheckStateChanged(this, &FFoliageTypePaintingCustomization::OnReapplyPropertyStateChanged, ReapplyProperty)
				.IsChecked(this, &FFoliageTypePaintingCustomization::GetReapplyPropertyState, ReapplyProperty)
				.Visibility(this, &FFoliageTypePaintingCustomization::GetReapplyModeVisibility)
				.ToolTipText(ReapplyProperty->GetToolTipText())
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				NameWidget.ToSharedRef()
			]
		]
		.ValueContent()
		.MinDesiredWidth(Row.ValueWidget.MinWidth)
		.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
		[
			ValueWidget.ToSharedRef()
		];
	}
	else 
	{
		if (InEnabled.IsSet())
		{
			PropertyRow.IsEnabled(InEnabled);
		}
	}

	if (InVisibility.IsSet())
	{
		PropertyRow.Visibility(InVisibility);
	}
	
	//
	return PropertyRow;
}

void FFoliageTypePaintingCustomization::HideFoliageCategory(IDetailLayoutBuilder& DetailLayoutBuilder, FName CategoryName)
{
	TArray<TSharedRef<IPropertyHandle>> CategoryProperties;
	DetailLayoutBuilder.EditCategory(CategoryName).GetDefaultProperties(CategoryProperties, true, true);

	for (auto& PropertyHandle : CategoryProperties)
	{
		DetailLayoutBuilder.HideProperty(PropertyHandle);
	}
}

EVisibility FFoliageTypePaintingCustomization::GetAlignMaxAngleVisibility() const
{
	bool bAlightToNormal;
	if (AlignToNormal->GetValue(bAlightToNormal) == FPropertyAccess::Success)
	{
		return bAlightToNormal ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetScaleXVisibility() const
{
	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetScaleYVisibility() const
{
	uint8 ScalingValue;
	if (Scaling->GetValue(ScalingValue) == FPropertyAccess::Success)
	{
		return (ScalingValue == (uint8)EFoliageScaling::Uniform) ? EVisibility::Collapsed : EVisibility::Visible;
	}
	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetScaleZVisibility() const
{
	uint8 ScalingValue;
	if (Scaling->GetValue(ScalingValue) == FPropertyAccess::Success)
	{
		return (ScalingValue == (uint8)EFoliageScaling::Uniform) ? EVisibility::Collapsed : EVisibility::Visible;
	}
	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetCollisionScaleVisibility() const
{
	bool bCollisionWithWorld = false;
	if (CollisionWithWorld->GetValue(bCollisionWithWorld) == FPropertyAccess::Success)
	{
		return bCollisionWithWorld ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetVertexColorMaskDetailsVisibility() const
{
	uint8 ColorMask;
	if (VertexColorMask->GetValue(ColorMask) == FPropertyAccess::Success)
	{
		return ColorMask != FOLIAGEVERTEXCOLORMASK_Disabled ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility FFoliageTypePaintingCustomization::GetReapplyModeVisibility() const
{
	return FoliageEditMode->UISettings.GetReapplyToolSelected() ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState FFoliageTypePaintingCustomization::GetReapplyPropertyState(TSharedPtr<IPropertyHandle> ReapplyProperty) const
{
	bool bState;
	if (ReapplyProperty->GetValue(bState) == FPropertyAccess::Success)
	{
		return bState ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
		
	return ECheckBoxState::Undetermined;
}

void FFoliageTypePaintingCustomization::OnReapplyPropertyStateChanged(ECheckBoxState CheckState, TSharedPtr<IPropertyHandle> ReapplyProperty)
{
	if (CheckState != ECheckBoxState::Undetermined)
	{
		ReapplyProperty->SetValue(CheckState == ECheckBoxState::Checked);
	}
}

bool FFoliageTypePaintingCustomization::IsReapplyPropertyEnabled(TSharedPtr<IPropertyHandle> ReapplyProperty) const
{
	if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		bool bState;
		if (ReapplyProperty->GetValue(bState) == FPropertyAccess::Success)
		{
			return bState;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE