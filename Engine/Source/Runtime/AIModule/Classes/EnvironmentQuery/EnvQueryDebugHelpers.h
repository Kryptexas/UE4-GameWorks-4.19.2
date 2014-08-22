// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
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

	struct FQueryData
	{
		TArray<FItemData> Items;
		TArray<FTestData> Tests;
		TArray<FDebugRenderSceneProxy::FSphere> SolidSpheres;
		TArray<FDebugRenderSceneProxy::FText3d> Texts;
		int32 NumValidItems;
		int32 Id;
		FString Name;
		float Timestamp;

		void Reset()
		{
			NumValidItems = 0;
			Id = INDEX_NONE;
			Name.Empty();
			Items.Reset();
			Tests.Reset();
			SolidSpheres.Reset();
			Texts.Reset();
			Timestamp = 0;
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
	return Ar;
}

#endif //USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG

#if ENABLE_VISUAL_LOG

#define UE_VLOG_EQS(Query, CategoryName, Verbosity) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* OwnerActor = FVisualLog::Get().GetVisualLogRedirection(Query->Owner.Get()); \
		ensure(OwnerActor != NULL); \
		if (OwnerActor) \
		{  \
			UE_VLOG(OwnerActor, CategoryName, Verbosity, TEXT("Executed EQS: \n - Name: '%s' (id=%d, option=%d),\n - All Items: %d,\n - ValidItems: %d"), *Query->QueryName, Query->QueryID, Query->OptionIndex, Query->ItemDetails.Num(), Query->NumValidItems); \
			UEnvQueryDebugHelpers::LogQuery(OwnerActor, Query, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity); \
		} \
	} \
}

#else
#define UE_VLOG_EQS(Query, CategoryName, Verbosity)

#endif //ENABLE_VISUAL_LOG

UCLASS(Abstract, CustomConstructor)
class AIMODULE_API UEnvQueryDebugHelpers : public UObject
{
	GENERATED_UCLASS_BODY()

	UEnvQueryDebugHelpers(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP) {}
#if USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG
	static void QueryToDebugData(struct FEnvQueryInstance* Query, EQSDebug::FQueryData& EQSLocalData);
	static void QueryToBlobArray(struct FEnvQueryInstance* Query, TArray<uint8>& BlobArray, bool bUseCompression = false);
	static void BlobArrayToDebugData(const TArray<uint8>& BlobArray, EQSDebug::FQueryData& EQSLocalData, bool bUseCompression = false);
	static void LogQuery(const class AActor* LogOwnerActor, struct FEnvQueryInstance* Query, const FName& CategoryName, ELogVerbosity::Type);
#endif
};
