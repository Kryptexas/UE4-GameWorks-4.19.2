// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Item.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

//----------------------------------------------------------------------//
// FEQSQueryDebugData
//----------------------------------------------------------------------//

void FEQSQueryDebugData::Store(const FEnvQueryInstance* QueryInstance)
{
	DebugItemDetails = QueryInstance->ItemDetails;
	DebugItems = QueryInstance->Items;
	RawData = QueryInstance->RawData;
}

//----------------------------------------------------------------------//
// FEnvQueryInstance
//----------------------------------------------------------------------//
#if USE_EQS_DEBUGGER
bool FEnvQueryInstance::bDebuggingInfoEnabled = true;
#endif // USE_EQS_DEBUGGER

bool FEnvQueryInstance::PrepareContext(UClass* ContextClass, FEnvQueryContextData& ContextData)
{
	if (ContextClass == NULL)
	{
		return false;
	}

	if (ContextClass != UEnvQueryContext_Item::StaticClass())
	{
		FEnvQueryContextData* CachedData = ContextCache.Find(ContextClass);
		if (CachedData == NULL)
		{
			UEnvQueryContext* ContextOb = ContextClass->GetDefaultObject<UEnvQueryContext>();
			ContextOb->ProvideContext(*this, ContextData);

			DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, GetContextAllocatedSize());

			ContextCache.Add(ContextClass, ContextData);

			INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, GetContextAllocatedSize());
		}
		else
		{
			ContextData = *CachedData;
		}
	}

	if (ContextData.NumValues == 0)
	{
		UE_LOG(LogEQS, Log, TEXT("Query [%s] is missing values for context [%s], skipping test %d:%d [%s]"),
			*QueryName, *UEnvQueryTypes::GetShortTypeName(ContextClass).ToString(),
			OptionIndex, CurrentTest,
			CurrentTest >=0 ? *UEnvQueryTypes::GetShortTypeName(Options[OptionIndex].GetTestObject(CurrentTest)).ToString() : TEXT("Generator")
			);

		return false;
	}

	return true;
}

