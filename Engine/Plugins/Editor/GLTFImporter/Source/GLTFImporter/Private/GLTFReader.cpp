// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GLTFReader.h"

#include "Misc/FileHelper.h"
#include "Misc/Base64.h"

using namespace GLTF;

// glTF uses arrays for storage, and references by index into these arrays
using INDEX = int32;
// INDEX_NONE = -1 from CoreMiscDefines.h

static FAccessor::ECompType CompTypeFromNumber(uint16 Num)
{
	switch (Num)
	{
	case 5120: return FAccessor::ECompType::S8;
	case 5121: return FAccessor::ECompType::U8;
	case 5122: return FAccessor::ECompType::S16;
	case 5123: return FAccessor::ECompType::U16;
	case 5125: return FAccessor::ECompType::U32;
	case 5126: return FAccessor::ECompType::F32;

	default: return FAccessor::ECompType::None;
	}
}

static FAccessor::EType TypeFromString(const FString& S)
{
	// case sensitive comparison
	#define CONVERT(String, Enum) \
		if (FPlatformString::Strcmp(*S, TEXT(String)) == 0) { return FAccessor::EType::Enum; }

	CONVERT("SCALAR", Scalar)
	else CONVERT("VEC2", Vec2)
	else CONVERT("VEC3", Vec3)
	else CONVERT("VEC4", Vec4)
	else CONVERT("MAT2", Mat2)
	else CONVERT("MAT3", Mat3)
	else CONVERT("MAT4", Mat4)

	return FAccessor::EType::None;

	#undef CONVERT
}

static FPrimitive::EMode ModeFromNumber(uint32 Num)
{
	if (Num > 6)
	{
		return FPrimitive::EMode::None;
	}

	// translate glTF enum [0..6] to our enum [1..7]
	return (FPrimitive::EMode)(Num + 1);
}

static FMat::EAlphaMode AlphaModeFromString(const FString& S)
{
	// case sensitive comparison
	#define CONVERT(String, Enum) \
		if (FPlatformString::Strcmp(*S, TEXT(String)) == 0) { return FMat::EAlphaMode::Enum; }

	CONVERT("OPAQUE", Opaque)
	else CONVERT("MASK", Mask)
	else CONVERT("BLEND", Blend)

	return FMat::EAlphaMode::None;

	#undef CONVERT
}

static FImage::EFormat ImageFormatFromMimeType(const FString& S)
{
	// case sensitive comparison
	#define CONVERT(String, Enum) \
		if (FPlatformString::Strcmp(*S, TEXT(String)) == 0) { return FImage::EFormat::Enum; }

	CONVERT("image/jpeg", JPEG)
	else CONVERT("image/png", PNG)

	return FImage::EFormat::None;

#undef CONVERT
}

static FImage::EFormat ImageFormatFromFilename(const FString& Filename)
{
	// attempt to guess from filename extension
	if (Filename.EndsWith(".png"))
	{
		return FImage::EFormat::PNG;
	}
	else if (Filename.EndsWith(".jpg") || Filename.EndsWith(".jpeg"))
	{
		return FImage::EFormat::JPEG;
	}

	return FImage::EFormat::None;
}

static FSampler::EFilter FilterFromNumber(uint16 Num)
{
	switch (Num)
	{
	case 9728: return FSampler::EFilter::Nearest;
	case 9729: return FSampler::EFilter::Linear;
	case 9984: return FSampler::EFilter::NearestMipmapNearest;
	case 9985: return FSampler::EFilter::LinearMipmapNearest;
	case 9986: return FSampler::EFilter::NearestMipmapLinear;
	case 9987: return FSampler::EFilter::LinearMipmapLinear;

	default: return FSampler::EFilter::None;
	}
}

static FSampler::EWrap WrapModeFromNumber(uint16 Num)
{
	switch (Num)
	{
	case 10497: return FSampler::EWrap::Repeat;
	case 33648: return FSampler::EWrap::MirroredRepeat;
	case 33071: return FSampler::EWrap::ClampToEdge;

	default: return FSampler::EWrap::None;
	}
}

// --- JSON utility functions --------------------------

static uint32 ArraySize(const FJsonObject& Object, const FString& Name)
{
	if (Object.HasTypedField<EJson::Array>(Name))
	{
		return Object.GetArrayField(Name).Num();
	}

	return 0; // for empty arrays & if array does not exist
}

