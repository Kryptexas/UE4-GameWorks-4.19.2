// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealHeaderTool.h"
#include "UHTMakefile/UHTMakefile.h"
#include "UHTMakefile/NameLookupCPPArchiveProxy.h"

FNameLookupCPPArchiveProxy::FNameLookupCPPArchiveProxy(const FUHTMakefile& UHTMakefile, const FNameLookupCPP* NameLookupCPP)
{
	TArray<TPair<FSerializeIndex, FString>> StructNameMap;
	StructNameMap.Empty(NameLookupCPP->StructNameMap.Num());
	for (auto& Kvp : NameLookupCPP->StructNameMap)
	{
		StructNameMap.Add(TPairInitializer<FSerializeIndex, FString>(UHTMakefile.GetStructIndex(Kvp.Key), Kvp.Value));
	}
	TArray<FString> InterfaceAllocations;
	InterfaceAllocations.Empty(NameLookupCPP->InterfaceAllocations.Num());
	for (TCHAR* InterfaceAllocation : NameLookupCPP->InterfaceAllocations)
	{
		InterfaceAllocations.Add(InterfaceAllocation);
	}
}

FArchive& operator<<(FArchive& Ar, FNameLookupCPPArchiveProxy& NameLookupCPPArchiveProxy)
{
	Ar << NameLookupCPPArchiveProxy.InterfaceAllocations;
	Ar << NameLookupCPPArchiveProxy.StructNameMap;

	return Ar;
}

void FNameLookupCPPArchiveProxy::Resolve(FNameLookupCPP* NameLookupCPP, FUHTMakefile& UHTMakefile)
{

}