bool FEnvQueryInstance::PrepareContext(UClass* Context, TArray<FEnvQuerySpatialData>& Data)
{
	if (Context == NULL)
	{
		return false;
	}

	FEnvQueryContextData ContextData;
	const bool bSuccess = PrepareContext(Context, ContextData);

	if (bSuccess && ContextData.ValueType && ContextData.ValueType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
	{
		UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ContextData.ValueType->GetDefaultObject();
		const uint16 DefTypeValueSize = DefTypeOb->GetValueSize();
		uint8* RawData = ContextData.RawData.GetTypedData();

		Data.Init(ContextData.NumValues);
		for (int32 ValueIndex = 0; ValueIndex < ContextData.NumValues; ValueIndex++)
		{
			Data[ValueIndex].Location = DefTypeOb->GetLocation(RawData);
			Data[ValueIndex].Rotation = DefTypeOb->GetRotation(RawData);
			RawData += DefTypeValueSize;
		}
	}

	return bSuccess;
}

bool FEnvQueryInstance::PrepareContext(UClass* Context, TArray<FVector>& Data)
{
	if (Context == NULL)
	{
		return false;
	}

	FEnvQueryContextData ContextData;
	const bool bSuccess = PrepareContext(Context, ContextData);

	if (bSuccess && ContextData.ValueType && ContextData.ValueType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
	{
		UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ContextData.ValueType->GetDefaultObject();
		const uint16 DefTypeValueSize = DefTypeOb->GetValueSize();
		uint8* RawData = (uint8*)ContextData.RawData.GetTypedData();

		Data.Init(ContextData.NumValues);
		for (int32 ValueIndex = 0; ValueIndex < ContextData.NumValues; ValueIndex++)
		{
			Data[ValueIndex] = DefTypeOb->GetLocation(RawData);
			RawData += DefTypeValueSize;
		}
	}

	return bSuccess;
}

bool FEnvQueryInstance::PrepareContext(UClass* Context, TArray<FRotator>& Data)
{
	if (Context == NULL)
	{
		return false;
	}

	FEnvQueryContextData ContextData;
	const bool bSuccess = PrepareContext(Context, ContextData);

	if (bSuccess && ContextData.ValueType && ContextData.ValueType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
	{
		UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ContextData.ValueType->GetDefaultObject();
		const uint16 DefTypeValueSize = DefTypeOb->GetValueSize();
		uint8* RawData = ContextData.RawData.GetTypedData();

		Data.Init(ContextData.NumValues);
		for (int32 ValueIndex = 0; ValueIndex < ContextData.NumValues; ValueIndex++)
		{
			Data[ValueIndex] = DefTypeOb->GetRotation(RawData);
			RawData += DefTypeValueSize;
		}
	}

	return bSuccess;
}

bool FEnvQueryInstance::PrepareContext(UClass* Context, TArray<AActor*>& Data)
{
	if (Context == NULL)
	{
		return false;
	}

	FEnvQueryContextData ContextData;
	const bool bSuccess = PrepareContext(Context, ContextData);

	if (bSuccess && ContextData.ValueType && ContextData.ValueType->IsChildOf(UEnvQueryItemType_ActorBase::StaticClass()))
	{
		UEnvQueryItemType_ActorBase* DefTypeOb = (UEnvQueryItemType_ActorBase*)ContextData.ValueType->GetDefaultObject();
		const uint16 DefTypeValueSize = DefTypeOb->GetValueSize();
		uint8* RawData = ContextData.RawData.GetTypedData();

		Data.Init(ContextData.NumValues);
		for (int32 ValueIndex = 0; ValueIndex < ContextData.NumValues; ValueIndex++)
		{
			Data[ValueIndex] = DefTypeOb->GetActor(RawData);
			RawData += DefTypeValueSize;
		}
	}

	return bSuccess;
}

void FEnvQueryInstance::ExecuteOneStep(double InTimeLimit)
{
	if (!Owner.IsValid())
	{
		Status = EEnvQueryStatus::OwnerLost;
		return;
	}

	FEnvQueryOptionInstance& OptionItem = Options[OptionIndex];
	CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_AI_EQS_GeneratorTime, CurrentTest < 0);
	CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_AI_EQS_TestTime, CurrentTest >= 0);

	const bool bDoingLastTest = (CurrentTest == OptionItem.TestsToPerform.Num() - 1);
	bool bStepDone = true;
	TimeLimit = InTimeLimit;

	if (CurrentTest < 0)
	{
		DEC_DWORD_STAT_BY(STAT_AI_EQS_NumItems, Items.Num());

//		SCOPE_LOG_TIME(TEXT("Generator"), nullptr);

		RawData.Reset();
		Items.Reset();
		ItemType = OptionItem.ItemType;
		bPassOnSingleResult = false;
		ValueSize = ((UEnvQueryItemType*)ItemType->GetDefaultObject())->GetValueSize();
		
		OptionItem.Generator->GenerateItems(*this);
		FinalizeGeneration();
	}
	else
	{
//		SCOPE_LOG_TIME(*UEnvQueryTypes::GetShortTypeName(Options[OptionIndex].GetTestObject(CurrentTest)).ToString(), nullptr);

		UEnvQueryTest* TestObject = OptionItem.GetTestObject(CurrentTest);

		// item generator uses this flag to alter the scoring behavior
		bPassOnSingleResult = (bDoingLastTest && Mode == EEnvQueryRunMode::SingleResult && TestObject->CanRunAsFinalCondition());

		const int32 ItemsAlreadyProcessed = CurrentTestStartingItem;
		TestObject->RunTest(*this);
		bStepDone = CurrentTestStartingItem >= Items.Num() || bFoundSingleResult
			// or no items processed ==> this means error
			|| (ItemsAlreadyProcessed == CurrentTestStartingItem);

		if (bStepDone)
		{
			FinalizeTest();
		}
	}
	
	if (bStepDone)
	{
#if USE_EQS_DEBUGGER
		if (bStoreDebugInfo)
		{
			DebugData.Store(this);
		}
#endif // USE_EQS_DEBUGGER

		CurrentTest++;
		CurrentTestStartingItem = 0;
	}

	// sort results or switch to next option when all tests are performed
	if (Status == EEnvQueryStatus::Processing &&
		(OptionItem.TestsToPerform.Num() == CurrentTest || NumValidItems <= 0))
	{
		if (NumValidItems > 0)
		{
			// found items, sort and finish
			FinalizeQuery();
		}
		else
		{
			// no items here, go to next option or finish			
			if (OptionIndex + 1 >= Options.Num())
			{
				// out of options, finish processing without errors
				FinalizeQuery();
			}
			else
			{
				// not doing it always for debugging purposes
				OptionIndex++;
				CurrentTest = -1;
			}
		}
	}
}

#if !NO_LOGGING
void FEnvQueryInstance::Log(const FString Msg) const
{
	UE_LOG(LogEQS, Warning, TEXT("%s"), *Msg);
}
#endif // !NO_LOGGING

void FEnvQueryInstance::ReserveItemData(int32 NumAdditionalItems)
{
	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, RawData.GetAllocatedSize());

	RawData.Reserve((RawData.Num() + NumAdditionalItems) * ValueSize);

	INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, RawData.GetAllocatedSize());
}