static FString GetString(const FJsonObject& Object, const char* Name, const FString& DefaultValue = FString())
{
	if (Object.HasTypedField<EJson::String>(Name))
	{
		return Object.GetStringField(Name);
	}

	return DefaultValue;
}

static bool GetBool(const FJsonObject& Object, const char* Name, bool DefaultValue = false)
{
	if (Object.HasTypedField<EJson::Boolean>(Name))
	{
		return Object.GetBoolField(Name);
	}

	return DefaultValue;
}

static uint32 GetUnsignedInt(const FJsonObject& Object, const char* Name, uint32 DefaultValue = 0)
{
	if (Object.HasTypedField<EJson::Number>(Name))
	{
		int32 SignedValue = Object.GetIntegerField(Name);
		if (SignedValue >= 0)
		{
			return (uint32)SignedValue;
		}
		// complain if negative? if fractional?
	}

	return DefaultValue;
}

static INDEX GetIndex(const FJsonObject& Object, const char* Name)
{
	return GetUnsignedInt(Object, Name, INDEX_NONE);
}

static float GetScalar(const FJsonObject& Object, const char* Name, float DefaultValue = 0.0f)
{
	if (Object.HasTypedField<EJson::Number>(Name))
	{
		return Object.GetNumberField(Name);
	}

	return DefaultValue;
}

static FVector GetVec3(const FJsonObject& Object, const char* Name, const FVector& DefaultValue = FVector::ZeroVector)
{
	if (Object.HasTypedField<EJson::Array>(Name))
	{
		const TArray<TSharedPtr<FJsonValue>>& Array = Object.GetArrayField(Name);
		if (Array.Num() == 3)
		{
			float X = Array[0]->AsNumber();
			float Y = Array[1]->AsNumber();
			float Z = Array[2]->AsNumber();
			return FVector(X,Y,Z);
		}
	}

	return DefaultValue;
}

static FVector4 GetVec4(const FJsonObject& Object, const char* Name, const FVector4& DefaultValue = FVector4())
{
	if (Object.HasTypedField<EJson::Array>(Name))
	{
		const TArray<TSharedPtr<FJsonValue>>& Array = Object.GetArrayField(Name);
		if (Array.Num() == 4)
		{
			float X = Array[0]->AsNumber();
			float Y = Array[1]->AsNumber();
			float Z = Array[2]->AsNumber();
			float W = Array[3]->AsNumber();
			return FVector4(X,Y,Z,W);
		}
	}

	return DefaultValue;
}

// --- Base64 utility function --------------------------

static bool DecodeDataURI(const FString& URI, FString& OutMimeType, TArray<uint8>& OutData)
{
	// Data URIs look like "data:[<mime-type>][;encoding],<data>"
	// glTF always uses base64 encoding for data URIs

	check(URI.StartsWith("data:"));

	int32 Semicolon, Comma;
	bool HasSemicolon = URI.FindChar(TEXT(';'), Semicolon);
	bool HasComma = URI.FindChar(TEXT(','), Comma);

	if (!(HasSemicolon && HasComma))
	{
		return false;
	}

	const FString Encoding = URI.Mid(Semicolon + 1, Comma - Semicolon - 1);

	if (Encoding != TEXT("base64"))
	{
		return false;
	}

	OutMimeType = URI.Mid(5, Semicolon - 5);

	const FString EncodedData = URI.RightChop(Comma + 1);

	return FBase64::Decode(EncodedData, OutData);
}

// --- FGLTFReader implementation ------------------------------

void FGLTFReader::SetupBuffer(const FJsonObject& Object)
{
	const uint32 ByteLength = GetUnsignedInt(Object, "byteLength");
	Asset.Buffers.Emplace(ByteLength);
	FBuffer& Buffer = Asset.Buffers.Last();
	// set Buffer.Data from Object.uri
	if (Object.HasTypedField<EJson::String>("uri"))
	{
		const FString& URI = Object.GetStringField("uri");
		if (URI.StartsWith("data:"))
		{
			FString MimeType;
			bool Success = DecodeDataURI(URI, MimeType, Buffer.Data);
			if (!Success || MimeType != "application/octet-stream")
			{
				Warn.Log("Problem decoding buffer from data URI.");
			}
		}
		else
		{
			// Load buffer from external file.
			const FString FullPath = Path / URI;
			FArchive* Reader = IFileManager::Get().CreateFileReader(*FullPath);
			if (Reader)
			{
				const int64 FileSize = Reader->TotalSize();
				if (ByteLength == FileSize)
				{
					Reader->Serialize(Buffer.Data.GetData(), ByteLength);
					// Warn.Log("Buffer loaded from file.");
				}
				else
				{
					Warn.Log("Buffer file size does not match.");
				}

				Reader->Close();
				delete Reader;
			}
			else
			{
				Warn.Log("Could not load file.");
			}
		}
	}
	else
	{
		// Missing URI means use binary chunk of GLB
		const uint32 BinSize = BinChunk.Num();
		if (BinSize == 0)
		{
			Warn.Log("Buffer from BIN chunk is missing or empty.");
		}
		else if (BinSize < ByteLength)
		{
			Warn.Log("Buffer from BIN chunk is too small.");
		}
		else
		{
			FMemory::Memcpy(Buffer.Data.GetData(), BinChunk.GetData(), ByteLength);
		}
	}
}

