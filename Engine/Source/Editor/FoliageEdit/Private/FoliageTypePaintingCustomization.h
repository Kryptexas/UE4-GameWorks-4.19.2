// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

class FEdModeFoliage;
class IDetailCategoryBuilder;
class IDetailPropertyRow;

/////////////////////////////////////////////////////
// FFoliageTypePaintingCustomization
class FFoliageTypePaintingCustomization : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance(FEdModeFoliage* InFoliageEditMode);

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface
private:
	FFoliageTypePaintingCustomization(FEdModeFoliage* InFoliageEditMode);
	
	EVisibility GetAlignMaxAngleVisibility() const;
	EVisibility GetScaleXVisibility() const;
	EVisibility GetScaleYVisibility() const;
	EVisibility GetScaleZVisibility() const;
	EVisibility GetCollisionScaleVisibility() const;
	EVisibility GetVertexColorMaskDetailsVisibility() const;
	
	EVisibility GetReapplyModeVisibility() const;
	ECheckBoxState GetReapplyPropertyState(TSharedPtr<IPropertyHandle> ReapplyProperty) const;
	void OnReapplyPropertyStateChanged(ECheckBoxState CheckState, TSharedPtr<IPropertyHandle> ReapplyProperty);
	bool IsReapplyPropertyEnabled(TSharedPtr<IPropertyHandle> ReapplyProperty) const;

	EVisibility GetDensityVisibility() const;
	EVisibility GetReapplyDensityAmountVisibility() const;

	IDetailPropertyRow& AddFoliageProperty(
		IDetailCategoryBuilder& Category,
		TSharedPtr<IPropertyHandle>& Property, 
		TSharedPtr<IPropertyHandle>& ReapplyProperty,
		const TAttribute<EVisibility>& InVisibility,
		const TAttribute<bool>& InEnabled);

	static void HideFoliageCategory(IDetailLayoutBuilder& DetailLayout,	FName CategoryName);
	
private:
	/** Pointer to the foliage edit mode. */
	FEdModeFoliage* FoliageEditMode;

	TSharedPtr<IPropertyHandle> Density;
	TSharedPtr<IPropertyHandle> Radius;
	TSharedPtr<IPropertyHandle> AlignToNormal;
	TSharedPtr<IPropertyHandle> AlignMaxAngle;
	TSharedPtr<IPropertyHandle> RandomYaw;
	TSharedPtr<IPropertyHandle> Scaling;
	TSharedPtr<IPropertyHandle> ScaleX;
	TSharedPtr<IPropertyHandle> ScaleY;
	TSharedPtr<IPropertyHandle> ScaleZ;
	TSharedPtr<IPropertyHandle> ZOffset;
	TSharedPtr<IPropertyHandle> RandomPitchAngle;
	TSharedPtr<IPropertyHandle> GroundSlopeAngle;
	TSharedPtr<IPropertyHandle> Height;
	TSharedPtr<IPropertyHandle>	LandscapeLayers;
	TSharedPtr<IPropertyHandle>	MinimumLayerWeight;
	TSharedPtr<IPropertyHandle> CollisionWithWorld;
	TSharedPtr<IPropertyHandle> CollisionScale;
	TSharedPtr<IPropertyHandle> VertexColorMask;
	TSharedPtr<IPropertyHandle> VertexColorMaskInvert;
	TSharedPtr<IPropertyHandle> VertexColorMaskThreshold;

	//
	TSharedPtr<IPropertyHandle> ReapplyDensity;
	TSharedPtr<IPropertyHandle> ReapplyDensityAmount;
	TSharedPtr<IPropertyHandle> ReapplyRadius;
	TSharedPtr<IPropertyHandle> ReapplyAlignToNormal;
	TSharedPtr<IPropertyHandle> ReapplyRandomYaw;
	TSharedPtr<IPropertyHandle> ReapplyScaleX;
	TSharedPtr<IPropertyHandle> ReapplyScaleY;
	TSharedPtr<IPropertyHandle> ReapplyScaleZ;
	TSharedPtr<IPropertyHandle> ReapplyRandomPitchAngle;
	TSharedPtr<IPropertyHandle> ReapplyGroundSlopeAngle;
	TSharedPtr<IPropertyHandle> ReapplyHeight;
	TSharedPtr<IPropertyHandle> ReapplyLandscapeLayer;
	TSharedPtr<IPropertyHandle> ReapplyZOffset;
	TSharedPtr<IPropertyHandle> ReapplyCollisionWithWorld;
	TSharedPtr<IPropertyHandle> ReapplyVertexColorMask;
};
