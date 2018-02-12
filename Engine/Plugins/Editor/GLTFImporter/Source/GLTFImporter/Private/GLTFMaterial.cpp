// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GLTFMaterial.h"
#include "GLTFPackage.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"

using namespace GLTF;

static TextureFilter ConvertFilter(FSampler::EFilter Filter)
{
	switch (Filter)
	{
	case FSampler::EFilter::Nearest: return TF_Nearest;
	case FSampler::EFilter::LinearMipmapNearest: return TF_Bilinear;
	case FSampler::EFilter::LinearMipmapLinear: return TF_Trilinear;
	// Other glTF filter values have no direct correlation to Unreal
	default: return TF_Default;
	}
}

static TextureAddress ConvertWrap(FSampler::EWrap Wrap)
{
	switch (Wrap)
	{
	case FSampler::EWrap::Repeat: return TA_Wrap;
	case FSampler::EWrap::MirroredRepeat: return TA_Mirror;
	case FSampler::EWrap::ClampToEdge: return TA_Clamp;
	default: return TA_Wrap;
	}
}

static EBlendMode ConvertAlphaMode(FMat::EAlphaMode Mode)
{
	switch (Mode)
	{
	case FMat::EAlphaMode::Opaque: return EBlendMode::BLEND_Opaque;
	case FMat::EAlphaMode::Blend: return EBlendMode::BLEND_Translucent;
	case FMat::EAlphaMode::Mask: return EBlendMode::BLEND_Masked;
	default: return EBlendMode::BLEND_Opaque;
	}
}

static const TCHAR* ImageFormatString(FImage::EFormat Format)
{
	switch (Format)
	{
	case FImage::EFormat::PNG: return TEXT("PNG");
	case FImage::EFormat::JPEG: return TEXT("JPEG");
	default: return TEXT("");
	}

	// NOTE: this is passed to UTextureFactory::FactoryCreateBinary
}

static UTexture2D* ImportTexture(UObject* Parent, FName InName, EObjectFlags Flags, int32 Index, const FTex& InTexture)
{
	// based on UnFbx::FFbxImporter::ImportTexture
	// GLTF::FImage does most of the heavy lifting, has already loaded data from file or other source.
	// This function handles creation of UTexture2D from encoded PNG or JPEG data.

	UTexture2D* Texture = nullptr;

	FString AssetName;
	UPackage* AssetPackage = GetAssetPackageAndName<UTexture2D>(Parent, InTexture.GetName(), TEXT("T"), InName, Index, AssetName);

	const TArray<uint8>& ImageData = InTexture.Source.Data;

	if (ImageData.Num() > 0)
	{
		const uint8* ImageDataPtr = ImageData.GetData();
		auto Factory = NewObject<UTextureFactory>();
		Factory->AddToRoot();

		// save texture settings if texture exists
		Factory->SuppressImportOverwriteDialog();

		// Defer compression until save? It's s'posed to decrease iteration times.

		Texture = (UTexture2D*)Factory->FactoryCreateBinary(
			UTexture2D::StaticClass(), AssetPackage, *AssetName,
			Flags, nullptr, ImageFormatString(InTexture.Source.Format),
			ImageDataPtr, ImageDataPtr + ImageData.Num(), GWarn);

		if (Texture != nullptr)
		{
			// --- additional texture properites ---
			Texture->Filter = ConvertFilter(InTexture.Sampler.MinFilter);
			Texture->AddressX = ConvertWrap(InTexture.Sampler.WrapS);
			Texture->AddressY = ConvertWrap(InTexture.Sampler.WrapT);
			// Material that uses this texture will set more properties based on whether
			// it's a normal map, base color, etc. We don't know this texture's usage here.

			// --- book-keeping ---
			// Make sure the AssetImportData points to the texture file and not the glTF file
			Texture->AssetImportData->Update(*InTexture.Source.FilePath);

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(Texture);

			// Set the dirty flag so this package will get saved later
			AssetPackage->SetDirtyFlag(true);
		}

		Factory->RemoveFromRoot();
	}

	return Texture;
}