void FGLTFReader::SetupBufferView(const FJsonObject& Object)
{
	const uint32 BufferIdx = GetUnsignedInt(Object, "buffer");
	if (BufferIdx < BufferCount) // must be true
	{
		const uint32 ByteOffset = GetUnsignedInt(Object, "byteOffset");
		const uint32 ByteLength = GetUnsignedInt(Object, "byteLength");
		const uint32 ByteStride = GetUnsignedInt(Object, "byteStride");
		Asset.BufferViews.Emplace(Asset.Buffers[BufferIdx], ByteOffset, ByteLength, ByteStride);
	}
}

void FGLTFReader::SetupAccessor(const FJsonObject& Object)
{
	const uint32 BufferViewIdx = GetUnsignedInt(Object, "bufferView");
	if (BufferViewIdx < BufferViewCount) // must be true
	{
		const uint32 ByteOffset = GetUnsignedInt(Object, "byteOffset");
		const FAccessor::ECompType CompType = CompTypeFromNumber(GetUnsignedInt(Object, "componentType"));
		const uint32 Count = GetUnsignedInt(Object, "count");
		const FAccessor::EType Type = TypeFromString(Object.GetStringField("type"));
		const bool Normalized = GetBool(Object, "normalized");
		Asset.Accessors.Emplace(Asset.BufferViews[BufferViewIdx], ByteOffset, Count, Type, CompType, Normalized);
	}
}

static const FAccessor& AccessorAtIndex(const TArray<FValidAccessor>& Accessors, INDEX Index)
{
	if (Accessors.IsValidIndex(Index))
	{
		return Accessors[Index];
	}
	else
	{
		static const FVoidAccessor Void;
		return Void;
	}
}

void FGLTFReader::SetupPrimitive(const FJsonObject& Object, FMesh& Mesh)
{
	const FPrimitive::EMode Mode = ModeFromNumber(GetUnsignedInt(Object, "mode", 4)); // default mode is Triangles
	const INDEX MaterialIndex = GetIndex(Object, "material");
	const TArray<FValidAccessor>& A = Asset.Accessors;

	const FAccessor& Indices = AccessorAtIndex(A, GetIndex(Object, "indices"));

	// the only required attribute is POSITION
	const FJsonObject& Attributes = *Object.GetObjectField("attributes");
	const FAccessor& Position = AccessorAtIndex(A, GetIndex(Attributes, "POSITION"));
	const FAccessor& Normal = AccessorAtIndex(A, GetIndex(Attributes, "NORMAL"));
	const FAccessor& Tangent = AccessorAtIndex(A, GetIndex(Attributes, "TANGENT"));
	const FAccessor& TexCoord0 = AccessorAtIndex(A, GetIndex(Attributes, "TEXCOORD_0"));
	const FAccessor& TexCoord1 = AccessorAtIndex(A, GetIndex(Attributes, "TEXCOORD_1"));
	const FAccessor& Color0 = AccessorAtIndex(A, GetIndex(Attributes, "COLOR_0"));

	Mesh.Primitives.Emplace(Mode, MaterialIndex, Indices, Position, Normal, Tangent, TexCoord0, TexCoord1, Color0);
	if (!Mesh.Primitives.Last().Validate())
	{
		Warn.Log("Invalid primitive :(");
	}
}

