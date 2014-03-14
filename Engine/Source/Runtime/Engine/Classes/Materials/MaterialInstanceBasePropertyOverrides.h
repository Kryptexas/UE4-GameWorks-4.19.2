// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MaterialInstanceBasePropertyOverrides.generated.h"

/** Properties from the base material that can be overridden in material instances. */
USTRUCT()
struct ENGINE_API FMaterialInstanceBasePropertyOverrides
{
	GENERATED_USTRUCT_BODY()

	FMaterialInstanceBasePropertyOverrides();
	void Init(const UMaterialInstance& Instance);

	/** Enables override of the opacity mask clip value. */
	UPROPERTY(EditAnywhere, Category=Material)
	bool bOverride_OpacityMaskClipValue;

	/** Enables override of the blend mode. */
	UPROPERTY(EditAnywhere, Category=Material)
	bool bOverride_BlendMode;

	/** Enables override of the lighting model. */
	UPROPERTY(EditAnywhere, Category=Material)
	bool bOverride_LightingModel;

	/** Enables override of the two sided property. */
	UPROPERTY(EditAnywhere, Category=Material)
	bool bOverride_TwoSided;

	/** If BlendMode is BLEND_Masked, the surface is not rendered where OpacityMask < OpacityMaskClipValue. */
	UPROPERTY(EditAnywhere, Category=Material, meta=(editcondition = "bOverride_OpacityMaskClipValue", NoSpinbox=true))
	float OpacityMaskClipValue;

	/** The blend mode */
	UPROPERTY(EditAnywhere, Category=Material, meta=(editcondition = "bOverride_BlendMode"))
	TEnumAsByte<EBlendMode> BlendMode;

	/** The lighting model */
	UPROPERTY(EditAnywhere, Category=Material, meta=(editcondition = "bOverride_LightingModel"))
	TEnumAsByte<EMaterialLightingModel> LightingModel;	
	
	/** If the material is two sided or not. */
	UPROPERTY(EditAnywhere, Category=Material, meta=(editcondition = "bOverride_TwoSided"))
	uint32 TwoSided:1;

	void UpdateHash(FSHA1& HashState) const;

	/** Updates the values of this set of overrides. Returns true if this should cause a recompile of the material, false otherwise. */ 
	bool Update(const FMaterialInstanceBasePropertyOverrides& Updated);

	bool operator==(const FMaterialInstanceBasePropertyOverrides& Other)const;
	bool operator!=(const FMaterialInstanceBasePropertyOverrides& Other)const;

	friend FArchive& operator<<(FArchive& Ar, FMaterialInstanceBasePropertyOverrides& Ref)
	{
		Ar	<< Ref.bOverride_OpacityMaskClipValue << Ref.OpacityMaskClipValue;

		if( Ar.UE4Ver() >= VER_UE4_MATERIAL_INSTANCE_BASE_PROPERTY_OVERRIDES_PHASE_2 )
		{
			Ar	<< Ref.bOverride_BlendMode << Ref.BlendMode
				<< Ref.bOverride_LightingModel << Ref.LightingModel
				<< Ref.bOverride_TwoSided;

			bool bTwoSided = Ref.TwoSided != 0;
			Ar << bTwoSided;
			if( Ar.IsLoading() )
			{
				Ref.TwoSided = bTwoSided;
			}
		}

		return Ar;
	}
};