FEnvQueryInstance::ItemIterator::ItemIterator(const UEnvQueryTest* QueryTest, FEnvQueryInstance& QueryInstance, int32 StartingItemIndex)
	: Instance(&QueryInstance)
	, CurrentItem(StartingItemIndex != INDEX_NONE ? StartingItemIndex : QueryInstance.CurrentTestStartingItem)
{
	Deadline = QueryInstance.TimeLimit > 0.0 ? (FPlatformTime::Seconds() + QueryInstance.TimeLimit) : -1.0;
	bDiscardFailed = QueryTest && QueryTest->bDiscardFailedItems;
	// it's possible item 'CurrentItem' has been already discarded. Find a valid starting index
	--CurrentItem;
	FindNextValidIndex();
	InitItemScore();
}

void FEnvQueryInstance::ItemIterator::HandleFailedTestResult()
{
	ItemScore = -1.f;
	Instance->Items[CurrentItem].Discard();
#if USE_EQS_DEBUGGER
	Instance->ItemDetails[CurrentItem].FailedTestIndex = Instance->CurrentTest;
#endif
	Instance->NumValidItems--;
}

void FEnvQueryInstance::ItemIterator::StoreTestResult()
{
	if (Instance->IsInSingleItemFinalSearch())
	{
		// handle SingleResult mode
		if (bPassed && !bSkipped)
		{
			Instance->PickSingleItem(CurrentItem);
			Instance->bFoundSingleResult = true;
		}
		else if (!bPassed || NumPartialScores == 0)
		{
			HandleFailedTestResult();
		}
	}
	else
	{
		if (bSkipped)
		{
			ItemScore = UEnvQueryTypes::SkippedItemValue;
		}
		else if (!bPassed || NumPartialScores == 0)
		{
			HandleFailedTestResult();
		}
		else
		{
			ItemScore /= NumPartialScores;
		}

		Instance->ItemDetails[CurrentItem].TestResults[Instance->CurrentTest] = ItemScore;
	}
}

void FEnvQueryInstance::NormalizeScores()
{
	// @note this function assumes results have been already sorted and all first NumValidItems
	// items in Items are valid (and the rest is not).
	check(NumValidItems <= Items.Num())

	float MinScore = 0.f;
	float MaxScore = -BIG_NUMBER;

	FEnvQueryItem* ItemInfo = Items.GetTypedData();
	for (int32 ItemIndex = 0; ItemIndex < NumValidItems; ItemIndex++, ItemInfo++)
	{
		ensure(ItemInfo->IsValid());

		MinScore = FMath::Min(MinScore, ItemInfo->Score);
		MaxScore = FMath::Max(MaxScore, ItemInfo->Score);
	}

	ItemInfo = Items.GetTypedData();
	if (MinScore == MaxScore)
	{
		for (int32 ItemIndex = 0; ItemIndex < NumValidItems; ItemIndex++, ItemInfo++)
		{
			ItemInfo->Score = 1.0f;
		}
	}
	else
	{
		const float ScoreRange = MaxScore - MinScore;
		for (int32 ItemIndex = 0; ItemIndex < NumValidItems; ItemIndex++, ItemInfo++)
		{
			ItemInfo->Score = (ItemInfo->Score - MinScore) / ScoreRange;
		}
	}
}

