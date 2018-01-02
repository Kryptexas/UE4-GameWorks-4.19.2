// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GLTFAsset.h"

#include "Misc/Paths.h"

namespace GLTF {

const FSampler FSampler::DefaultSampler;

FString FTex::GetName() const
{
	if (!Name.IsEmpty())
	{
		return Name;
	}
	else if (!Source.Name.IsEmpty())
	{
		return Source.Name;
	}
	else
	{
		return FPaths::GetBaseFilename(Source.URI);
		// This assumes file URI
		// TODO: handle empty URI or data URI properly
	}
}

void FAsset::Reset(uint32 BufferCount, uint32 BufferViewCount, uint32 AccessorCount, uint32 MeshCount,
                   uint32 ImageCount, uint32 SamplerCount, uint32 TextureCount, uint32 MaterialCount)
{
	Buffers.Empty(BufferCount);
	BufferViews.Empty(BufferViewCount);
	Accessors.Empty(AccessorCount);
	Meshes.Empty(MeshCount);

	Images.Empty(ImageCount);
	Samplers.Empty(SamplerCount);
	Textures.Empty(TextureCount);
	Materials.Empty(MaterialCount);
}

static bool IsConvertibleTo01(const FAccessor& Attrib)
{
	// convertible to 0.0 .. 1.0 factor
	// (colors, tex coords, weights, etc.)
	return Attrib.CompType == FAccessor::ECompType::F32 ||
	         (Attrib.Normalized &&
	            ( Attrib.CompType == FAccessor::ECompType::U8 ||
	              Attrib.CompType == FAccessor::ECompType::U16
	            )
	         );
}

bool FPrimitive::Validate() const
{
	// make sure all semantic attributes meet the spec

	if (!Position.IsValid())
	{
		return false;
	}

	const uint32 VertexCount = Position.Count;

	if (Position.Type != FAccessor::EType::Vec3 || Position.CompType != FAccessor::ECompType::F32)
	{
		return false;
	}

	if (Normal.IsValid())
	{
		if (Normal.Count != VertexCount)
		{
			return false;
		}

		if (Normal.Type != FAccessor::EType::Vec3 || Normal.CompType != FAccessor::ECompType::F32)
		{
			return false;
		}
	}

	if (Tangent.IsValid())
	{
		if (Tangent.Count != VertexCount)
		{
			return false;
		}

		if (Tangent.Type != FAccessor::EType::Vec4 || Tangent.CompType != FAccessor::ECompType::F32)
		{
			return false;
		}
	}

	//for (const FAccessor& TexCoord : { TexCoord0, TexCoord1 }) // runtime hates this
	for (int i : { 0, 1 })
	{
		const FAccessor& TexCoord = (i == 0) ? TexCoord0 : TexCoord1;

		if (TexCoord.IsValid())
		{
			if (TexCoord.Count != VertexCount)
			{
				return false;
			}

			if (TexCoord.Type != FAccessor::EType::Vec2 || !IsConvertibleTo01(TexCoord))
			{
				return false;
			}
		}
	}

	if (Color0.IsValid())
	{
		if (Color0.Count != VertexCount)
		{
			return false;
		}

		if (!(Color0.Type == FAccessor::EType::Vec3 || Color0.Type == FAccessor::EType::Vec4) || !IsConvertibleTo01(Color0))
		{
			return false;
		}
	}

	// TODO: validate ranges? index buffer values?

	return true;
}

bool FAsset::Validate() const
{
	bool IsValid = true;
	for (const FMesh& Mesh : Meshes)
	{
		for (const FPrimitive& Prim : Mesh.Primitives)
		{
			if (!Prim.Validate())
			{
				IsValid = false;
			}
		}
	}

	// TODO: lots more validation

	return IsValid;
}

} // namespace GLTF
