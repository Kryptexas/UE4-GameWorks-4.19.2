// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FLightMap;
class FShadowMap;
class FSceneViewStateInterface;

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
 * Class used to reference an FSceneViewStateInterface that allows destruction and recreation of all FSceneViewStateInterface's when needed. 
 * This is used to support reloading the renderer module on the fly.
 */
class FSceneViewStateReference
{
public:
	FSceneViewStateReference() :
		Reference(NULL)
	{}

	ENGINE_API virtual ~FSceneViewStateReference();

	/** Allocates the Scene view state. */
	ENGINE_API void Allocate();

	/** Destorys the Scene view state. */
	ENGINE_API void Destroy();

	/** Destroys all view states, but does not remove them from the linked list. */
	ENGINE_API static void DestroyAll();

	/** Recreates all view states in the global list. */
	ENGINE_API static void AllocateAll();

	FSceneViewStateInterface* GetReference()
	{
		return Reference;
	}

private:
	FSceneViewStateInterface* Reference;
	TLinkedList<FSceneViewStateReference*> GlobalListLink;

	static TLinkedList<FSceneViewStateReference*>*& GetSceneViewStateList();
};

/** different light component types */
enum ELightComponentType
{
	LightType_Directional = 0,
	LightType_Point,
	LightType_Spot,
	LightType_MAX,
	LightType_NumBits = 2
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

enum EShadowMapInteractionType
{
	SMIT_None = 0,
	SMIT_Texture = 2,

	SMIT_NumBits = 3
};

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