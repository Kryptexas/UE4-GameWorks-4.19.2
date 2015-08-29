// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CustomVersion.cpp: Unreal custom versioning system.
=============================================================================*/

#include "CorePrivatePCH.h"
#include "Serialization/CustomVersion.h"

namespace
{
	FCustomVersion UnusedCustomVersion(FGuid(0, 0, 0, 0xF99D40C1), 0, TEXT("Unused custom version"));

	struct FEnumCustomVersion_DEPRECATED
	{
		uint32 Tag;
		int32  Version;

		FCustomVersion ToCustomVersion() const
		{
			// We'll invent a GUID from three zeroes and the original tag
			return FCustomVersion(FGuid(0, 0, 0, Tag), Version, FString::Printf(TEXT("Old enum-based tag: %u"), Tag));
		}
	};

	FArchive& operator<<(FArchive& Ar, FEnumCustomVersion_DEPRECATED& Version)
	{
		// Serialize keys
		Ar << Version.Tag;
		Ar << Version.Version;

		return Ar;
	}
}

const FCustomVersionContainer& FCustomVersionContainer::GetRegistered()
{
	return GetInstance();
}

void FCustomVersionContainer::Empty()
{
	Versions.Empty();
}

FString FCustomVersionContainer::ToString(const FString& Indent) const
{
	FString VersionsAsString;
	for (int32 i=0; i<Versions.Num(); i++)
	{
		const FCustomVersion& SomeVersion = Versions[i];
		VersionsAsString += Indent;
		VersionsAsString += FString::Printf(TEXT("Key=%s  Version=%d  Friendly Name=%s \n"), *SomeVersion.Key.ToString(), SomeVersion.Version, *SomeVersion.FriendlyName );
	}

	return VersionsAsString;
}

FCustomVersionContainer& FCustomVersionContainer::GetInstance()
{
	static FCustomVersionContainer Singleton;

	return Singleton;
}

FArchive& operator<<(FArchive& Ar, FCustomVersion& Version)
{
	Ar << Version.Key;
	Ar << Version.Version;
	Ar << Version.FriendlyName;

	return Ar;
}

void FCustomVersionContainer::Serialize(FArchive& Ar, ECustomVersionSerializationFormat::Type Format)
{
	switch (Format)
	{
		default: check(false);

		case ECustomVersionSerializationFormat::Enums:
		{
			// We should only ever be loading enums.  They should never be saved - they only exist for backward compatibility.
			check(Ar.IsLoading());

			TArray<FEnumCustomVersion_DEPRECATED> OldTags;
			Ar << OldTags;

			Versions.Empty(OldTags.Num());
			for (auto It = OldTags.CreateConstIterator(); It; ++It)
			{
				Versions.Add(It->ToCustomVersion());
			}
		}
		break;

		case ECustomVersionSerializationFormat::Guids:
			Ar << Versions;
			break;
	}
}

const FCustomVersion* FCustomVersionContainer::GetVersion(FGuid Key) const
{
	// A testing tag was written out to a few archives during testing so we need to
	// handle the existence of it to ensure that those archives can still be loaded.
	if (Key == UnusedCustomVersion.Key)
		return &UnusedCustomVersion;

	return Versions.FindByKey(Key);
}

void FCustomVersionContainer::SetVersion(FGuid CustomKey, int32 Version, FString FriendlyName)
{
	if (CustomKey == UnusedCustomVersion.Key)
		return;

	if (FCustomVersion* Found = Versions.FindByKey(CustomKey))
	{
		Found->Version      = Version;
		Found->FriendlyName = MoveTemp(FriendlyName);
	}
	else
	{
		Versions.Add(FCustomVersion(CustomKey, Version, MoveTemp(FriendlyName)));
	}
}

FCustomVersionRegistration::FCustomVersionRegistration(FGuid InKey, int32 InVersion, const TCHAR* InFriendlyName)
{
	TArray<FCustomVersion>& Versions = FCustomVersionContainer::GetInstance().Versions;

	// Check if this tag hasn't already been registered
	if (FCustomVersion* ExistingRegistration = Versions.FindByKey(InKey))
	{
		// We don't allow the registration details to change across registrations - this code path only exists to support hotreload

		// If you hit this then you've probably either:
		// * Changed registration details during hotreload.
		// * Accidentally copy-and-pasted an FCustomVersionRegistration object.
		ensureMsgf(
			ExistingRegistration->Version == InVersion && ExistingRegistration->FriendlyName == InFriendlyName,
			TEXT("Custom version registrations cannot change between hotreloads - \"%s\" version %d is being reregistered as \"%s\" version %d"),
			*ExistingRegistration->FriendlyName,
			ExistingRegistration->Version,
			InFriendlyName,
			InVersion
		);

		++ExistingRegistration->ReferenceCount;
	}
	else
	{
		Versions.Add(FCustomVersion(InKey, InVersion, InFriendlyName));
	}

	Key = InKey;
}

FCustomVersionRegistration::~FCustomVersionRegistration()
{
	TArray<FCustomVersion>& Versions = FCustomVersionContainer::GetInstance().Versions;

	FCustomVersion* FoundKey = Versions.FindByKey(Key);

	// Ensure this tag has been registered
	check(FoundKey);

	--FoundKey->ReferenceCount;
	if (FoundKey->ReferenceCount == 0)
	{
		Versions.RemoveAt(FoundKey - Versions.GetData());
	}
}
