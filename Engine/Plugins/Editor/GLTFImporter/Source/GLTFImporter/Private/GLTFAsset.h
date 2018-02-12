// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// round up to the nearest multiple of 4 (used for padding & vertex attribute alignment)
inline uint32 Pad4(uint32 X)
{
	return (X + 3) & ~3;
}

namespace GLTF {

// for storing triangle indices as a unit
struct UInt32Vector
{
	uint32 A { 0 }, B { 0 }, C { 0 };
};

// --- in-memory versions of glTF types -----------

struct FBuffer
{
	const uint32 ByteLength;
	TArray<uint8> Data;

	// TODO: owned vs. shared data (file mapped)
	// Right now each FBuffer has a copy of its data.

	FBuffer(uint32 InByteLength)
		: ByteLength(InByteLength)
		{
		Data.AddUninitialized(ByteLength);
		}

	const void* DataAt(uint32 Offset) const
	{
		return Data.GetData() + Offset;
	}
};

struct FBufferView
{
	const FBuffer& Buffer;
	const uint32 ByteOffset;
	const uint32 ByteLength;
	uint32 ByteStride; // range 4..252

	FBufferView(const FBuffer& InBuffer, uint32 InOffset, uint32 InLength, uint32 InStride)
		: Buffer(InBuffer)
		, ByteOffset(InOffset)
		, ByteLength(InLength)
		, ByteStride(InStride)
	{
		// check that view fits completely inside the buffer
	}

	const void* DataAt(uint32 Index, uint32 Offset) const
	{
		return Buffer.DataAt(Index * ByteStride + Offset + ByteOffset);
	}
};

struct FAccessor
{
	// accessor stores the data but has no usage semantics

	enum class EType
	{
		None,

		Scalar,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4
	};

	enum class ECompType
	{
		None,

		S8, // signed byte
		U8, // unsigned byte
		S16, // signed short
		U16, // unsigned short
		U32, // unsigned int -- only valid for indices, not attributes
		F32 // float
	};

	const uint32 Count;
	const EType Type;
	const ECompType CompType;
	const bool Normalized;

	FAccessor(uint32 InCount, EType InType, ECompType InCompType, bool InNormalized)
		: Count(InCount)
		, Type(InType)
		, CompType(InCompType)
		, Normalized(InNormalized)
	{
		// if (CompType == ECompType::F32 && Normalized) complain?
	}

	virtual bool IsValid() const = 0;

	virtual uint32 GetUnsignedInt(uint32 Index) const = 0;
	virtual FVector2D GetVec2(uint32 Index) const = 0;
	virtual FVector GetVec3(uint32 Index) const = 0;
	virtual FVector4 GetVec4(uint32 Index) const = 0;

	// GetMatrix functions are not needed yet, will be added later.

	virtual TArray<uint32> GetUnsignedIntArray() const = 0;
	virtual TArray<FVector2D> GetVec2Array() const = 0;
	virtual TArray<FVector> GetVec3Array() const = 0;
	virtual TArray<FVector4> GetVec4Array() const = 0;

	virtual uint32 ValueSize() const = 0;
};

struct FValidAccessor final : FAccessor
{
private:
	const FBufferView& BufferView;
	const uint32 ByteOffset;

	const void* DataAt(uint32 Index) const
	{
		return BufferView.DataAt(Index, ByteOffset);
	}

public:
	FValidAccessor(FBufferView& InBufferView, uint32 InOffset, uint32 InCount, EType InType, ECompType InCompType, bool InNormalized)
		: FAccessor(InCount, InType, InCompType, InNormalized)
		, BufferView(InBufferView)
		, ByteOffset(InOffset)
	{
		if (InBufferView.ByteStride == 0)
		{
			// Hack! The first Accessor of this BufferView will set its ByteStride.
			// This is ok for valid glTF files since any BufferView with >1 Accessors
			// is required to have an explicit ByteStride property.
			InBufferView.ByteStride = ValueSize(); // tightly packed
		}
	}

	bool IsValid() const override
	{
		return true;
	}

