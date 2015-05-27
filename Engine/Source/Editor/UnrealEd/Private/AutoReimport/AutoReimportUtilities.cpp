// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutoReimportUtilities.h"
#include "EditorReimportHandler.h"
#include "ARFilter.h"
#include "IAssetRegistry.h"
#include "AssetSourceFilenameCache.h"

bool MatchExtensionString(const TCHAR* Filename, const TCHAR* Extensions)
{
	const int32 StrLength = FCString::Strlen(Filename);

	const TCHAR* Ext = FCString::Strrchr(Filename, '.');
	if (Ext && *(++Ext) != '\0')
	{
		const int32 ExtLength = StrLength - (Ext - Filename);

		const TCHAR* Search = FCString::Strchr(Extensions, ';');
		while(Search)
		{
			if (*(++Search) == '\0')
			{
				break;
			}

			if (FCString::Strnicmp(Search, Ext, ExtLength) == 0 && *(Search + ExtLength) == ';')
			{
				return true;
			}

			Search = FCString::Strchr(Search, ';');
		}
	}

	return false;
}

/** Test MatchExtensionString */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchExtensionStringTest, "System.Editor.Auto Reimport.Extension Matching", EAutomationTestFlags::ATF_Editor)
bool FMatchExtensionStringTest::RunTest(const FString& Parameters)
{
	auto Test = [](const TCHAR* Needle, const TCHAR* Haystack, bool bExpected) -> bool{

		if (MatchExtensionString(Needle, Haystack) != bExpected)
		{
			if (bExpected)
			{
				UE_LOG(LogAutoReimportManager, Error, TEXT("Unable to match '%s' in '%s'"), Needle, Haystack);
			}
			else
			{
				UE_LOG(LogAutoReimportManager, Error, TEXT("Erroneously matched '%s' in '%s'"), Needle, Haystack);
			}
			return false;
		}
		return true;
	};

	bool Result = true;
	Result = Test(TEXT("bla.txt"), TEXT(";;"), false) && Result;
	Result = Test(TEXT("bla.txt"), TEXT(";"), false) && Result;
	Result = Test(TEXT("bla.txt"), TEXT(""), false) && Result;
	Result = Test(TEXT("bla.txt"), TEXT(";txt;"), true) && Result;
	Result = Test(TEXT("bla.text"), TEXT(";txt;"), false) && Result;
	Result = Test(TEXT("bla.txt1"), TEXT(";txt;"), false) && Result;
	Result = Test(TEXT("bla."), TEXT(";bla;"), false) && Result;
	Result = Test(TEXT("bla.png"), TEXT(";png;txt;"), true) && Result;
	Result = Test(TEXT("bla.txt"), TEXT(";png;txt;"), true) && Result;
	Result = Test(TEXT("/folder.bin/bla.txt"), TEXT(";png;txt;"), true) && Result;
	Result = Test(TEXT("/folder.bin/bla"), TEXT(";png;bin;"), false) && Result;

	return true;
}

struct FWildcardRule : IMatchRule
{
	FWildcardString WildcardString;
	bool bInclude;

	virtual ~FWildcardRule(){}
	
	virtual TOptional<bool> IsFileApplicable(const TCHAR* Filename) const override
	{
		if (WildcardString.IsMatch(Filename))
		{
			return bInclude;
		}
		return TOptional<bool>();
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << WildcardString;
		Ar << bInclude;
	}
};

void FMatchRules::AddWildcardRule(const FWildcardString& WildcardString, bool bInclude)
{
	FMatchRule Rule;
	Rule.Type = FMatchRule::Wildcard;
	auto* Impl = new FWildcardRule;
	Impl->WildcardString = WildcardString;
	Impl->bInclude = bInclude;
	Rule.RuleImpl = MakeShareable(Impl);
	Impls.Add(Rule);

	// If there are any include patterns, we default to not matching all files
	if (bInclude)
	{
		bDefaultIncludeState = false;
	}
}

void FMatchRules::SetApplicableExtensions(const FString& InExtensions)
{
	ApplicableExtensions = InExtensions;

	// Ensure that the extension strings are of the form ;ext1;ext2;ext3;
	if (ApplicableExtensions[ApplicableExtensions.Len() - 1] != ';')
	{
		ApplicableExtensions += TEXT(";");
	}
	if (ApplicableExtensions[0] != ';')
	{
		ApplicableExtensions.InsertAt(0, ';');
	}

}

void FMatchRules::FMatchRule::Serialize(FArchive& Ar)
{
	Ar << Type;
	
	if (Ar.IsLoading())
	{
		if (Type == Wildcard)
		{
			RuleImpl = MakeShareable(new FWildcardRule);
		}
	}

	if (RuleImpl.IsValid())
	{
		RuleImpl->Serialize(Ar);
	}
}

bool FMatchRules::IsFileApplicable(const TCHAR* Filename) const
{
	if (!ApplicableExtensions.IsEmpty() && !MatchExtensionString(Filename, *ApplicableExtensions))
	{
		return false;
	}
	
	// If we have no rules, we match everything
	if (Impls.Num() == 0)
	{
		return true;
	}

	bool bApplicable = bDefaultIncludeState;

	// Otherwise we match any of the rules
	for (const auto& Rule : Impls)
	{
		if (Rule.RuleImpl.IsValid())
		{
			auto IsApplicable = Rule.RuleImpl->IsFileApplicable(Filename);
			if (IsApplicable.IsSet())
			{
				if (!IsApplicable.GetValue())
				{
					return false;
				}
				else
				{
					bApplicable = true;
				}
			}
		}
	}

	return bApplicable;
}

namespace Utils
{
	TArray<FAssetData> FindAssetsPertainingToFile(const IAssetRegistry& Registry, const FString& AbsoluteFilename)
	{
		return FAssetSourceFilenameCache::Get().GetAssetsPertainingToFile(Registry, AbsoluteFilename);
	}

	TArray<FString> ExtractSourceFilePaths(UObject* Object)
	{
		TArray<FString> Temp;
		ExtractSourceFilePaths(Object, Temp);
		return Temp;
	}

	void ExtractSourceFilePaths(UObject* Object, TArray<FString>& OutSourceFiles)
	{
		TArray<UObject::FAssetRegistryTag> TagList;
		Object->GetAssetRegistryTags(TagList);

		const FName TagName = UObject::SourceFileTagName();
		for (const auto& Tag : TagList)
		{
			if (Tag.Name == TagName)
			{
				int32 PreviousNum = OutSourceFiles.Num();

				TOptional<FAssetImportInfo> ImportInfo = FAssetImportInfo::FromJson(Tag.Value);
				if (ImportInfo.IsSet())
				{
					for (const auto& File : ImportInfo->GetSourceFileData())
					{
						OutSourceFiles.Emplace(UAssetImportData::ResolveImportFilename(File.RelativeFilename, Object->GetOutermost()));
					}
				}
				break;
			}
		}
	}
}