TArray<UTexture2D*> ImportTextures(const FAsset& Asset, UObject* InParent, FName InName, EObjectFlags Flags)
{
	TArray<UTexture2D*> Textures;
	Textures.Reserve(Asset.Textures.Num());

	for (int32 Index = 0; Index < Asset.Textures.Num(); ++Index)
	{
		const FTex& Tex = Asset.Textures[Index];
		UTexture2D* UnTex = ImportTexture(InParent, InName, Flags, Index, Tex);

		// No nullptr check because we want this array to match 1-to-1 with Asset.Textures
		Textures.Add(UnTex);
	}

	return Textures;
}

static constexpr int32 GridSnap = 16;

static void PlaceOnGrid(UMaterialExpression* Node, int32 X, int32 Y, const FString& Description = FString())
{
	Node->MaterialExpressionEditorX = GridSnap * X;
	Node->MaterialExpressionEditorY = GridSnap * Y;

	if (!Description.IsEmpty())
	{
		Node->Desc = Description;
		Node->bCommentBubbleVisible = true;
	}
}

static void PlaceOnGridNextTo(UMaterialExpression* Node, const UMaterialExpression* ExistingNode)
{
	int32 X = ExistingNode->MaterialExpressionEditorX / GridSnap;
	int32 Y = ExistingNode->MaterialExpressionEditorY / GridSnap;

	PlaceOnGrid(Node, X + 8, Y + 2);
}

static void ConnectInput(UMaterial* Material, FExpressionInput& MaterialInput, UMaterialExpression* Factor, UMaterialExpression* Texture, int32 TextureOutputIndex = 0)
{
	if (Texture && Factor)
	{
		// multiply these to get final input

		// [Factor]---\            |
		//             [Multiply]--| Material
		// [Texture]--/            |

		UMaterialExpressionMultiply* MultiplyNode = NewObject<UMaterialExpressionMultiply>(Material);
		PlaceOnGridNextTo(MultiplyNode, Factor);
		Material->Expressions.Add(MultiplyNode); // necessary?
		MultiplyNode->A.Connect(0, Factor);
		MultiplyNode->B.Connect(TextureOutputIndex, Texture);
		MaterialInput.Connect(0, MultiplyNode);
	}
	else if (Texture)
	{
		MaterialInput.Connect(TextureOutputIndex, Texture);
	}
	else if (Factor)
	{
		MaterialInput.Connect(0, Factor);
	}

	// Add Texture & Factor to Material->Expressions in this func? If so use AddUnique for Texture.
}