	uint32 ValueSize() const override
	{
		static const uint8 ComponentSize[] = { 0, 1, 1, 2, 2, 4, 4 }; // keep in sync with ECompType
		static const uint8 ComponentsPerValue[] = { 0, 1, 2, 3, 4, 4, 9, 16 }; // keep in sync with EType

		return ComponentsPerValue[(int)Type] * ComponentSize[(int)CompType];
	}

	uint32 GetUnsignedInt(uint32 Index) const override
	{
		// should be Scalar, not Normalized, unsigned integer (8, 16 or 32 bit)

		if (Index < Count)
		{
			if (Type == EType::Scalar && !Normalized)
			{
				const void* Pointer = DataAt(Index);

				switch (CompType)
				{
					case ECompType::U8:
						return *(const uint8*)Pointer;
					case ECompType::U16:
						return *(const uint16*)Pointer;
					case ECompType::U32:
						return *(const uint32*)Pointer;
					default:
						break;
				}
			}
		}

		// complain?
		return 0;
	}

	FVector2D GetVec2(uint32 Index) const override
	{
		// Spec-defined attributes (TEXCOORD_0, TEXCOORD_1) use only these formats:
		// - F32
		// - U8 normalized
		// - U16 normalized
		// Custom attributes can use any CompType, so add support for those when needed.

		if (Index < Count)
		{
			if (Type == EType::Vec2) // strict format match, unlike GPU shader fetch
			{
				const void* Pointer = DataAt(Index);

				if (CompType == ECompType::F32)
				{
					// copy float vec2 directly from buffer
					const float* P = static_cast<const float*>(Pointer);
					return FVector2D(P[0], P[1]);
				}
				else if (Normalized)
				{
					// convert to 0..1
					if (CompType == ECompType::U8)
					{
						const uint8* P = static_cast<const uint8*>(Pointer);
						constexpr float S = 1.0f / 255.0f;
						return FVector2D(S * P[0], S * P[1]);
					}
					else if (CompType == ECompType::U16)
					{
						const uint16* P = static_cast<const uint16*>(Pointer);
						constexpr float S = 1.0f / 65535.0f;
						return FVector2D(S * P[0], S * P[1]);
					}
				}
			}
		}

		// unsupported format
		return FVector2D::ZeroVector;
	}

	FVector GetVec3(uint32 Index) const override
	{
		// Spec-defined attributes (POSITION, NORMAL, COLOR_0) use only these formats:
		// - F32
		// - U8 normalized
		// - U16 normalized
		// Custom attributes can use any CompType, so add support for those when needed.

		if (Index < Count)
		{
			if (Type == EType::Vec3) // strict format match, unlike GPU shader fetch
			{
				const void* Pointer = DataAt(Index);

				if (CompType == ECompType::F32)
				{
					// copy float vec3 directly from buffer
					const float* P = static_cast<const float*>(Pointer);
					return FVector(P[0], P[1], P[2]);
				}
				else if (Normalized)
				{
					// convert to 0..1
					if (CompType == ECompType::U8)
					{
						const uint8* P = static_cast<const uint8*>(Pointer);
						constexpr float S = 1.0f / 255.0f;
						return FVector(S * P[0], S * P[1], S * P[2]);
					}
					else if (CompType == ECompType::U16)
					{
						const uint16* P = static_cast<const uint16*>(Pointer);
						constexpr float S = 1.0f / 65535.0f;
						return FVector(S * P[0], S * P[1], S * P[2]);
					}
				}
			}
		}

		// unsupported format
		return FVector::ZeroVector;
	}

	FVector4 GetVec4(uint32 Index) const override
	{
		// Spec-defined attributes (TANGENT, COLOR_0) use only these formats:
		// - F32
		// - U8 normalized
		// - U16 normalized
		// Custom attributes can use any CompType, so add support for those when needed.

		if (Index < Count)
		{
			if (Type == EType::Vec4) // strict format match, unlike GPU shader fetch
			{
				const void* Pointer = DataAt(Index);

				if (CompType == ECompType::F32)
				{
					// copy float vec4 directly from buffer
					const float* P = static_cast<const float*>(Pointer);
					return FVector4(P[0], P[1], P[2], P[3]);
				}
				else if (Normalized)
				{
					// convert to 0..1
					if (CompType == ECompType::U8)
					{
						const uint8* P = static_cast<const uint8*>(Pointer);
						constexpr float S = 1.0f / 255.0f;
						return FVector4(S * P[0], S * P[1], S * P[2], S * P[3]);
					}
					else if (CompType == ECompType::U16)
					{
						const uint16* P = static_cast<const uint16*>(Pointer);
						constexpr float S = 1.0f / 65535.0f;
						return FVector4(S * P[0], S * P[1], S * P[2], S * P[3]);
					}
				}
			}
		}

		// unsupported format
		return FVector4();
	}

