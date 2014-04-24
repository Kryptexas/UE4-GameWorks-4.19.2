// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"

#include "ClassMaps.h"

TMap<UClass*, FString>                   GClassStrippedHeaderTextMap;
TMap<UClass*, FString>                   GClassSourceFileMap;
TMap<UClass*, TUniqueObj<TArray<FName>>> GClassDependentOnMap;
TMap<UClass*, FString>                   GClassHeaderFilenameMap;
TMap<UClass*, FString>                   GClassHeaderNameWithNoPathMap;
TMap<UClass*, FString>                   GClassModuleRelativePathMap;
TMap<UClass*, FString>                   GClassIncludePathMap;
TSet<UClass*>                            GPublicClassSet;
TSet<UClass*>                            GExportedClasses;
TMap<UProperty*, FString>                GArrayDimensions;
