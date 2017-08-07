// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Misc/StringAssetReference.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/UnrealType.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"
#include "UObject/LinkerLoad.h"
#include "UObject/UObjectThreadContext.h"
#include "Misc/RedirectCollector.h"

FStringAssetReference::FStringAssetReference(const UObject* InObject)
{
	if (InObject)
	{
		SetPath(InObject->GetPathName());
	}
}

void FStringAssetReference::SetPath(FString Path)
{
	if (Path.IsEmpty())
	{
		// Empty path, just empty the pathname.
		AssetLongPathname.Empty();
	}
	else if (FPackageName::IsShortPackageName(Path))
	{
		// Short package name. Try to expand it.
		AssetLongPathname = FPackageName::GetNormalizedObjectPath(Path); 
	}
	else if (Path[0] != '/')
	{
		// Possibly an ExportText path. Trim the ClassName.
		AssetLongPathname = FPackageName::ExportTextPathToObjectPath(Path);
	}
	else
	{
		// Normal object path. Fast path set directly.
		AssetLongPathname = MoveTemp(Path);
	}
}

bool FStringAssetReference::PreSavePath()
{
#if WITH_EDITOR
	FString* FoundRedirection = GRedirectCollector.GetAssetPathRedirection(ToString());

	if (FoundRedirection)
	{
		SetPath(*FoundRedirection);
		return true;
	}
#endif // WITH_EDITOR
	return false;
}

void FStringAssetReference::PostLoadPath() const
{
#if WITH_EDITOR
	GRedirectCollector.OnStringAssetReferenceLoaded(ToString());
#endif // WITH_EDITOR
}

bool FStringAssetReference::Serialize(FArchive& Ar)
{
	// Archivers will call back into SerializePath for the various fixups
	Ar << *this;

	return true;
}

void FStringAssetReference::SerializePath(FArchive& Ar, bool bSkipSerializeIfArchiveHasSize)
{
#if WITH_EDITOR
	if (Ar.IsSaving())
	{
		PreSavePath();
	}
#endif // WITH_EDITOR

	FString Path = ToString();

	if (!bSkipSerializeIfArchiveHasSize || Ar.IsObjectReferenceCollector() || Ar.Tell() < 0)
	{
		Ar << Path;
	}

	if (Ar.IsLoading())
	{
		if (Ar.UE4Ver() < VER_UE4_KEEP_ONLY_PACKAGE_NAMES_IN_STRING_ASSET_REFERENCES_MAP)
		{
			Path = FPackageName::GetNormalizedObjectPath(Path);
		}
		
		SetPath(MoveTemp(Path));
		
#if WITH_EDITOR
		if (Ar.IsPersistent())
		{
			PostLoadPath();
		}
		if (Ar.GetPortFlags()&PPF_DuplicateForPIE)
		{
			// Remap unique ID if necessary
			// only for fixing up cross-level references, inter-level references handled in FDuplicateDataReader
			FixupForPIE();
		}
#endif // WITH_EDITOR
	}
}

bool FStringAssetReference::operator==(FStringAssetReference const& Other) const
{
	return ToString() == Other.ToString();
}

FStringAssetReference& FStringAssetReference::operator=(FStringAssetReference const& Other)
{
	SetPath(Other.ToString());
	return *this;
}

bool FStringAssetReference::ExportTextItem(FString& ValueStr, FStringAssetReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	if (0 != (PortFlags & EPropertyPortFlags::PPF_ExportCpp))
	{
		return false;
	}

	if (IsValid())
	{
		// Fixup any redirectors
		FStringAssetReference Temp = *this;
		Temp.PreSavePath();

		ValueStr += Temp.ToString();
	}
	else
	{
		ValueStr += TEXT("None");
	}
	return true;
}

bool FStringAssetReference::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	FString ImportedPath = TEXT("");
	const TCHAR* NewBuffer = UPropertyHelpers::ReadToken(Buffer, ImportedPath, 1);
	if (!NewBuffer)
	{
		return false;
	}
	Buffer = NewBuffer;
	if (ImportedPath == TEXT("None"))
	{
		ImportedPath = TEXT("");
	}
	else
	{
		if (*Buffer == TCHAR('\''))
		{
			// A ' token likely means we're looking at an asset string in the form "Texture2d'/Game/UI/HUD/Actions/Barrel'" and we need to read and append the path part
			// We have to skip over the first ' as UPropertyHelpers::ReadToken doesn't read single-quoted strings correctly, but does read a path correctly
			Buffer++; // Skip the leading '
			ImportedPath.Reset();
			NewBuffer = UPropertyHelpers::ReadToken(Buffer, ImportedPath, 1);
			if (!NewBuffer)
			{
				return false;
			}
			Buffer = NewBuffer;
			if (*Buffer++ != TCHAR('\''))
			{
				return false;
			}
		}
	}

	SetPath(MoveTemp(ImportedPath));

	// Consider this a load, so Config string asset references get cooked
	PostLoadPath();

	return true;
}

#include "Misc/StringReferenceTemplates.h"

bool FStringAssetReference::SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar)
{
	struct UObjectTypePolicy
	{
		typedef UObject Type;
		static const FName FORCEINLINE GetTypeName() { return NAME_ObjectProperty; }
	};

	FString Path = ToString();

	bool bReturn = SerializeFromMismatchedTagTemplate<UObjectTypePolicy>(Path, Tag, Ar);

	if (Ar.IsLoading())
	{
		SetPath(MoveTemp(Path));
		PostLoadPath();
	}

	return bReturn;
}

