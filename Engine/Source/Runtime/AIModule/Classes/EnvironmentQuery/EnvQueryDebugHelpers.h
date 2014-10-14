// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTypes.h"
#include "DebugRenderSceneProxy.h"
#include "VisualLoggerExtension.h"
#include "EnvQueryDebugHelpers.generated.h"


#if  USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG
namespace EQSDebug
{
	struct FItemData
	{
		FString Desc;
		int32 ItemIdx;
		float TotalScore;

		TArray<float> TestValues;
		TArray<float> TestScores;
	};

	struct FTestData
	{
		FString ShortName;
		FString Detailed;
	};

	// struct filled while collecting data (to store additional debug data needed to display per rendered item)
	struct FDebugHelper
	{
		FDebugHelper() : Location(FVector::ZeroVector), Radius(0), FailedTestIndex(INDEX_NONE){}
		FDebugHelper(FVector Loc, float R) : Location(Loc), Radius(R), FailedTestIndex(INDEX_NONE) {}
		FDebugHelper(FVector Loc, float R, const FString& Desc) : Location(Loc), Radius(R), FailedTestIndex(INDEX_NONE), AdditionalInformation(Desc) {}

		FVector Location;
		float Radius;
		int32 FailedTestIndex;
		float FailedScore;
		FString AdditionalInformation;
	};

	struct FQueryData
	{
		TArray<FItemData> Items;
		TArray<FTestData> Tests;
		TArray<FDebugRenderSceneProxy::FSphere> SolidSpheres;
		TArray<FDebugRenderSceneProxy::FText3d> Texts;
		TArray<FDebugHelper> RenderDebugHelpers;
		TArray<FString>	Options;
		int32 UsedOption;
		int32 NumValidItems;
		int32 Id;
		FString Name;
		float Timestamp;

		void Reset()
		{
			UsedOption = 0;
			Options.Reset();
			NumValidItems = 0;
			Id = INDEX_NONE;
			Name.Empty();
			Items.Reset();
			Tests.Reset();
			SolidSpheres.Reset();
			Texts.Reset();
			Timestamp = 0;
			RenderDebugHelpers.Reset();
		}
	};
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FDebugRenderSceneProxy::FSphere& Data)
{
	Ar << Data.Radius;
	Ar << Data.Location;
	Ar << Data.Color;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FDebugRenderSceneProxy::FText3d& Data)
{
	Ar << Data.Text;
	Ar << Data.Location;
	Ar << Data.Color;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, EQSDebug::FItemData& Data)
{
	Ar << Data.Desc;
	Ar << Data.ItemIdx;
	Ar << Data.TotalScore;
	Ar << Data.TestValues;
	Ar << Data.TestScores;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, EQSDebug::FTestData& Data)
{
	Ar << Data.ShortName;
	Ar << Data.Detailed;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, EQSDebug::FDebugHelper& Data)
{
	Ar << Data.Location;
	Ar << Data.Radius;
	Ar << Data.AdditionalInformation;
	Ar << Data.FailedTestIndex;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, EQSDebug::FQueryData& Data)
{
	Ar << Data.Items;
	Ar << Data.Tests;
	Ar << Data.SolidSpheres;
	Ar << Data.Texts;
	Ar << Data.NumValidItems;
	Ar << Data.Id;
	Ar << Data.Name;
	Ar << Data.Timestamp;
	Ar << Data.RenderDebugHelpers;
	Ar << Data.Options;
	Ar << Data.UsedOption;
	return Ar;
}

#endif //USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG

#if ENABLE_VISUAL_LOG && USE_EQS_DEBUGGER

#define UE_VLOG_EQS(Query, CategoryName, Verbosity) \
{ \
	UEnvQueryDebugHelpers::LogQuery(Query, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, \
		FString::Printf(TEXT("Executed EQS: \n - Name: '%s' (id=%d, option=%d),\n - All Items: %d,\n - ValidItems: %d"), *Query->QueryName, Query->QueryID, Query->OptionIndex, Query->ItemDetails.Num(), Query->NumValidItems)); \
}

#else
#define UE_VLOG_EQS(Query, CategoryName, Verbosity)

#endif //ENABLE_VISUAL_LOG && USE_EQS_DEBUGGER

UCLASS(Abstract, CustomConstructor)
class AIMODULE_API UEnvQueryDebugHelpers : public UObject
{
	GENERATED_UCLASS_BODY()

	UEnvQueryDebugHelpers(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
#if USE_EQS_DEBUGGER
	static void QueryToDebugData(struct FEnvQueryInstance* Query, EQSDebug::FQueryData& EQSLocalData);
	static void QueryToBlobArray(struct FEnvQueryInstance* Query, TArray<uint8>& BlobArray, bool bUseCompression = false);
	static void BlobArrayToDebugData(const TArray<uint8>& BlobArray, EQSDebug::FQueryData& EQSLocalData, bool bUseCompression = false);
	static void LogQuery(struct FEnvQueryInstance* Query, const FName& CategoryName, ELogVerbosity::Type, const FString& AdditionalLogInfo);
#endif
};

#if USE_EQS_DEBUGGER
FORCEINLINE void UEnvQueryDebugHelpers::LogQuery(struct FEnvQueryInstance* Query, const FName& CategoryName, ELogVerbosity::Type Type, const FString& AdditionalLogInfo)
{
#if ENABLE_VISUAL_LOG
	if (GEngine && GEngine->bDisableAILogging || Query == NULL || FVisualLogger::Get().IsRecording() == false || (FVisualLogger::Get().IsBlockedForAllCategories() && FVisualLogger::Get().GetWhiteList().Find(CategoryName) == INDEX_NONE))
	{
		return;
	}

	const UObject* LogOwner = FVisualLogger::Get().FindRedirection(Query->Owner.Get());
	if (LogOwner == NULL || LogOwner->HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(Query->Owner.Get(), false);
	if (!World)
	{
		return;
	}

	TArray<uint8> BlobArray;
	UEnvQueryDebugHelpers::QueryToBlobArray(Query, BlobArray);

	FVisualLogEntry* EntryToWrite = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	const int32 UniqueId = FVisualLogger::Get().GetUniqueId(World->TimeSeconds);
	if (EntryToWrite)
	{
		FVisualLogEntry::FLogLine Line(CategoryName, Type, AdditionalLogInfo, Query->QueryID);
		Line.TagName = *EVisLogTags::TAG_EQS;
		Line.UniqueId = UniqueId;
		EntryToWrite->LogLines.Add(Line);

		EntryToWrite->AddDataBlock(EVisLogTags::TAG_EQS, BlobArray, CategoryName).UniqueId = UniqueId;
	}
#endif
}
#endif

