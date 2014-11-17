// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"

#include "ClassMaps.h"

TMap<UClass*, FString>                   GClassStrippedHeaderTextMap;
TMap<UClass*, FString>                   GClassSourceFileMap;
TMap<UClass*, int32>                     GClassDeclarationLineNumber;
TMap<UClass*, FClassHeaderInfo>          GClassGeneratedFileMap;
TMap<UClass*, TUniqueObj<TArray<FName>>> GClassDependentOnMap;
TMap<UClass*, FString>                   GClassHeaderNameWithNoPathMap;
TMap<UClass*, FString>                   GClassModuleRelativePathMap;
TMap<UClass*, FString>                   GClassIncludePathMap;
TSet<UClass*>                            GPublicClassSet;
TSet<UClass*>                            GExportedClasses;
TMap<UProperty*, FString>                GArrayDimensions;
TMap<UPackage*,  const FManifestModule*> GPackageToManifestModuleMap;
