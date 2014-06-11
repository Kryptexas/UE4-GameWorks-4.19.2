// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FLightMap;
class FShadowMap;

/** 
 * Class used to identify UPrimitiveComponents on the rendering thread without having to pass the pointer around, 
 * Which would make it easy for people to access game thread variables from the rendering thread.
 */
class FPrimitiveComponentId
{
public:

	FPrimitiveComponentId() : Value(0)
	{}

	inline bool IsValid() const
	{
		return Value > 0;
	}

	inline bool operator==(FPrimitiveComponentId OtherId) const
	{
		return Value == OtherId.Value;
	}

	friend uint32 GetTypeHash( FPrimitiveComponentId Id )
	{
		return GetTypeHash(Id.Value);
	}

	uint64 Value;
};

/**
 * The types of interactions between a light and a primitive.
 */
enum ELightMapInteractionType
{
	LMIT_None	= 0,
	LMIT_Texture= 2,

	LMIT_NumBits= 3
};

/** A reference to a light-map. */
typedef TRefCountPtr<FLightMap> FLightMapRef;

enum EShadowMapInteractionType
{
	SMIT_None = 0,
	SMIT_Texture = 2,

	SMIT_NumBits = 3
};

/** A reference to a shadow-map. */
typedef TRefCountPtr<FShadowMap> FShadowMapRef;

/** Quality levels that a material can be compiled for. */
namespace EMaterialQualityLevel
{
	enum Type
	{
		Low,
		High,
		Num
	};
}

//
//	EMaterialProperty
//

enum EMaterialProperty
{
	MP_EmissiveColor = 0,
	MP_Opacity,
	MP_OpacityMask,
	MP_DiffuseColor,
	MP_SpecularColor,
	MP_BaseColor,
	MP_Metallic,
	MP_Specular,
	MP_Roughness,
	MP_Normal,
	MP_WorldPositionOffset,
	MP_WorldDisplacement,
	MP_TessellationMultiplier,
	MP_SubsurfaceColor,
	MP_AmbientOcclusion,
	MP_Refraction,
	MP_CustomizedUVs0,
	MP_CustomizedUVs1,
	MP_CustomizedUVs2,
	MP_CustomizedUVs3,
	MP_CustomizedUVs4,
	MP_CustomizedUVs5,
	MP_CustomizedUVs6,
	MP_CustomizedUVs7,
	MP_MaterialAttributes,
	MP_MAX,
};