void FGLTFReader::SetupMesh(const FJsonObject& Object)
{
	Asset.Meshes.Emplace();
	FMesh& Mesh = Asset.Meshes.Last();

	const TArray<TSharedPtr<FJsonValue>>& PrimArray = Object.GetArrayField("primitives");

	Mesh.Name = GetString(Object, "name");
	Mesh.Primitives.Reserve(PrimArray.Num());

	for (TSharedPtr<FJsonValue> Value : PrimArray)
	{
		const FJsonObject& PrimObject = *Value->AsObject();
		SetupPrimitive(PrimObject, Mesh);
	}
}

void FGLTFReader::SetupImage(const FJsonObject& Object)
{
	Asset.Images.Emplace();
	FImage& Image = Asset.Images.Last();

	Image.Name = GetString(Object, "name");

	// Get data now, so Unreal doesn't need to care about where the data came from.
	// Unreal *is* responsible for decoding Data based on Format.
	if (Object.HasTypedField<EJson::String>("uri"))
	{
		Image.URI = Object.GetStringField("uri");
		if (Image.URI.StartsWith("data:"))
		{
			FString MimeType;
			bool Success = DecodeDataURI(Image.URI, MimeType, Image.Data);
			Image.Format = ImageFormatFromMimeType(MimeType);
			if (!Success || Image.Format == FImage::EFormat::None)
			{
				Warn.Log("Problem decoding image from data URI.");
			}
		}
		else // Load buffer from external file.
		{
			if (Image.Format == FImage::EFormat::None)
			{
				Image.Format = ImageFormatFromFilename(Image.URI);
			}

			Image.FilePath = Path / Image.URI;
			if (FFileHelper::LoadFileToArray(Image.Data, *Image.FilePath))
			{
				// Warn.Log("Image loaded from file.");
			}
			else
			{
				Warn.Log("Could not load image file.");
			}
		}
	}
	else
	{
		// Missing URI means use a BufferView
		const INDEX Index = GetIndex(Object, "bufferView");
		if (Asset.BufferViews.IsValidIndex(Index))
		{
			Image.Format = ImageFormatFromMimeType(GetString(Object, "mimeType"));
			// TODO: verify Format is PNG or JPEG (don't let invalid glTF crash/corrupt us)

			const FBufferView& BufferView = Asset.BufferViews[Index];
			const uint32 Size = BufferView.ByteLength;
			// We just created Image, so Image.Data is empty. Fill it with encoded bytes!
			Image.Data.AddUninitialized(Size);
			FMemory::Memcpy(Image.Data.GetData(), BufferView.DataAt(0, 0), Size);
		}
	}
}

void FGLTFReader::SetupSampler(const FJsonObject& Object)
{
	Asset.Samplers.Emplace();
	FSampler& Sampler = Asset.Samplers.Last();

	Sampler.MinFilter = FilterFromNumber(GetUnsignedInt(Object, "minFilter"));
	Sampler.MagFilter = FilterFromNumber(GetUnsignedInt(Object, "magFilter"));
	Sampler.WrapS = WrapModeFromNumber(GetUnsignedInt(Object, "wrapS", 10497)); // default mode is Repeat
	Sampler.WrapT = WrapModeFromNumber(GetUnsignedInt(Object, "wrapT", 10497));
}

void FGLTFReader::SetupTexture(const FJsonObject& Object)
{
	INDEX SourceIndex = GetIndex(Object, "source");
	INDEX SamplerIndex = GetIndex(Object, "sampler");

	// According to the spec it's possible to have a Texture with no Image source.
	// In that case use a default image (checkerboard?).

	if (Asset.Images.IsValidIndex(SourceIndex))
	{
		const bool HasSampler = Asset.Samplers.IsValidIndex(SamplerIndex);

		const FString TexName = GetString(Object, "name");

		const FImage& Source = Asset.Images[SourceIndex];
		const FSampler& Sampler = HasSampler ? Asset.Samplers[SamplerIndex] : FSampler::DefaultSampler;

		Asset.Textures.Emplace(TexName, Source, Sampler);
	}
}

float FGLTFReader::SetupMaterialTexture(FMatTex& MatTex, const FJsonObject& Object, const char* TexName, const char* ScaleName = nullptr)
{
	float Scale = 1.0f;

	if (Object.HasTypedField<EJson::Object>(TexName))
	{
		const FJsonObject& TexObj = *Object.GetObjectField(TexName);
		INDEX TexIndex = GetIndex(TexObj, "index");
		if (Asset.Textures.IsValidIndex(TexIndex))
		{
			MatTex.TextureIndex = TexIndex;
			uint32 TexCoord = GetUnsignedInt(TexObj, "texCoord");
			check(TexCoord < 2); // How many texcoords does Unreal support? This plugin supports only 0 and 1 at the moment.
			MatTex.TexCoord = TexCoord;
			if (ScaleName)
			{
				if (TexObj.HasTypedField<EJson::Number>(ScaleName))
				{
					Scale = TexObj.GetNumberField(ScaleName);
				}
			}
		}
	}

	return Scale;
}

