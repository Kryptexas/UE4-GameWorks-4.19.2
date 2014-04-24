#include "UnrealVersionSelector.h"
#include "PlatformInstallation.h"

int32 ParseReleaseVersion(const FString &Version)
{
	TCHAR *End;

	uint64 Major = FCString::Strtoui64(*Version, &End, 10);
	if (Major >= MAX_int16 || *(End++) != '.')
	{
		return INDEX_NONE;
	}

	uint64 Minor = FCString::Strtoui64(End, &End, 10);
	if (Minor >= MAX_int16 || *End != 0)
	{
		return INDEX_NONE;
	}

	return (Major << 16) + Minor;
}

bool GetDefaultEngineId(FString &OutId)
{
	TMap<FString, FString> Installations;
	FPlatformInstallation::EnumerateEngineInstallations(Installations);

	bool bRes = false;
	if (Installations.Num() > 0)
	{
		// Default to the first install
		OutId = TMap<FString, FString>::TConstIterator(Installations).Key();

		// Try to find the highest versioned install
		int32 BestVersion = INDEX_NONE;
		for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
		{
			int32 Version = ParseReleaseVersion(Iter.Key());
			if (Version > BestVersion)
			{
				OutId = Iter.Key();
				BestVersion = Version;
			}
		}
	}
	return bRes;
}

bool GetDefaultEngineRootDir(FString &OutDirName)
{
	FString Id;
	return GetDefaultEngineId(Id) && GetEngineRootDirFromId(Id, OutDirName);
}

bool GetEngineRootDirFromId(const FString &Id, FString &OutDirName)
{
	TMap<FString, FString> Installations;
	FPlatformInstallation::EnumerateEngineInstallations(Installations);

	const FString *DirName = Installations.Find(Id);
	if (DirName != NULL)
	{
		OutDirName = *DirName;
		return true;
	}

	return false;
}

bool GetEngineIdFromRootDir(const FString &DirName, FString &OutId)
{
	TMap<FString, FString> Installations;
	FPlatformInstallation::EnumerateEngineInstallations(Installations);

	// Find the label for the given directory
	for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
	{
		if (Iter.Value() == DirName)
		{
			OutId = Iter.Key();
			return true;
		}
	}
	return false;
}