	TArray<uint32> GetUnsignedIntArray() const override
	{
		// cheesy & unoptimized
		TArray<uint32> Result;
		Result.Reserve(Count);
		for (uint32 i = 0; i < Count; ++i)
		{
			Result.Push(GetUnsignedInt(i));
		}
		return Result;
	}

	TArray<FVector2D> GetVec2Array() const override
	{
		// cheesy & unoptimized
		TArray<FVector2D> Result;
		Result.Reserve(Count);
		for (uint32 i = 0; i < Count; ++i)
		{
			Result.Push(GetVec2(i));
		}
		return Result;
	}

	TArray<FVector> GetVec3Array() const override
	{
		// cheesy & unoptimized
		TArray<FVector> Result;
		Result.Reserve(Count);
		for (uint32 i = 0; i < Count; ++i)
		{
			Result.Push(GetVec3(i));
		}
		return Result;
	}

	TArray<FVector4> GetVec4Array() const override
	{
		// cheesy & unoptimized
		TArray<FVector4> Result;
		Result.Reserve(Count);
		for (uint32 i = 0; i < Count; ++i)
		{
			Result.Push(GetVec4(i));
		}
		return Result;
	}
};

struct FVoidAccessor final : FAccessor
{
	FVoidAccessor()
		: FAccessor(0, EType::None, ECompType::None, false)
	{
	}

	bool IsValid() const override
	{
		return false;
	}

	uint32 GetUnsignedInt(uint32 Index) const override
	{
		return 0;
	}

	FVector2D GetVec2(uint32 Index) const override
	{
		return FVector2D::ZeroVector;
	}

	FVector GetVec3(uint32 Index) const override
	{
		return FVector::ZeroVector;
	}

	FVector4 GetVec4(uint32 Index) const override
	{
		return FVector4();
	}

	TArray<uint32> GetUnsignedIntArray() const override
	{
		return TArray<uint32>();
	}

	TArray<FVector2D> GetVec2Array() const override
	{
		return TArray<FVector2D>();
	}

	TArray<FVector> GetVec3Array() const override
	{
		return TArray<FVector>();
	}

	TArray<FVector4> GetVec4Array() const override
	{
		return TArray<FVector4>();
	}

	uint32 ValueSize() const override
	{
		return 0;
	}
};

class FPrimitive
{
	// index buffer
	const FAccessor& Indices;

	// attributes
	const FAccessor& Position; // required
	const FAccessor& Normal;
	const FAccessor& Tangent;
	const FAccessor& TexCoord0;
	const FAccessor& TexCoord1;
	const FAccessor& Color0;

public:
	enum class EMode
	{
		None,

		// valid but unsupported
		Points,
		Lines,
		LineLoop,
		LineStrip,

		// initially supported
		Triangles,

		// will be supported prior to release
		TriangleStrip,
		TriangleFan
	};

	const EMode Mode;
	const int32 MaterialIndex;

	FPrimitive(EMode InMode, int32 InMaterial,
	           const FAccessor& InIndices,
	           const FAccessor& InPosition,
	           const FAccessor& InNormal,
	           const FAccessor& InTangent,
	           const FAccessor& InTexCoord0,
	           const FAccessor& InTexCoord1,
	           const FAccessor& InColor0)
		: Indices(InIndices)
		, Position(InPosition)
		, Normal(InNormal)
		, Tangent(InTangent)
		, TexCoord0(InTexCoord0)
		, TexCoord1(InTexCoord1)
		, Color0(InColor0)
		, Mode(InMode)
		, MaterialIndex(InMaterial)	
	{
	}

	bool Validate() const;