void FGLTFReader::SetupMaterial(const FJsonObject& Object)
{
	Asset.Materials.Emplace(GetString(Object, "name"));
	FMat& Mat = Asset.Materials.Last();

	SetupMaterialTexture(Mat.Emissive, Object, "emissiveTexture");
	Mat.EmissiveFactor = GetVec3(Object, "emissiveFactor");

	Mat.NormalScale = SetupMaterialTexture(Mat.Normal, Object, "normalTexture", "scale");
	Mat.OcclusionStrength = SetupMaterialTexture(Mat.Occlusion, Object, "occlusionTexture", "strength");

	if (Object.HasTypedField<EJson::Object>("pbrMetallicRoughness"))
	{
		const FJsonObject& PBR = *Object.GetObjectField("pbrMetallicRoughness");

		SetupMaterialTexture(Mat.BaseColor, PBR, "baseColorTexture");
		Mat.BaseColorFactor = GetVec4(PBR, "baseColorFactor", FVector4(1.0f, 1.0f, 1.0f, 1.0f));

		SetupMaterialTexture(Mat.MetallicRoughness, PBR, "metallicRoughnessTexture");
		Mat.MetallicFactor = GetScalar(PBR, "metallicFactor", 1.0f);
		Mat.RoughnessFactor = GetScalar(PBR, "roughnessFactor", 1.0f);
	}

	if (Object.HasTypedField<EJson::String>("alphaMode"))
	{
		Mat.AlphaMode = AlphaModeFromString(Object.GetStringField("alphaMode"));
		if (Mat.AlphaMode == FMat::EAlphaMode::Mask)
		{
			Mat.AlphaCutoff = GetScalar(Object, "alphaCutoff", 0.5f);
		}
	}

	Mat.DoubleSided = GetBool(Object, "doubleSided");
}

FGLTFReader::FGLTFReader(FAsset& OutAsset, const FJsonObject& Root, const FString& InPath, FFeedbackContext& InWarn, const TArray<uint8>& InChunk)
	: Asset(OutAsset)
	, BinChunk(InChunk)
	, Path(InPath)
	, Warn(InWarn)
{
	// Check file format version to make sure we can read it.
	const TSharedPtr<FJsonObject>& AssetInfo = Root.GetObjectField("asset");
	if (AssetInfo->HasTypedField<EJson::Number>("minVersion"))
	{
		const double MinVersion = AssetInfo->GetNumberField("minVersion");
		if (MinVersion > 2.0)
		{
			Warn.Log("This importer supports glTF version 2.0 (or compatible) assets.");
			return;
		}
	}
	else
	{
		const double Version = AssetInfo->GetNumberField("version");
		if (Version < 2.0)
		{
			Warn.Log("This importer supports glTF asset version 2.0 or later.");
			return;
		}
	}

	BufferCount = ArraySize(Root, "buffers"); // would prefer not to repeat string literals (DRY)
	BufferViewCount = ArraySize(Root, "bufferViews");
	AccessorCount = ArraySize(Root, "accessors");
	MeshCount = ArraySize(Root, "meshes");

	ImageCount = ArraySize(Root, "images");
	SamplerCount = ArraySize(Root, "samplers");
	TextureCount = ArraySize(Root, "textures");
	MaterialCount = ArraySize(Root, "materials");

	Asset.Reset(BufferCount, BufferViewCount, AccessorCount, MeshCount,
	            ImageCount, SamplerCount, TextureCount, MaterialCount);

	if (BufferCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("buffers"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupBuffer(Object);
		}
	}

	if (BufferViewCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("bufferViews"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupBufferView(Object);
		}
	}

	if (AccessorCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("accessors"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupAccessor(Object);
		}
	}

	if (MeshCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("meshes"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupMesh(Object);
		}
	}

	if (ImageCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("images"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupImage(Object);
		}
	}

	if (SamplerCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("samplers"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupSampler(Object);
		}
	}

	if (TextureCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("textures"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupTexture(Object);
		}
	}

	if (MaterialCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : Root.GetArrayField("materials"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupMaterial(Object);
		}
	}
}
