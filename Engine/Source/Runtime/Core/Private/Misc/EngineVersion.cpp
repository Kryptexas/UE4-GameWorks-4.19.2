// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "EngineVersion.h"
#include "Runtime/Launch/Resources/Version.h"

FEngineVersion::FEngineVersion()
{
	Empty();
}

FEngineVersion::FEngineVersion(uint16 InMajor, uint16 InMinor, uint16 InPatch, uint32 InChangelist, const FString &InBranch)
{
	Set(InMajor, InMinor, InPatch, InChangelist, InBranch);
}

void FEngineVersion::Set(uint16 InMajor, uint16 InMinor, uint16 InPatch, uint32 InChangelist, const FString &InBranch)
{
	Major = InMajor;
	Minor = InMinor;
	Patch = InPatch;
	Changelist = InChangelist;
	Branch = InBranch;
}

uint32 FEngineVersion::GetChangelist() const
{
	return Changelist;
}

void FEngineVersion::Empty()
{
	Set(0, 0, 0, 0, FString());
}

bool FEngineVersion::IsEmpty() const
{
	return Major == 0 && Minor == 0 && Patch == 0;
}

bool FEngineVersion::IsPromotedBuild() const
{
	return Changelist != 0;
}

bool FEngineVersion::IsCompatibleWith(const FEngineVersion &Other) const
{
	// If this is not a promoted build, always assume compatibility. 
	if(!IsPromotedBuild())
	{
		return true;
	}

	// Otherwise compare the versions
	EVersionComponent::Type Component;
	EVersionComparison::Type Comparison = FEngineVersion::GetNewest(*this, Other, &Component);

	// If this engine version is the same or newer, it's definitely compatible
	if(Comparison == EVersionComparison::Neither || Comparison == EVersionComparison::First)
	{
		return true;
	}

	// Otherwise check if we require a strict version match or just a major/minor version match.
	if (Component != EVersionComponent::Major && Component != EVersionComponent::Minor && FRocketSupport::IsRocket())
	{
		return true;
	}
	else
	{
		return false;
	}
}

FString FEngineVersion::ToString(EVersionComponent::Type LastComponent) const
{
	FString Result = FString::Printf(TEXT("%u"), Major);
	if(LastComponent >= EVersionComponent::Major)
	{
		Result += FString::Printf(TEXT(".%u"), Minor);
		if(LastComponent >= EVersionComponent::Patch)
		{
			Result += FString::Printf(TEXT(".%u"), Patch);
			if(LastComponent >= EVersionComponent::Changelist)
			{
				Result += FString::Printf(TEXT("-%u"), Changelist);
				if(LastComponent >= EVersionComponent::Branch && Branch.Len() > 0)
				{
					Result += FString::Printf(TEXT("+%s"), *Branch);
				}
			}
		}
	}
	return Result;
}

bool FEngineVersion::Parse(const FString &Text, FEngineVersion &OutVersion)
{
	TCHAR *End;

	// Read the major/minor/patch numbers
	uint64 Major = FCString::Strtoui64(*Text, &End, 10);
	if(Major > MAX_uint16 || *(End++) != '.') return false;

	uint64 Minor = FCString::Strtoui64(End, &End, 10);
	if(Minor > MAX_uint16 || *(End++) != '.') return false;

	uint64 Patch = FCString::Strtoui64(End, &End, 10);
	if(Patch > MAX_uint16) return false;

	// Read the optional changelist number
	uint64 Changelist = 0;
	if(*End == '-')
	{
		End++;
		Changelist = FCString::Strtoui64(End, &End, 10);
		if(Changelist > MAX_uint32) return false;
	}

	// Read the optional branch name
	FString Branch;
	if(*End == '+')
	{
		End++;
		// read to the end of the string. There's no standard for the branch name to verify.
		Branch = FString(End);
	}

	// Build the output version
	OutVersion.Set(Major, Minor, Patch, Changelist, Branch);
	return true;
}

EVersionComparison::Type FEngineVersion::GetNewest(const FEngineVersion &First, const FEngineVersion &Second, EVersionComponent::Type *OutComponent)
{
	// Compare major versions
	if(First.Major != Second.Major)
	{
		*OutComponent = EVersionComponent::Major;
		return (First.Major > Second.Major)? EVersionComparison::First : EVersionComparison::Second;
	}

	// Compare minor versions
	if(First.Minor != Second.Minor)
	{
		*OutComponent = EVersionComponent::Minor;
		return (First.Minor > Second.Minor)? EVersionComparison::First : EVersionComparison::Second;
	}

	// Compare patch versions
	if(First.Patch != Second.Patch)
	{
		*OutComponent = EVersionComponent::Patch;
		return (First.Patch > Second.Patch)? EVersionComparison::First : EVersionComparison::Second;
	}

	// Compare changelist versions
	if(First.Changelist != Second.Changelist)
	{
		*OutComponent = EVersionComponent::Changelist;
		return (First.Changelist > Second.Changelist)? EVersionComparison::First : EVersionComparison::Second;
	}

	// Otherwise they're the same
	return EVersionComparison::Neither;
}

void operator<<(FArchive &Ar, FEngineVersion &Version)
{
	Ar << Version.Major;
	Ar << Version.Minor;
	Ar << Version.Patch;
	Ar << Version.Changelist;
	Ar << Version.Branch;
}

// Global instance of the current engine version
const FEngineVersion GEngineVersion(ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION, BUILT_FROM_CHANGELIST, BRANCH_NAME);