	TArray<FVector> GetPositions() const
	{
		return Position.GetVec3Array();
	}

	bool HasNormals() const
	{
		return Normal.IsValid();
	}

	TArray<FVector> GetNormals() const
	{
		return Normal.GetVec3Array();
	}

	bool HasTangents() const
	{
		return Tangent.IsValid();
	}

	TArray<FVector4> GetTangents() const
	{
		return Tangent.GetVec4Array();
	}

	bool HasTexCoords(uint32 Index) const
	{
		switch (Index)
		{
			case 0:
				return TexCoord0.IsValid();
			case 1:
				return TexCoord1.IsValid();
			default:
				return false;
		}
	}

	TArray<FVector2D> GetTexCoords(uint32 Index) const
	{
		switch (Index)
		{
			case 0:
				return TexCoord0.GetVec2Array();
			case 1:
				return TexCoord1.GetVec2Array();
			default:
				return TArray<FVector2D>();
		}
	}

	UInt32Vector TriangleVerts(uint32 T) const
	{
		UInt32Vector Result;
		if (T < TriangleCount())
		{
			const bool Indexed = Indices.IsValid();
			switch (Mode)
			{
				case EMode::Triangles:
					if (Indexed)
					{
						Result.A = Indices.GetUnsignedInt(3 * T);
						Result.B = Indices.GetUnsignedInt(3 * T + 1);
						Result.C = Indices.GetUnsignedInt(3 * T + 2);
					}
					else
					{
						Result.A = 3 * T;
						Result.B = 3 * T + 1;
						Result.C = 3 * T + 2;
					}
					break;
				case EMode::TriangleStrip:
					// are indexed TriangleStrip & TriangleFan valid?
					// I don't see anything in the spec that says otherwise...
					if (Indexed)
					{
						if (T % 2 == 0)
						{
							Result.A = Indices.GetUnsignedInt(T);
							Result.B = Indices.GetUnsignedInt(T + 1);
						}
						else
						{
							Result.A = Indices.GetUnsignedInt(T + 1);
							Result.B = Indices.GetUnsignedInt(T);
						}
						Result.C = Indices.GetUnsignedInt(T + 2);
					}
					else
					{
						if (T % 2 == 0)
						{
							Result.A = T;
							Result.B = T + 1;
						}
						else
						{
							Result.A = T + 1;
							Result.B = T;
						}
						Result.C = T + 2;
					}
					break;
				case EMode::TriangleFan:
					if (Indexed)
					{
						Result.A = Indices.GetUnsignedInt(0);
						Result.B = Indices.GetUnsignedInt(T + 1);
						Result.C = Indices.GetUnsignedInt(T + 2);
					}
					else
					{
						Result.A = 0;
						Result.B = T + 1;
						Result.C = T + 2;
					}
					break;
				default:
					break;
			}
		}

		return Result;
	}

	TArray<uint32> GetTriangleIndices() const
	{
		// TODO: cache result & return a const reference

		TArray<uint32> Result;
		if (Mode == EMode::Triangles)
		{
			if (Indices.IsValid())
			{
				return Indices.GetUnsignedIntArray();
			}
			else
			{
				// generate indices [0 1 2][3 4 5]...
				const uint32 N = TriangleCount() * 3;
				Result.Reserve(N);
				for (uint32 i = 0; i < N; ++i)
				{
					Result.Push(i);
				}
			}
		}
		else
		{
			const uint32 N = TriangleCount() * 3;
			Result.Reserve(N);
			for (uint32 i = 0; i < TriangleCount(); ++i)
			{
				UInt32Vector Tri = TriangleVerts(i);
				Result.Push(Tri.A);
				Result.Push(Tri.B);
				Result.Push(Tri.C);
			}
		}

		return Result;
	}

	uint32 VertexCount() const
	{
		if (Indices.IsValid())
			return Indices.Count;
		else
			return Position.Count;
	}

	uint32 TriangleCount() const
	{
		switch (Mode)
		{
			case EMode::Triangles:
				return VertexCount() / 3;
			case EMode::TriangleStrip:
			case EMode::TriangleFan:
				return VertexCount() - 2;
			default:
				return 0;
		}
	}
};

struct FMesh
{
	FString Name;