void FEnvQueryInstance::SortScores()
{
	const int32 NumItems = Items.Num();
	if (Options[OptionIndex].bShuffleItems)
	{
		for (int32 ItemIndex = 0; ItemIndex < NumItems; ItemIndex++)
		{
			const int32 Idx1 = FMath::RandHelper(NumItems);
			const int32 Idx2 = FMath::RandHelper(NumItems);
			Items.Swap(Idx1, Idx2);

#if USE_EQS_DEBUGGER
			ItemDetails.Swap(Idx1, Idx2);
#endif
		}
	}

#if USE_EQS_DEBUGGER
	struct FSortHelperForDebugData
	{
		FEnvQueryItem	Item;
		struct FEnvQueryItemDetails ItemDetails;

		FSortHelperForDebugData(const FEnvQueryItem&	InItem, struct FEnvQueryItemDetails& InDebugDetails) : Item(InItem), ItemDetails(InDebugDetails) {}
		bool operator<(const FSortHelperForDebugData& Other) const
		{
			return Item < Other.Item;
		}
	};
	TArray<FSortHelperForDebugData> AllData;
	AllData.Reserve(NumItems);
	for (int32 Index = 0; Index < NumItems; ++Index)
	{
		AllData.Add(FSortHelperForDebugData(Items[Index], ItemDetails[Index]));
	}
	AllData.Sort(TGreater<FSortHelperForDebugData>());

	for (int32 Index = 0; Index < NumItems; ++Index)
	{
		Items[Index] = AllData[Index].Item;
		ItemDetails[Index] = AllData[Index].ItemDetails;
	}
#else
	Items.Sort(TGreater<FEnvQueryItem>());
#endif
}

void FEnvQueryInstance::StripRedundantData()
{
#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		DebugData.Reset();
	}
#endif // USE_EQS_DEBUGGER
	Items.SetNum(NumValidItems);
}

void FEnvQueryInstance::PickBestItem()
{
	// find first valid item with score worse than best one
	int32 NumBestItems = NumValidItems;
	for (int32 ItemIndex = 1; ItemIndex < NumValidItems; ItemIndex++)
	{
		if (Items[ItemIndex].Score < Items[0].Score)
		{
			NumBestItems = ItemIndex;
			break;
		}
	}

	// pick only one, discard others
	PickSingleItem(FMath::RandHelper(NumBestItems));
}

void FEnvQueryInstance::PickSingleItem(int32 ItemIndex)
{
	FEnvQueryItem BestItem;
	BestItem.Score = 1.0f;
	BestItem.DataOffset = Items[ItemIndex].DataOffset;

	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());

#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		Items.Swap(0, ItemIndex);
		Items[0].Score = 1.0f;
		ItemDetails.Swap(0, ItemIndex);

		DebugData.bSingleItemResult = true;

		// do not limit valid items amount for debugger purposes!
		// bFoundSingleResult can be used to determine if more than one item is valid

		// normalize all scores for debugging
		//NormalizeScores();
	}
	else
	{
#endif
		Items.Empty(1);
		Items.Add(BestItem);
		NumValidItems = 1;
#if USE_EQS_DEBUGGER
	}
#endif

	INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());

}

void FEnvQueryInstance::FinalizeQuery()
{
	if (NumValidItems > 0)
	{
		if (Mode == EEnvQueryRunMode::SingleResult)
		{
			// if last test was not pure condition: sort and pick one of best items
			if (bFoundSingleResult == false && bPassOnSingleResult == false)
			{
				SortScores();
				PickBestItem();
			}
		}
		else
		{
			SortScores();

			DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());

			// remove failed ones from Items
			Items.SetNum(NumValidItems);

			INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());

			// normalizing after scoring and reducing number of elements to not 
			// do anything for discarded items
			NormalizeScores();
		}
	}
	else
	{
		Items.Reset();
		ItemDetails.Reset();
		RawData.Reset();
	}

	Status = EEnvQueryStatus::Success;
}

