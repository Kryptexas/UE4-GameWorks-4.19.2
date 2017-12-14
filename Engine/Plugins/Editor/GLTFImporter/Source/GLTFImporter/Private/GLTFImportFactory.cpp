// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

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
		// Binary glTF files begin with a 12-byte header:
		// - magic bytes "glTF"
		// - format version
		// - size of this file

		FArchive* Reader = IFileManager::Get().CreateFileReader(*Filename);
		if (Reader)
		{
			const int64 FileSize = Reader->TotalSize();
			if (FileSize >= 12)
			{
				uint8 Magic[4];
				Reader->Serialize(Magic, 4);
				bool MagicOk = (Magic[0] == 'g' &&
				                Magic[1] == 'l' &&
				                Magic[2] == 'T' &&
				                Magic[3] == 'F');

				uint32 Version;
				Reader->SerializeInt(Version, MAX_uint32);
				bool VersionOk = Version == 2;

				uint32 Size;
				Reader->SerializeInt(Size, MAX_uint32);
				bool SizeOk = Size == FileSize;

				bCanImport = MagicOk && VersionOk && SizeOk;
			}

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
		// Binary glTF files begin with a 12-byte header:
		// - magic bytes "glTF"
		// - format version
		// - size of this file
		// followed by 1 chunk of JSON and (optionally) 1 chunk of binary data.

		FArchive* FileReader = IFileManager::Get().CreateFileReader(*Filename);
		if (FileReader)
		{
			const int64 FileSize = FileReader->TotalSize();

			// Can we count on glTF header being valid, since FactoryCanImport returned true?
			// If so, skip Magic and Version checks.
			// Update: We get here even if FactoryCanImport returns false :/

			if (FileSize >= 12)
			{
				uint8 Magic[4];
				FileReader->Serialize(Magic, 4);
				bool MagicOk = (Magic[0] == 'g' &&
				                Magic[1] == 'l' &&
				                Magic[2] == 'T' &&
				                Magic[3] == 'F');

				uint32 Version;
				FileReader->SerializeInt(Version, MAX_uint32);
				bool VersionOk = Version == 2;

				uint32 ReportedSize;
				FileReader->SerializeInt(ReportedSize, MAX_uint32);
				bool SizeOk = ReportedSize == FileSize;

				if (MagicOk && VersionOk && SizeOk)
				{
					uint32 ChunkSize;
					FileReader->SerializeInt(ChunkSize, MAX_uint32);

					uint8 ChunkType[4];
					FileReader->Serialize(ChunkType, 4);
					bool ChunkTypeOk = (ChunkType[0] == 'J' &&
					                    ChunkType[1] == 'S' &&
					                    ChunkType[2] == 'O' &&
					                    ChunkType[3] == 'N');

					bool ChunkSizeOk = ChunkSize <= (FileSize - FileReader->Tell());

					// Get JSON chunk 0 for later parsing
					// JSON reader fails when it hits the first non-ASCII byte (BIN chunk size),
					// so we must isolate the JSON chunk.
					FArrayReader JSON;
					JSON.AddUninitialized(ChunkSize);
					FileReader->Serialize(JSON.GetData(), ChunkSize);

					// Get non-JSON chunk 1 if present
					TArray<uint8> PackedBinData;

					//                  _ GLB header
					//                 /    _ JSON chunk type + size
					//                /    /           _ JSON data
					//               /    /           /         _ BIN chunk type + size 
					//              /    /           /         /
					if (FileSize > 12 + 8 + Pad4(ChunkSize) + 8) // anything after this is BIN data
					{
						// Plenty of room for post-JSON data...

						// Skip past inter-chunk padding.
						if (ChunkSize < Pad4(ChunkSize))
						{
							FileReader->Seek(12 + 8 + Pad4(ChunkSize));
						}

						// get BIN ChunkSize
						FileReader->SerializeInt(ChunkSize, MAX_uint32);

						// verify ChunkType == "BIN"
						FileReader->Serialize(ChunkType, 4);
						ChunkTypeOk = (ChunkType[0] == 'B' &&
						               ChunkType[1] == 'I' &&
						               ChunkType[2] == 'N' &&
						               ChunkType[3] == '\0');

						ChunkSizeOk = ChunkSize <= (FileSize - FileReader->Tell());

						if (ChunkTypeOk && ChunkSizeOk)
						{
							PackedBinData.AddUninitialized(ChunkSize);
							FileReader->Serialize(PackedBinData.GetData(), ChunkSize);
							// Could get this data asynchronously while parsing JSON...
							// Buffer.ByteLength needs to be in place while building Asset;
							// contents of buffer can wait until just before *using* Asset.
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