static UMaterial* ImportMaterial(UObject* Parent, FName InName, EObjectFlags Flags, int32 Index, const FMat& InMaterial, const TArray<UTexture2D*>& Textures)
{
	FString AssetName;
	UPackage* AssetPackage = GetAssetPackageAndName<UMaterial>(Parent, InMaterial.Name, TEXT("M"), InName, Index, AssetName);

	UMaterial* Material = NewObject<UMaterial>(AssetPackage, UMaterial::StaticClass(), FName(*AssetName), Flags);

	// glTF defines a straightforward material model with Metallic + Roughness PBR.
	// The set of possible expression nodes and their relationships is known, but some may be unused.
	// Build an Unreal material graph that implements this glTF material description.

	// The textures array is not altered, but textures within the array might have properties set based on
	// how they are used by this material.
	// Multiple materials can share textures. Make sure they agree on how shared textures are used.
	// (not required by glTF spec)

	//                          v-- freak out if named texture doesn't exist
	#define GET_TEXTURE(Name) check(InMaterial.Name.TextureIndex != INDEX_NONE); UTexture2D* Texture = Textures[InMaterial.Name.TextureIndex];
	#define SET_OR_VERIFY(Property, Value) Texture->Property = Value;
	//                                        ^-- this simply overwrites already-set values. TODO: verify already-set values.

	#define DECLARE_SAMPLER_NODE(Name) UMaterialExpressionTextureSample* Name##SamplerNode = nullptr;
	#define CREATE_SAMPLER_NODE(Name, Type) { \
		Name##SamplerNode = NewObject<UMaterialExpressionTextureSample>(Material); \
		Name##SamplerNode->Texture = Texture; \
		Name##SamplerNode->SamplerType = SAMPLERTYPE_##Type; \
		Name##SamplerNode->ConstCoordinate = InMaterial.Name.TexCoord; \
		Material->Expressions.Add(Name##SamplerNode); \
		}

	// BaseColor
	// -- texture
	// -- (vec4)factor * texture
	// -- (vec4)factor (as const color value)

	// Color and alpha go to different inputs, so handle factor.rgb and factor.a separately.

	const bool bHasBaseColorTexture = InMaterial.BaseColor.TextureIndex != INDEX_NONE;
	const bool bHasBaseColorFactor = FVector(InMaterial.BaseColorFactor) != FVector(1.0f, 1.0f, 1.0f);

	DECLARE_SAMPLER_NODE(BaseColor)

	if (bHasBaseColorTexture)
	{
		GET_TEXTURE(BaseColor)
		SET_OR_VERIFY(SRGB, true)
		SET_OR_VERIFY(LODGroup, TEXTUREGROUP_World)
		SET_OR_VERIFY(CompressionSettings, TC_Default)

		CREATE_SAMPLER_NODE(BaseColor, Color)
		PlaceOnGrid(BaseColorSamplerNode, -40, -14, "baseColorTexture");
	}

	UMaterialExpressionConstant3Vector* BaseColorFactorNode = nullptr;

	if (bHasBaseColorFactor)
	{
		BaseColorFactorNode = NewObject<UMaterialExpressionConstant3Vector>(Material);
		BaseColorFactorNode->Constant = FLinearColor(InMaterial.BaseColorFactor);
		BaseColorFactorNode->bCollapsed = true;
		PlaceOnGrid(BaseColorFactorNode, -25, -15, "baseColorFactor.rgb");
	}

	ConnectInput(Material, Material->BaseColor, BaseColorFactorNode, BaseColorSamplerNode);

	// Handle alpha (if needed) as part of BaseColor
	if (!InMaterial.IsOpaque())
	{
		UMaterialExpressionTextureSample* AlphaSamplerNode = nullptr;
		UMaterialExpressionConstant* AlphaFactorNode = nullptr;

		const bool bHasAlphaFactor = InMaterial.BaseColorFactor.W != 1.0f;

		if (bHasAlphaFactor)
		{
			AlphaFactorNode = NewObject<UMaterialExpressionConstant>(Material);
			AlphaFactorNode->R = InMaterial.BaseColorFactor.W;
			PlaceOnGrid(AlphaFactorNode, -25, -8, "baseColorFactor.a");
		}

		if (bHasBaseColorTexture)
		{
			GET_TEXTURE(BaseColor)
			if (Texture->HasAlphaChannel())
			{
				AlphaSamplerNode = BaseColorSamplerNode;
			}
		}

		FExpressionInput& Input = (InMaterial.AlphaMode == FMat::EAlphaMode::Mask) ? Material->OpacityMask : Material->Opacity;
		ConnectInput(Material, Input, AlphaFactorNode, AlphaSamplerNode, 4);
	}

	// Normal
	// -- texture
	// -- (float)scale * texture

	const bool bHasNormalTexture = InMaterial.Normal.TextureIndex != INDEX_NONE;

	if (bHasNormalTexture)
	{
		GET_TEXTURE(Normal)
		SET_OR_VERIFY(SRGB, false)
		SET_OR_VERIFY(LODGroup, TEXTUREGROUP_WorldNormalMap)
		SET_OR_VERIFY(CompressionSettings, TC_Normalmap)
		SET_OR_VERIFY(bFlipGreenChannel, false)

		DECLARE_SAMPLER_NODE(Normal)
		CREATE_SAMPLER_NODE(Normal, Normal)
		PlaceOnGrid(NormalSamplerNode, -21, 38, "normalTexture");

		const bool bHasNormalScale = InMaterial.NormalScale != 1.0f;

		UMaterialExpressionConstant* NormalScaleNode = nullptr;

		if (bHasNormalScale)
		{
			NormalScaleNode = NewObject<UMaterialExpressionConstant>(Material);
			NormalScaleNode->R = InMaterial.NormalScale;
			PlaceOnGrid(NormalScaleNode, -18, 32, "normal.scale");
		}

		ConnectInput(Material, Material->Normal, NormalScaleNode, NormalSamplerNode);
	}

	// Metal + Rough are packed in a single texture, with separate scaling factors.
	// Metallic
	// -- texture.B
	// -- (float)factor * texture.B
	// -- (float)factor (as const metallic value)
	// Roughness
	// -- texture.G
	// -- (float)factor * texture.G
	// -- (float)factor (as const roughness value)

	const bool bHasMetallicRoughnessTexture = InMaterial.MetallicRoughness.TextureIndex != INDEX_NONE;
	const bool bFullyRough = !bHasMetallicRoughnessTexture && InMaterial.RoughnessFactor == 1.0f;
	// For factor * texture, we create a node if factor != 1
	// For solo factor (no texture) we create a node if it differs from Unreal's default unconnected input values:
	// metallic = 0, roughness = 0.5
	const bool bHasMetallicFactor = InMaterial.MetallicFactor != (bHasMetallicRoughnessTexture ? 1.0f : 0.0f);
	const bool bHasRoughnessFactor = !bFullyRough && InMaterial.RoughnessFactor != (bHasMetallicRoughnessTexture ? 1.0f : 0.5f);

	DECLARE_SAMPLER_NODE(MetallicRoughness)

	if (bHasMetallicRoughnessTexture)
	{
		GET_TEXTURE(MetallicRoughness)
		SET_OR_VERIFY(SRGB, false)
		SET_OR_VERIFY(LODGroup, TEXTUREGROUP_WorldSpecular) // maybe?
		SET_OR_VERIFY(CompressionSettings, TC_Masks) // maybe?

		CREATE_SAMPLER_NODE(MetallicRoughness, Masks)
		PlaceOnGrid(MetallicRoughnessSamplerNode, -40, 2, "metallicRoughnessTexture");
	}

	UMaterialExpressionConstant* MetallicFactorNode = nullptr;
	UMaterialExpressionConstant* RoughnessFactorNode = nullptr;

	if (bHasMetallicFactor)
	{
		MetallicFactorNode = NewObject<UMaterialExpressionConstant>(Material);
		PlaceOnGrid(MetallicFactorNode, -33, 2, "metallicFactor");
		MetallicFactorNode->R = InMaterial.MetallicFactor;
	}

	if (bHasRoughnessFactor)
	{
		RoughnessFactorNode = NewObject<UMaterialExpressionConstant>(Material);
		RoughnessFactorNode->R = InMaterial.RoughnessFactor;
		PlaceOnGrid(RoughnessFactorNode, -33, 12, "roughnessFactor");
	}

	ConnectInput(Material, Material->Metallic, MetallicFactorNode, MetallicRoughnessSamplerNode, 3); // B channel
	ConnectInput(Material, Material->Roughness, RoughnessFactorNode, MetallicRoughnessSamplerNode, 2); // G channel

	// Occlusion
	// -- texture.R
	// -- (float)strength * texture.R

	const bool bHasOcclusionTexture = InMaterial.Occlusion.TextureIndex != INDEX_NONE;

	if (bHasOcclusionTexture)
	{
		GET_TEXTURE(Occlusion)
		DECLARE_SAMPLER_NODE(Occlusion)

		// Occlusion is often packed into the same texture as MetallicRoughness.
		const bool bSharedWithMetalRough = MetallicRoughnessSamplerNode && (Texture == MetallicRoughnessSamplerNode->Texture);

		if (bSharedWithMetalRough)
		{
			OcclusionSamplerNode = MetallicRoughnessSamplerNode;
			OcclusionSamplerNode->Desc = TEXT("occlusionRoughnessMetallicTexture");
		}
		else
		{
			SET_OR_VERIFY(SRGB, false)
			SET_OR_VERIFY(LODGroup, TEXTUREGROUP_WorldSpecular) // maybe?
			SET_OR_VERIFY(CompressionSettings, TC_Masks) // maybe?

			CREATE_SAMPLER_NODE(Occlusion, Masks)
			PlaceOnGrid(OcclusionSamplerNode, -45, 17, "occlusionTexture");
		}

		const bool bHasOcclusionStrength = InMaterial.OcclusionStrength != 1.0f;

		UMaterialExpressionConstant* OcclusionStrengthNode = nullptr;

		if (bHasOcclusionStrength)
		{
			OcclusionStrengthNode = NewObject<UMaterialExpressionConstant>(Material);
			OcclusionStrengthNode->R = InMaterial.OcclusionStrength;
			PlaceOnGrid(OcclusionStrengthNode, -33, 22, "occlusion.strength");
		}

		ConnectInput(Material, Material->AmbientOcclusion, OcclusionStrengthNode, OcclusionSamplerNode, 1); // R channel
	}

	// Emissive
	// -- texture
	// -- (vec3)factor * texture
	// -- (vec3)factor (as const emissive color)

	const bool bHasEmissiveTexture = InMaterial.Emissive.TextureIndex != INDEX_NONE;
	// For factor * texture, we create a node if factor != vec3(1)
	// For solo factor (no texture) we create a node if it differs from Unreal's default emission of vec3(0)
	const bool bHasEmissiveFactor = InMaterial.EmissiveFactor != (bHasEmissiveTexture ? FVector(1.0f, 1.0f, 1.0f) : FVector::ZeroVector);

	DECLARE_SAMPLER_NODE(Emissive)

	if (bHasEmissiveTexture)
	{
		GET_TEXTURE(Emissive)
		SET_OR_VERIFY(SRGB, true)
		SET_OR_VERIFY(LODGroup, TEXTUREGROUP_World)
		SET_OR_VERIFY(CompressionSettings, TC_Default)

		CREATE_SAMPLER_NODE(Emissive, Color)
		PlaceOnGrid(EmissiveSamplerNode, -34, 39, "emissiveTexture");
	}

	UMaterialExpressionConstant3Vector* EmissiveFactorNode = nullptr;

	if (bHasEmissiveFactor)
	{
		EmissiveFactorNode = NewObject<UMaterialExpressionConstant3Vector>(Material);
		EmissiveFactorNode->Constant = FLinearColor(InMaterial.EmissiveFactor);
		EmissiveFactorNode->bCollapsed = true;
		PlaceOnGrid(EmissiveFactorNode, -28, 33, "emissiveFactor");
	}

	ConnectInput(Material, Material->EmissiveColor, EmissiveFactorNode, EmissiveSamplerNode);

	#undef GET_TEXTURE
	#undef SET_OR_VERIFY

	// Overall material properties (not inputs)
	//	glTF materials are compatible with MaterialDomain::Surface, MaterialShadingModel::DefaultLit
	Material->bFullyRough = bFullyRough;
	Material->TwoSided = InMaterial.DoubleSided;
	Material->BlendMode = ConvertAlphaMode(InMaterial.AlphaMode);
	if (InMaterial.AlphaMode == FMat::EAlphaMode::Mask)
	{
		Material->OpacityMaskClipValue = InMaterial.AlphaCutoff;
	}

	Material->PostEditChange();
	FAssetRegistryModule::AssetCreated(Material);

	// Set the dirty flag so this package will get saved later
	AssetPackage->SetDirtyFlag(true);

	return Material;
}

TArray<UMaterial*> ImportMaterials(const FAsset& Asset, UObject* InParent, FName InName, EObjectFlags Flags)
{
	const TArray<UTexture2D*> Textures = ImportTextures(Asset, InParent, InName, Flags);

	TArray<UMaterial*> Materials;
	Materials.Reserve(Asset.Materials.Num() + 1);

	for (int32 Index = 0; Index < Asset.Materials.Num(); ++Index)
	{
		const FMat& Mat = Asset.Materials[Index];
		UMaterial* UnMat = ImportMaterial(InParent, InName, Flags, Index, Mat, Textures);

		// No nullptr check because we want this array to match 1-to-1 with Asset.Materials
		// (ImportMaterial should never return nullptr, btw)
		Materials.Add(UnMat);
	}

	return Materials;
}