	TArray<FPrimitive> Primitives;

	bool HasNormals() const
	{
		for (const FPrimitive& Prim : Primitives)
		{
			if (Prim.HasNormals())
			{
				return true;
			}
		}
		return false;
	}

	bool HasTangents() const
	{
		for (const FPrimitive& Prim : Primitives)
		{
			if (Prim.HasTangents())
			{
				return true;
			}
		}
		return false;
	}

	bool HasTexCoords(uint32 Index) const
	{
		for (const FPrimitive& Prim : Primitives)
		{
			if (Prim.HasTexCoords(Index))
			{
				return true;
			}
		}
		return false;
	}
};

struct FImage
{
	enum class EFormat
	{
		None,

		PNG,
		JPEG
	};

	FString Name;
	FString URI;
	EFormat Format;

	// bool bFromFile; // would this be useful?
	FString FilePath;

	// Image data is kept encoded in Format, to be decoded when
	// needed by Unreal.
	TArray<uint8> Data;
};

struct FSampler
{
	enum class EFilter
	{
		None,

		// valid for Min & Mag
		Nearest,
		Linear,

		// valid for Min only
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear
	};

	enum class EWrap
	{
		None,

		Repeat,
		MirroredRepeat,
		ClampToEdge
	};

	// Spec defines no default min/mag filter.
	// Should we set reasonable (high quality) default values here, or set to None
	// and let GLTFMaterial.cpp interpret however it wants?
	EFilter MinFilter { EFilter::None }; // best quality = LML
	EFilter MagFilter { EFilter::None }; // best quality = L

	EWrap WrapS { EWrap::Repeat };
	EWrap WrapT { EWrap::Repeat };

	FSampler() {}

	static const FSampler DefaultSampler;
};

struct FTex
{
private:
	const FString Name;

public:
	const FImage& Source;
	const FSampler& Sampler;

	FTex(const FString& InName, const FImage& InSource, const FSampler& InSampler)
		: Name(InName)
		, Source(InSource)
		, Sampler(InSampler)
	{}

	FString GetName() const;
};

struct FMatTex
{
	int32 TextureIndex { INDEX_NONE };
	uint8 TexCoord { 0 };
};

struct FMat
{
	enum class EAlphaMode
	{
		None,

		Opaque,
		Mask,
		Blend
	};

	const FString Name;

	FMat(const FString& InName)
		: Name(InName)
	{}

	// PBR material inputs
	FMatTex BaseColor;
	FMatTex MetallicRoughness;
	FVector4 BaseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
	float MetallicFactor { 1.0f };
	float RoughnessFactor { 1.0f };

	// base material inputs
	FMatTex Normal;
	FMatTex Occlusion;
	FMatTex Emissive;
	float NormalScale { 1.0f };
	float OcclusionStrength { 1.0f };
	FVector EmissiveFactor { FVector::ZeroVector };

	// material properties
	bool DoubleSided { false };
	EAlphaMode AlphaMode { EAlphaMode::Opaque };
	float AlphaCutoff { 0.5f }; // only meaningful when AlphaMode == Mask

	bool IsFullyRough() const // move this to GLTFMaterial.cpp ?
	{
		// No separate roughness texture in glTF :(
		return RoughnessFactor == 1.0f && MetallicRoughness.TextureIndex == INDEX_NONE;
	}

	bool IsOpaque() const
	{
		return AlphaMode == EAlphaMode::Opaque;
		// Can add more conditions here:
		// BaseColorTexture has no alpha channel && BaseColorFactor.A == 1.0
	}
};

struct FAsset
{
	TArray<FBuffer> Buffers;
	TArray<FBufferView> BufferViews;
	TArray<FValidAccessor> Accessors;
	TArray<FMesh> Meshes;

	TArray<FImage> Images;
	TArray<FSampler> Samplers;
	TArray<FTex> Textures;
	TArray<FMat> Materials;

	void Reset(uint32 BufferCount, uint32 BufferViewCount, uint32 AccessorCount, uint32 MeshCount,
	           uint32 ImageCount, uint32 SamplerCount, uint32 TextureCount, uint32 MaterialCount);

	bool Validate() const;
};

} // namespace GLTF
