// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GLTFImportFactory.h"
#include "GLTFReader.h"
#include "GLTFStaticMesh.h"
#include "GLTFMaterial.h"

#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/StaticMesh.h"
#include "RawMesh.h"
#include "Serialization/ArrayReader.h"
#include "Editor/UnrealEd/Public/Editor.h"

#define LOCTEXT_NAMESPACE "GLTFFactory"

UGLTFImportFactory::UGLTFImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true; // binary / general file source
	bText = false; // text source (maybe support?)

	SupportedClass = UStaticMesh::StaticClass();

	Formats.Add(TEXT("gltf;GL Transmission Format"));
	Formats.Add(TEXT("glb;GL Transmission Format (Binary)"));
}

static bool SignatureMatches(uint32 Signature, const char* ExpectedSignature)
{
	return Signature == *(const uint32*)ExpectedSignature;
}

static bool IsHeaderValid(FArchive* Archive)
{
	// Binary glTF files begin with a 12-byte header:
	// - magic bytes "glTF"
	// - format version
	// - size of this file

	const int64 FileSize = Archive->TotalSize();
	if (FileSize < 12)
	{
		return false;
	}

	uint32 Magic;
	Archive->SerializeInt(Magic, MAX_uint32);
	bool MagicOk = SignatureMatches(Magic, "glTF");

	uint32 Version;
	Archive->SerializeInt(Version, MAX_uint32);
	bool VersionOk = Version == 2;

	uint32 Size;
	Archive->SerializeInt(Size, MAX_uint32);
	bool SizeOk = Size == FileSize;

	return MagicOk && VersionOk && SizeOk;
}

bool UGLTFImportFactory::FactoryCanImport(const FString& Filename)
{
	bool bCanImport = false;

	const FString Extension = FPaths::GetExtension(Filename);
	if (Extension == TEXT("gltf"))
	{
		// File contains standard JSON with no magic number.
		bCanImport = true;
	}
	else if (Extension == TEXT("glb"))
	{
		FArchive* Reader = IFileManager::Get().CreateFileReader(*Filename);
		if (Reader)
		{
			bCanImport = IsHeaderValid(Reader);

			Reader->Close();
			delete Reader;
		}
	}

	return bCanImport;
}

static UStaticMesh* ImportMeshesAndMaterialsFromJSON(FArchive* Archive, const FString& Path, UObject* InParent, FName InName, EObjectFlags Flags, FFeedbackContext* Warn, TArray<uint8> PackedBinData = TArray<uint8>())
{
	UStaticMesh* StaticMesh = nullptr;

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	TSharedRef<TJsonReader<char>> JsonReader = TJsonReader<char>::Create(Archive);
	// Actually it's UTF-8 (pure ASCII for glTF-defined strings, UTF-8 for app-defined names)

	bool JsonOk = FJsonSerializer::Deserialize(JsonReader, Root);

	if (JsonOk)
	{
		// Build glTF asset from JSON
		GLTF::FAsset Asset;
		FGLTFReader GLTFReader(Asset, *Root, Path, *Warn, PackedBinData);

		// Import materials first so new StaticMeshes can refer to them
		const TArray<UMaterial*> Materials = ImportMaterials(Asset, InParent, InName, Flags);

		// Build StaticMeshes from glTF asset
		const TArray<UStaticMesh*> StaticMeshes = ImportStaticMeshes(Asset, Materials, InParent, InName, Flags);
		if (StaticMeshes.Num() > 0)
		{
			StaticMesh = StaticMeshes[0];
		}
	}
	else
	{
		Warn->Log("Problem with JSON.");
	}

	return StaticMesh;
}

// Returns whether the current chunk is of the expected type
// OutData is filled only if these match
static bool ReadChunk(FArchive* Archive, bool& OutHasMoreData, TArray<uint8>& OutData, const char* ExpectedChunkType)
{
	// Align to next 4-byte boundary before reading anything
	uint32 Offset = Archive->Tell();
	uint32 AlignedOffset = Pad4(Offset);
	if (Offset != AlignedOffset)
	{
		Archive->Seek(AlignedOffset);
	}

	// Each chunk has the form [Size][Type][...Data...]
	uint32 ChunkType, ChunkDataSize;
	Archive->SerializeInt(ChunkDataSize, MAX_uint32);
	Archive->SerializeInt(ChunkType, MAX_uint32);

	constexpr uint32 ChunkHeaderSize = 8;
	const uint32 AvailableData = Archive->TotalSize() - (AlignedOffset + ChunkHeaderSize);

	// Is there room for another chunk after this one?
	OutHasMoreData = AvailableData - Pad4(ChunkDataSize) >= ChunkHeaderSize;

	// Is there room for this chunk's data? (should always be true)
	if (ChunkDataSize > AvailableData)
	{
		return false;
	}

	if (SignatureMatches(ChunkType, ExpectedChunkType))
	{
		// Read this chunk's data
		OutData.SetNumUninitialized(ChunkDataSize, true);
		Archive->Serialize(OutData.GetData(), ChunkDataSize);
		return true;
	}
	else
	{
		// Skip past this chunk's data
		Archive->Seek(AlignedOffset + ChunkHeaderSize + ChunkDataSize);
		return false;
	}
}

UObject* UGLTFImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UStaticMesh* StaticMesh = nullptr;

	FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Parms);

	Warn->Log(Filename);
	const FString Extension = FPaths::GetExtension(Filename);
	const FString Path = FPaths::GetPath(Filename);

	if (Extension == TEXT("gltf"))
	{
		// File contains standard JSON
		FArchive* FileReader = IFileManager::Get().CreateFileReader(*Filename);
		if (FileReader)
		{
			StaticMesh = ImportMeshesAndMaterialsFromJSON(FileReader, Path, InParent, InName, Flags, Warn);
		}
		FileReader->Close();
		delete FileReader;
	}
	else if (Extension == TEXT("glb"))
	{
		// Binary glTF files begin with a 12-byte header
		// followed by 1 chunk of JSON and (optionally) 1 chunk of binary data.

		FArchive* FileReader = IFileManager::Get().CreateFileReader(*Filename);
		if (FileReader)
		{
			// Can we count on glTF header being valid, since FactoryCanImport returned true?
			// If so, skip Magic and Version checks.
			// Update: We get here even if FactoryCanImport returns false :/

			if (IsHeaderValid(FileReader))
			{
				// Get JSON chunk for later parsing (always first in the file)
				// JSON reader fails when it hits the first non-ASCII byte (BIN chunk size),
				// so we must isolate the JSON chunk.

				FArrayReader JSON;
				bool HasMoreData;

				if (ReadChunk(FileReader, HasMoreData, JSON, "JSON"))
				{
					// Get BIN chunk if present
					TArray<uint8> PackedBinData;

					while (HasMoreData)
					{
						if (ReadChunk(FileReader, HasMoreData, PackedBinData, "BIN"))
						{
							break;
						}
					}

					StaticMesh = ImportMeshesAndMaterialsFromJSON(&JSON, Path, InParent, InName, Flags, Warn, PackedBinData);
				}
			}

			FileReader->Close();
			delete FileReader;
		}
	}

	// Is this necessary? Should call for all new assets, not just the first StaticMesh?
	FEditorDelegates::OnAssetPostImport.Broadcast(this, StaticMesh);

	return StaticMesh;
}

#undef LOCTEXT_NAMESPACE