void FEnvQueryInstance::FinalizeGeneration()
{
	FEnvQueryOptionInstance& OptionInstance = Options[OptionIndex];
	const int32 NumTests = OptionInstance.TestsToPerform.Num();

	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, ItemDetails.GetAllocatedSize());
	INC_DWORD_STAT_BY(STAT_AI_EQS_NumItems, Items.Num());

	NumValidItems = Items.Num();
	ItemDetails.Reset();
	bFoundSingleResult = false;

	if (NumValidItems > 0)
	{
		ItemDetails.Reserve(NumValidItems);
		for (int32 ItemIndex = 0; ItemIndex < NumValidItems; ItemIndex++)
		{
			ItemDetails.Add(FEnvQueryItemDetails(NumTests, ItemIndex));
		}
	}

	INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, ItemDetails.GetAllocatedSize());

	ItemTypeVectorCDO = (ItemType && ItemType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass())) ?
		(UEnvQueryItemType_VectorBase*)ItemType->GetDefaultObject() :	NULL;

	ItemTypeActorCDO = (ItemType && ItemType->IsChildOf(UEnvQueryItemType_ActorBase::StaticClass())) ?
		(UEnvQueryItemType_ActorBase*)ItemType->GetDefaultObject() : NULL;
}

void FEnvQueryInstance::FinalizeTest()
{
	FEnvQueryOptionInstance& OptionInstance = Options[OptionIndex];
	const int32 NumTests = OptionInstance.TestsToPerform.Num();

	UEnvQueryTest* DefTestOb = OptionInstance.GetTestObject(CurrentTest);
#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		DebugData.PerformedTestNames.Add(UEnvQueryTypes::GetShortTypeName(DefTestOb).ToString());
	}
#endif

	// if it's not the last and final test
	if (IsInSingleItemFinalSearch() == false)
	{
		// do regular normalization
		DefTestOb->NormalizeItemScores(*this);
	}
	else
	{
#if USE_EQS_DEBUGGER
		if (bStoreDebugInfo == false)
#endif // USE_EQS_DEBUGGER
			ItemDetails.Reset();	// mind the "if (bStoreDebugInfo == false)" above
	}
}

#if STATS
uint32 FEnvQueryInstance::GetAllocatedSize() const
{
	uint32 MemSize = sizeof(*this) + Items.GetAllocatedSize() + RawData.GetAllocatedSize();
	MemSize += GetContextAllocatedSize();
	MemSize += NamedParams.GetAllocatedSize();
	MemSize += ItemDetails.GetAllocatedSize();
	MemSize += Options.GetAllocatedSize();

	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); OptionIndex++)
	{
		MemSize += Options[OptionIndex].GetAllocatedSize();
	}

	return MemSize;
}

uint32 FEnvQueryInstance::GetContextAllocatedSize() const
{
	uint32 MemSize = ContextCache.GetAllocatedSize();
	for (TMap<UClass*, FEnvQueryContextData>::TConstIterator It(ContextCache); It; ++It)
	{
		MemSize += It.Value().GetAllocatedSize();
	}

	return MemSize;
}
#endif // STATS

FBox FEnvQueryInstance::GetBoundingBox() const
{
	const TArray<FEnvQueryItem>& Items = 
#if USE_EQS_DEBUGGER
	DebugData.DebugItems.Num() > 0 ? DebugData.DebugItems : 
#endif // USE_EQS_DEBUGGER
		this->Items;

	const TArray<uint8>& RawData = 
#if USE_EQS_DEBUGGER
		DebugData.RawData.Num() > 0 ? DebugData.RawData : 
#endif // USE_EQS_DEBUGGER
		this->RawData;

	FBox BBox(0);

	if (ItemType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
	{
		UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ItemType->GetDefaultObject();

		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{		
			BBox += DefTypeOb->GetLocation(RawData.GetTypedData() + Items[Index].DataOffset);
		}
	}

	return BBox;
}