UObject* FStringAssetReference::TryLoad() const
{
	UObject* LoadedObject = nullptr;

	if ( IsValid() )
	{
		LoadedObject = LoadObject<UObject>(nullptr, *ToString());
		while (UObjectRedirector* Redirector = Cast<UObjectRedirector>(LoadedObject))
		{
			LoadedObject = Redirector->DestinationObject;
		}
	}

	return LoadedObject;
}

UObject* FStringAssetReference::ResolveObject() const
{
	// Don't try to resolve if we're saving a package because StaticFindObject can't be used here
	// and we usually don't want to force references to weak pointers while saving.
	if (!IsValid() || GIsSavingPackage)
	{
		return nullptr;
	}

	UObject* FoundObject = FindObject<UObject>(nullptr, *ToString());
	while (UObjectRedirector* Redirector = Cast<UObjectRedirector>(FoundObject))
	{
		FoundObject = Redirector->DestinationObject;
	}

	return FoundObject;
}

FStringAssetReference FStringAssetReference::GetOrCreateIDForObject(const class UObject *Object)
{
	check(Object);
	return FStringAssetReference(Object);
}

void FStringAssetReference::SetPackageNamesBeingDuplicatedForPIE(const TArray<FString>& InPackageNamesBeingDuplicatedForPIE)
{
	PackageNamesBeingDuplicatedForPIE = InPackageNamesBeingDuplicatedForPIE;
}

void FStringAssetReference::ClearPackageNamesBeingDuplicatedForPIE()
{
	PackageNamesBeingDuplicatedForPIE.Empty();
}

void FStringAssetReference::FixupForPIE()
{
	if (FPlatformProperties::HasEditorOnlyData() && IsValid())
	{
		FString Path = ToString();

		// Determine if this reference has already been fixed up for PIE
		const FString ShortPackageOuterAndName = FPackageName::GetLongPackageAssetName(Path);
		if (!ShortPackageOuterAndName.StartsWith(PLAYWORLD_PACKAGE_PREFIX))
		{
			check(GPlayInEditorID != -1);

			FString PIEPath = FString::Printf(TEXT("%s/%s_%d_%s"), *FPackageName::GetLongPackagePath(Path), PLAYWORLD_PACKAGE_PREFIX, GPlayInEditorID, *ShortPackageOuterAndName);

			// Determine if this refers to a package that is being duplicated for PIE
			for (auto PackageNameIt = PackageNamesBeingDuplicatedForPIE.CreateConstIterator(); PackageNameIt; ++PackageNameIt)
			{
				const FString PathPrefix = (*PackageNameIt) + TEXT(".");
				if (PIEPath.StartsWith(PathPrefix))
				{
					// Need to prepend PIE prefix, as we're in PIE and this refers to an object in a PIE package
					SetPath(MoveTemp(PIEPath));

					break;
				}
			}
		}
	}
}

bool FStringAssetReferenceThreadContext::GetSerializationOptions(FName& OutPackageName, FName& OutPropertyName, EStringAssetReferenceCollectType& OutCollectType) const
{
	FName CurrentPackageName, CurrentPropertyName;
	EStringAssetReferenceCollectType CurrentCollectType = EStringAssetReferenceCollectType::AlwaysCollect;
	bool bFoundAnything = false;
	if (OptionStack.Num() > 0)
	{
		// Go from the top of the stack down
		for (int32 i = OptionStack.Num() - 1; i >= 0; i--)
		{
			const FSerializationOptions& Options = OptionStack[i];
			// Find first valid package/property names. They may not necessarily match
			if (Options.PackageName != NAME_None && CurrentPackageName == NAME_None)
			{
				CurrentPackageName = Options.PackageName;
			}
			if (Options.PropertyName != NAME_None && CurrentPropertyName == NAME_None)
			{
				CurrentPropertyName = Options.PropertyName;
			}

			// Restrict based on lowest/most restrictive collect type
			if (Options.CollectType < CurrentCollectType)
			{
				CurrentCollectType = Options.CollectType;
			}
		}

		bFoundAnything = true;
	}
	
	// Check UObject thread context as a backup
	FUObjectThreadContext& ThreadContext = FUObjectThreadContext::Get();
	if (ThreadContext.SerializedObject)
	{
		FLinkerLoad* Linker = ThreadContext.SerializedObject->GetLinker();
		if (Linker)
		{
			if (CurrentPackageName == NAME_None)
			{
				CurrentPackageName = FName(*FPackageName::FilenameToLongPackageName(Linker->Filename));
			}
			if (Linker->GetSerializedProperty() && CurrentPropertyName == NAME_None)
			{
				CurrentPropertyName = Linker->GetSerializedProperty()->GetFName();
			}
#if WITH_EDITORONLY_DATA
			bool bEditorOnly = Linker->IsEditorOnlyPropertyOnTheStack();
#else
			bool bEditorOnly = false;
#endif
			// If we were always collect before and not overridden by stack options, set to editor only
			if (bEditorOnly && CurrentCollectType == EStringAssetReferenceCollectType::AlwaysCollect)
			{
				CurrentCollectType = EStringAssetReferenceCollectType::EditorOnlyCollect;
			}

			bFoundAnything = true;
		}
	}

	if (bFoundAnything)
	{
		OutPackageName = CurrentPackageName;
		OutPropertyName = CurrentPropertyName;
		OutCollectType = CurrentCollectType;
		return true;
	}
	
	return bFoundAnything;
}

FThreadSafeCounter FStringAssetReference::CurrentTag(1);
TArray<FString> FStringAssetReference::PackageNamesBeingDuplicatedForPIE;
