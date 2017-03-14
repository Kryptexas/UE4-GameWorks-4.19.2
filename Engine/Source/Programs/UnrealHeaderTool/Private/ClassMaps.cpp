// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClassMaps.h"
#include "UnrealHeaderTool.h"

#include "UnrealTypeDefinitionInfo.h"
#include "UHTMakefile/UHTMakefile.h"

TMap<FString, TSharedRef<FUnrealSourceFile> > GUnrealSourceFilesMap;
TMap<UField*, TSharedRef<FUnrealTypeDefinitionInfo> > GTypeDefinitionInfoMap;
TMap<UClass*, FString> GClassStrippedHeaderTextMap;
TMap<UClass*, FString> GClassHeaderNameWithNoPathMap;
TSet<FUnrealSourceFile*> GPublicSourceFileSet;
TMap<UProperty*, FString> GArrayDimensions;
TMap<UPackage*,  const FManifestModule*> GPackageToManifestModuleMap;
TMap<UField*, uint32> GGeneratedCodeCRCs;
TMap<UEnum*,  EUnderlyingEnumType> GEnumUnderlyingTypes;
TMap<FName, TSharedRef<FClassDeclarationMetaData> > GClassDeclarations;
TSet<UProperty*> GUnsizedProperties;

TSharedRef<FUnrealTypeDefinitionInfo> AddTypeDefinition(FUHTMakefile& UHTMakefile, FUnrealSourceFile* SourceFile, UField* Field, int32 Line)
{
	FUnrealTypeDefinitionInfo* UnrealTypeDefinitionInfo = new FUnrealTypeDefinitionInfo(*SourceFile, Line);
	UHTMakefile.AddUnrealTypeDefinitionInfo(SourceFile, UnrealTypeDefinitionInfo);

	TSharedRef<FUnrealTypeDefinitionInfo> DefinitionInfo = MakeShareable(UnrealTypeDefinitionInfo);
	UHTMakefile.AddTypeDefinitionInfoMapEntry(SourceFile, Field, UnrealTypeDefinitionInfo);
	GTypeDefinitionInfoMap.Add(Field, DefinitionInfo);

	return DefinitionInfo;
}
