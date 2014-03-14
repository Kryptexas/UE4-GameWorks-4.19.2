// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SourceCodeNavigation.h"

DEFINE_LOG_CATEGORY_STATIC(LogGatherTextFromMetaDataCommandlet, Log, All);

//////////////////////////////////////////////////////////////////////////
//GatherTextFromMetaDataCommandlet

UGatherTextFromMetaDataCommandlet::UGatherTextFromMetaDataCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

int32 UGatherTextFromMetaDataCommandlet::Main( const FString& Params )
{
	// Parse command line - we're interested in the param vals
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParamVals);

	//Set config file
	const FString* ParamVal = ParamVals.Find(FString(TEXT("Config")));
	FString GatherTextConfigPath;
	
	if ( ParamVal )
	{
		GatherTextConfigPath = *ParamVal;
	}
	else
	{
		UE_LOG(LogGatherTextFromMetaDataCommandlet, Error, TEXT("No config specified."));
		return -1;
	}

	//Set config section
	ParamVal = ParamVals.Find(FString(TEXT("Section")));
	FString SectionName;

	if ( ParamVal )
	{
		SectionName = *ParamVal;
	}
	else
	{
		UE_LOG(LogGatherTextFromMetaDataCommandlet, Error, TEXT("No config section specified."));
		return -1;
	}

	//Include paths
	TArray<FString> IncludePaths;
	GetConfigArray(*SectionName, TEXT("IncludePaths"), IncludePaths, GatherTextConfigPath);

	if (IncludePaths.Num() == 0)
	{
		UE_LOG(LogGatherTextFromMetaDataCommandlet, Error, TEXT("No include paths in section %s"), *SectionName);
		return -1;
	}

	//Exclude paths
	TArray<FString> ExcludePaths;
	GetConfigArray(*SectionName, TEXT("ExcludePaths"), ExcludePaths, GatherTextConfigPath);

	//Required module names
	TArray<FString> RequiredModuleNames;
	GetConfigArray(*SectionName, TEXT("RequiredModuleNames"), ExcludePaths, GatherTextConfigPath);

	// Pre-load all required modules so that UFields from those modules can be guaranteed a chance to have their metadata gathered.
	for(const FString& RequiredModuleName : RequiredModuleNames)
	{
		FModuleManager::Get().LoadModule(*RequiredModuleName);
	}

	// Execute gather.
	GatherTextFromUObjects(IncludePaths, ExcludePaths);

	// Add any manifest dependencies if they were provided
	TArray<FString> ManifestDependenciesList;
	GetConfigArray(*SectionName, TEXT("ManifestDependencies"), ManifestDependenciesList, GatherTextConfigPath);
	
	if( !ManifestInfo->AddManifestDependencies( ManifestDependenciesList ) )
	{
		UE_LOG(LogGatherTextFromMetaDataCommandlet, Error, TEXT("The GatherTextFromMetaData commandlet couldn't find all the specified manifest dependencies."));
		return -1;
	}

	return 0;
}

void UGatherTextFromMetaDataCommandlet::GatherTextFromUObjects(const TArray<FString>& IncludePaths, const TArray<FString>& ExcludePaths)
{
	for(TObjectIterator<UField> It; It; ++It)
	{
		FString SourceFilePath;
		FSourceCodeNavigation::FindClassHeaderPath(*It, SourceFilePath);

		// Returns true if in an include path. False otherwise.
		auto IncludePathLogic = [&]() -> bool
		{
			for(int32 i = 0; i < IncludePaths.Num(); ++i)
			{
				if(SourceFilePath.MatchesWildcard(FPaths::RootDir() + IncludePaths[i] + TEXT("*")))
				{
					return true;
				}
			}
			return false;
		};
		if(!IncludePathLogic())
		{
			continue;
		}

		// Returns true if in an exclude path. False otherwise.
		auto ExcludePathLogic = [&]() -> bool
		{
			for(int32 i = 0; i < ExcludePaths.Num(); ++i)
			{
				if(SourceFilePath.MatchesWildcard(ExcludePaths[i]))
				{
					return true;
				}
			}
			return false;
		};
		if(ExcludePathLogic())
		{
			continue;
		}

		GatherTextFromUObject(*It);
	}
}

void UGatherTextFromMetaDataCommandlet::GatherTextFromUObject(UField* const Field)
{
	const int32 MetaDataCount = 3;
	const FString MetaDataKeys[MetaDataCount] =
	{	TEXT("ToolTip"),			TEXT("DisplayName"),			TEXT("Category")			};
	const FString AssociatedNamespaces[MetaDataCount] =
	{	TEXT("UObjectToolTips"),	TEXT("UObjectDisplayNames"),	TEXT("UObjectCategories")	};

	// Gather for object.
	{
		for(int32 i = 0; i < MetaDataCount; ++i)
		{
			const FString& MetaDataValue = Field->GetMetaData(*MetaDataKeys[i]);
			if(!(MetaDataValue.IsEmpty()))
			{
				const FString Namespace = AssociatedNamespaces[i];
				FLocItem LocItem(MetaDataValue);
				FContext Context;
				Context.Key =	MetaDataKeys[i] == TEXT("Category")
							?	MetaDataValue
							:	Field->GetFullGroupName(true) + TEXT(".") + Field->GetName();
				Context.SourceLocation = TEXT("Run-time MetaData");
				ManifestInfo->AddEntry(TEXT("EntryDescription"), Namespace, LocItem, Context);
			}
		}
	}

	// For enums, also gather for enum values.
	{
		UEnum* Enum = Cast<UEnum>(Field);
		if(Enum)
		{
			const int32 ValueCount = Enum->NumEnums();
			for(int32 i = 0; i < ValueCount; ++i)
			{
				for(int32 j = 0; j < MetaDataCount; ++j)
				{
					const FString& MetaDataValue = Enum->GetMetaData(*MetaDataKeys[j], i);
					if(!(MetaDataValue.IsEmpty()))
					{
						const FString Namespace = AssociatedNamespaces[j];
						FLocItem LocItem(MetaDataValue);
						FContext Context;
						Context.Key =	MetaDataKeys[j] == TEXT("Category")
									?	MetaDataValue
									:	Enum->GetFullGroupName(true) + TEXT(".") + Enum->GetName() + TEXT(".") + Enum->GetEnumName(i);
						Context.SourceLocation = TEXT("Run-time MetaData");
						ManifestInfo->AddEntry(TEXT("EntryDescription"), Namespace, LocItem, Context);
					}
				}
			}
		}
	}
}