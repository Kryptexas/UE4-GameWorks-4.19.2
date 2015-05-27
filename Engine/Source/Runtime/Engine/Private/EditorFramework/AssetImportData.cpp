// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "EditorFramework/AssetImportData.h"

// This whole class is compiled out in non-editor
UAssetImportData::UAssetImportData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	bDirty = false;
#endif
}

#if WITH_EDITORONLY_DATA

FOnImportDataChanged UAssetImportData::OnImportDataChanged;

FString FAssetImportInfo::ToJson() const
{
	FString Json;
	Json.Reserve(1024);
	Json += TEXT("[");

	for (int32 Index = 0; Index < SourceFiles.Num(); ++Index)
	{
		Json += FString::Printf(TEXT("{ \"RelativeFilename\" : \"%s\", \"Timestamp\" : %d, \"FileMD5\" : \"%s\" }"),
			*SourceFiles[Index].RelativeFilename,
			SourceFiles[Index].Timestamp.ToUnixTimestamp(),
			*LexicalConversion::ToString(SourceFiles[Index].FileHash)
			);

		if (Index != SourceFiles.Num() - 1)
		{
			Json += TEXT(",");
		}
	}

	Json += TEXT("]");
	return Json;
}

TOptional<FAssetImportInfo> FAssetImportInfo::FromJson(FString InJsonString)
{
	// Load json
	TSharedRef<TJsonReader<TCHAR>> Reader = FJsonStringReader::Create(MoveTemp(InJsonString));

	TArray<TSharedPtr<FJsonValue>> JSONSourceFiles;
	if (!FJsonSerializer::Deserialize(Reader, JSONSourceFiles))
	{
		return TOptional<FAssetImportInfo>();
	}
	
	FAssetImportInfo Info;

	for (const auto& Value : JSONSourceFiles)
	{
		const TSharedPtr<FJsonObject>& SourceFile = Value->AsObject();
		if (!SourceFile.IsValid())
		{
			continue;
		}

		FString RelativeFilename, TimestampString, MD5String;
		SourceFile->TryGetStringField("RelativeFilename", RelativeFilename);
		SourceFile->TryGetStringField("Timestamp", TimestampString);
		SourceFile->TryGetStringField("FileMD5", MD5String);

		if (RelativeFilename.IsEmpty())
		{
			continue;
		}

		int64 UnixTimestamp = 0;
		LexicalConversion::FromString(UnixTimestamp, *TimestampString);

		FMD5Hash FileHash;
		LexicalConversion::FromString(FileHash, *MD5String);

		Info.SourceFiles.Emplace(MoveTemp(RelativeFilename), UnixTimestamp, FileHash);
	}

	return Info;
}

void UAssetImportData::UpdateFilenameOnly(const FString& InPath)
{
	SourceFiles.Reset();
	SourceFiles.Emplace(SanitizeImportFilename(InPath));
}

void UAssetImportData::Update(const FString& InPath)
{
	FAssetImportInfo Old;
	Old.CopyFrom(*this);

	SourceFiles.Reset();
	SourceFiles.Emplace(
		SanitizeImportFilename(InPath),
		IFileManager::Get().GetTimeStamp(*InPath),
		FMD5Hash::HashFile(*InPath)
		);

	bDirty = true;
	
	OnImportDataChanged.Broadcast(Old, this);
}

FString UAssetImportData::GetFirstFilename() const
{
	return SourceFiles.Num() > 0 ? ResolveImportFilename(SourceFiles[0].RelativeFilename) : FString();
}

void UAssetImportData::ExtractFilenames(TArray<FString>& AbsoluteFilenames) const
{
	for (const auto& File : SourceFiles)
	{
		AbsoluteFilenames.Add(ResolveImportFilename(File.RelativeFilename));
	}
}

FString UAssetImportData::SanitizeImportFilename(const FString& InPath) const
{
	const UPackage* Package = GetOutermost();
	if (Package)
	{
		const bool		bIncludeDot = true;
		const FString	PackagePath	= Package->GetPathName();
		const FName		MountPoint	= FPackageName::GetPackageMountPoint(PackagePath);
		const FString	PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPaths::GetExtension(InPath, bIncludeDot));
		const FString	AbsolutePath = FPaths::ConvertRelativePathToFull(InPath);

		if ((MountPoint == FName("Engine") && AbsolutePath.StartsWith(FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()))) ||
			(MountPoint == FName("Game") &&	AbsolutePath.StartsWith(FPaths::ConvertRelativePathToFull(FPaths::GameDir()))))
		{
			FString RelativePath = InPath;
			FPaths::MakePathRelativeTo(RelativePath, *PackageFilename);
			return RelativePath;
		}
	}

	return IFileManager::Get().ConvertToRelativePath(*InPath);
}

FString UAssetImportData::ResolveImportFilename(const FString& InRelativePath, const UPackage* Outermost)
{
	FString RelativePath = InRelativePath;

	if (Outermost)
	{
		// Relative to the package filename?
		const FString PathRelativeToPackage = FPaths::GetPath(FPackageName::LongPackageNameToFilename(Outermost->GetPathName())) / InRelativePath;
		if (FPaths::FileExists(PathRelativeToPackage))
		{
			RelativePath = PathRelativeToPackage;
		}
	}

	// Convert relative paths
	return FPaths::ConvertRelativePathToFull(RelativePath);	
}

FString UAssetImportData::ResolveImportFilename(const FString& InRelativePath) const
{
	return ResolveImportFilename(InRelativePath, GetOutermost());
}

void UAssetImportData::Serialize(FArchive& Ar)
{
	if (Ar.UE4Ver() >= VER_UE4_ASSET_IMPORT_DATA_AS_JSON)
	{
		FString Json;
		if (Ar.IsLoading())
		{
			Ar << Json;
			TOptional<FAssetImportInfo> Copy = FromJson(MoveTemp(Json));
			if (Copy.IsSet())
			{
				CopyFrom(Copy.GetValue());
			}
		}
		else if (Ar.IsSaving())
		{
			Json = ToJson();
			Ar << Json;
		}
	}

	Super::Serialize(Ar);
}

void UAssetImportData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bDirty = true;
}

#endif // WITH_EDITORONLY_DATA