// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "CookingStatsPCH.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY(LogCookingStats);

FCookingStats::FCookingStats()
{
	RunGuid = FName(*FGuid::NewGuid().ToString());
}

FCookingStats::~FCookingStats()
{
}

void FCookingStats::AddRunTag(const FName& Tag, const FString& Value)
{
	AddTagValue(RunGuid, Tag, Value);
}

void FCookingStats::AddTag(const FName& Key, const FName& Tag)
{
	AddTagValue(Key, Tag, FString());
}

void FCookingStats::AddTagValue(const FName& Key, const FName& Tag, const FString& TagValue)
{
	FScopeLock ScopeLock(&SyncObject);
	auto Value = KeyTags.Find(Key);

	if (Value == nullptr)
	{
		Value = &KeyTags.Add(Key);
	}

	FTag FullTag;
	FullTag.Name = Tag;
	FullTag.Value = TagValue;
	
	
	Value->Add(FullTag);

}

bool FCookingStats::SaveStatsAsCSV(const FString& Filename)
{
	FString Output;
	FScopeLock ScopeLock(&SyncObject);

	for (const auto& Stat : KeyTags)
	{
		Output += Stat.Key.ToString();

		for (const auto& Tag : Stat.Value)
		{
			Output += TEXT(", ");
			Output += Tag.Name.ToString();
			if (Tag.Value.IsEmpty() == false)
			{
				Output += TEXT("=");
				Output += Tag.Value;
			}
			
		}

		for (const auto& Tag : GlobalTags)
		{
			Output += TEXT(", ");
			Output += Tag.Name.ToString();
			if (Tag.Value.IsEmpty() == false)
			{
				Output += TEXT("=");
				Output += Tag.Value;
			}
		}

		Output += FString::Printf(TEXT(", RunGuid=%s\r\n"), *RunGuid.ToString());
	}

	FFileHelper::SaveStringToFile(Output, *Filename);
	return true;
}

