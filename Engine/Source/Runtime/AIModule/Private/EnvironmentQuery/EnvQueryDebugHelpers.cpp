// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "DebugRenderSceneProxy.h"
#include "EnvironmentQuery/EQSRenderingComponent.h"
#include "EnvironmentQuery/EnvQueryDebugHelpers.h"
#include "EnvironmentQuery/EQSQueryResultSourceInterface.h"

#if USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG
void UEnvQueryDebugHelpers::QueryToBlobArray(struct FEnvQueryInstance* Query, TArray<uint8>& BlobArray, bool bUseCompression)
{
	EQSDebug::FQueryData EQSLocalData;
	UEnvQueryDebugHelpers::QueryToDebugData(Query, EQSLocalData);

	if (!bUseCompression)
	{
		FMemoryWriter ArWriter(BlobArray);
		ArWriter << EQSLocalData;
	}
	else
	{
		TArray<uint8> UncompressedBuffer;
		FMemoryWriter ArWriter(UncompressedBuffer);
		ArWriter << EQSLocalData;

		const int32 UncompressedSize = UncompressedBuffer.Num();
		const int32 HeaderSize = sizeof(int32);
		BlobArray.Init(0, HeaderSize + FMath::TruncToInt(1.1f * UncompressedSize));

		int32 CompressedSize = BlobArray.Num() - HeaderSize;
		uint8* DestBuffer = BlobArray.GetTypedData();
		FMemory::Memcpy(DestBuffer, &UncompressedSize, HeaderSize);
		DestBuffer += HeaderSize;

		FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory), (void*)DestBuffer, CompressedSize, (void*)UncompressedBuffer.GetData(), UncompressedSize);

		BlobArray.SetNum(CompressedSize + HeaderSize, false);
	}
}

void UEnvQueryDebugHelpers::QueryToDebugData(struct FEnvQueryInstance* Query, EQSDebug::FQueryData& EQSLocalData)
{
	// step 1: data for rendering component
	EQSLocalData.SolidSpheres.Reset();
	EQSLocalData.Texts.Reset();

	FEQSSceneProxy::CollectEQSData(Query, Query, EQSLocalData.SolidSpheres, EQSLocalData.Texts, true);

	// step 2: detailed scoring data for HUD
	const int32 MaxDetailedItems = 10;
	const int32 FirstItemIndex = 0;

	const int32 NumTests = Query->ItemDetails.IsValidIndex(0) ? Query->ItemDetails[0].TestResults.Num() : 0;
	const int32 NumItems = FMath::Min(MaxDetailedItems, Query->NumValidItems);

	EQSLocalData.Name = Query->QueryName;
	EQSLocalData.Id = Query->QueryID;
	EQSLocalData.Items.Reset(NumItems);
	EQSLocalData.Tests.Reset(NumTests);
	EQSLocalData.NumValidItems = Query->NumValidItems;

	UEnvQueryItemType* ItemCDO = Query->ItemType.GetDefaultObject();
	for (int32 ItemIdx = 0; ItemIdx < NumItems; ItemIdx++)
	{
		EQSDebug::FItemData ItemInfo;
		ItemInfo.ItemIdx = ItemIdx + FirstItemIndex;
		ItemInfo.TotalScore = Query->Items[ItemInfo.ItemIdx].Score;

		const uint8* ItemData = Query->RawData.GetTypedData() + Query->Items[ItemInfo.ItemIdx].DataOffset;
		ItemInfo.Desc = FString::Printf(TEXT("[%d] %s"), ItemInfo.ItemIdx, *ItemCDO->GetDescription(ItemData));

		ItemInfo.TestValues.Reserve(NumTests);
		ItemInfo.TestScores.Reserve(NumTests);
		for (int32 TestIdx = 0; TestIdx < NumTests; TestIdx++)
		{
			const float ScoreN = Query->ItemDetails[ItemInfo.ItemIdx].TestResults[TestIdx];
			const float ScoreW = Query->ItemDetails[ItemInfo.ItemIdx].TestWeightedScores[TestIdx];

			ItemInfo.TestValues.Add(ScoreN);
			ItemInfo.TestScores.Add(ScoreW);
		}

		EQSLocalData.Items.Add(ItemInfo);
	}

	for (int32 TestIdx = 0; TestIdx < NumTests; TestIdx++)
	{
		EQSDebug::FTestData TestInfo;

		UEnvQueryTest* TestOb = Query->Options[Query->OptionIndex].GetTestObject(TestIdx);
		TestInfo.ShortName = TestOb->GetDescriptionTitle();
		TestInfo.Detailed = TestOb->GetDescriptionDetails().ToString().Replace(TEXT("\n"), TEXT(", "));

		EQSLocalData.Tests.Add(TestInfo);
	}
}

void  UEnvQueryDebugHelpers::BlobArrayToDebugData(const TArray<uint8>& BlobArray, EQSDebug::FQueryData& EQSLocalData, bool bUseCompression)
{
	if (!bUseCompression)
	{
		FMemoryReader ArReader(BlobArray);
		ArReader << EQSLocalData;
	}
	else
	{
		TArray<uint8> UncompressedBuffer;
		int32 UncompressedSize = 0;
		const int32 HeaderSize = sizeof(int32);
		uint8* SrcBuffer = (uint8*)BlobArray.GetTypedData();
		FMemory::Memcpy(&UncompressedSize, SrcBuffer, HeaderSize);
		SrcBuffer += HeaderSize;
		const int32 CompressedSize = BlobArray.Num() - HeaderSize;

		UncompressedBuffer.AddZeroed(UncompressedSize);

		FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB), (void*)UncompressedBuffer.GetTypedData(), UncompressedSize, SrcBuffer, CompressedSize);
		FMemoryReader ArReader(UncompressedBuffer);

		ArReader << EQSLocalData;
	}
}

void UEnvQueryDebugHelpers::LogQuery(struct FEnvQueryInstance* Query, const FName& CategoryName, ELogVerbosity::Type Type)
{
	TArray<uint8> BlobArray;
	UEnvQueryDebugHelpers::QueryToBlobArray(Query, BlobArray);
	FVisualLog::Get().GetEntryToWrite(Cast<AActor>(Query->Owner.Get()))->AddDataBlock(EVisLogTags::TAG_EQS, BlobArray, CategoryName);
}

#endif //ENABLE_VISUAL_LOG
