// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQueryTest.h"
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

struct FItemSortPredicate
{
	TArray<FEnvQueryItem>* PtrItems;
	FItemSortPredicate(TArray<FEnvQueryItem>* InItems) : PtrItems(InItems) {}

	FORCEINLINE bool operator()(const FEnvQueryItemDetails& A, const FEnvQueryItemDetails& B) const
	{
		return (*PtrItems)[B.ItemIndex] < (*PtrItems)[A.ItemIndex];
	}
};
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
			CurrentTest >=0 ? *UEnvQueryTypes::GetShortTypeName(Options[OptionIndex].TestDelegates[CurrentTest].GetUObject()).ToString() : TEXT("Generator")
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

	bool bStepDone = true;
	TimeLimit = InTimeLimit;

	if (CurrentTest < 0)
	{
		DEC_DWORD_STAT_BY(STAT_AI_EQS_NumItems, Items.Num());

		RawData.Reset();
		Items.Reset();
		ItemType = OptionItem.ItemType;
		ValueSize = ((UEnvQueryItemType*)ItemType->GetDefaultObject())->GetValueSize();

		OptionItem.GenerateDelegate.Execute(*this);
		FinalizeGeneration();
	}
	else
	{
		const int32 ItemsAlreadyProcessed = CurrentTestStartingItem;
		OptionItem.TestDelegates[CurrentTest].Execute(*this);
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
		(OptionItem.TestDelegates.Num() == CurrentTest || NumValidItems <= 0))
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


FEnvQueryInstance::ItemIterator::ItemIterator(class UEnvQueryTest* QueryTest, FEnvQueryInstance& QueryInstance)
	: Instance(&QueryInstance), CurrentItem(QueryInstance.CurrentTestStartingItem)
{
	Deadline = QueryInstance.TimeLimit > 0.0 ? (FPlatformTime::Seconds() + QueryInstance.TimeLimit) : -1.0;
	bDiscardFailed = QueryTest && QueryTest->bDiscardFailedItems;
	InitItemScore();
}

void FEnvQueryInstance::ItemIterator::StoreTestResult()
{
	if (Instance->Mode == EEnvQueryRunMode::SingleResult &&	Instance->bPassOnSingleResult)
	{
		// handle SingleResult mode
		if (bPassed && !bSkipped)
		{
			Instance->PickSingleItem(CurrentItem);
			Instance->bFoundSingleResult = true;
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
			ItemScore = -1.f;
			Instance->Items[CurrentItem].Discard();
#if USE_EQS_DEBUGGER
			Instance->ItemDetails[CurrentItem].FailedTestIndex = Instance->CurrentTest;
#endif
			Instance->NumValidItems--;
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
	if (Options[OptionIndex].bShuffleItems)
	{
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++)
		{
			const int32 Idx1 = FMath::RandHelper(Items.Num());
			const int32 Idx2 = FMath::RandHelper(Items.Num());
			Items.Swap(Idx1, Idx2);

#if USE_EQS_DEBUGGER
			ItemDetails.Swap(Idx1, Idx2);
#endif
		}
	}

#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		ItemDetails.Sort(FItemSortPredicate(&Items));
	}
	else
	{
#endif
		ItemDetails.Reset();
#if USE_EQS_DEBUGGER
	}
#endif

	Items.Sort(TGreater<FEnvQueryItem>());
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
#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		Items.Swap(0, ItemIndex);
		ItemDetails.Swap(0, ItemIndex);

		// do not limit valid items amount for debugger purposes!
		// bFoundSingleResult can be used to determine if more than one item is valid

		// normalize all scores for debugging
		NormalizeScores();
	}
	else
	{
#endif
		FEnvQueryItem BestItem;
		BestItem.Score = 1.0f;
		BestItem.DataOffset = Items[ItemIndex].DataOffset;

		DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());

		Items.Empty(1);
		Items.Add(BestItem);
		NumValidItems = 1;

		INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Items.GetAllocatedSize());
#if USE_EQS_DEBUGGER
	}
#endif
}

void FEnvQueryInstance::FinalizeQuery()
{
	if (NumValidItems > 0)
	{
		if (Mode == EEnvQueryRunMode::SingleResult)
		{
			// if last test was not pure condition: sort and pick one of best items
			if (!bFoundSingleResult)
			{
				SortScores();
				PickBestItem();
				bFoundSingleResult = true;
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
	const int32 NumTests = OptionInstance.TestDelegates.Num();

	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, ItemDetails.GetAllocatedSize());
	INC_DWORD_STAT_BY(STAT_AI_EQS_NumItems, Items.Num());

	NumValidItems = Items.Num();
	ItemDetails.Reset();
	bPassOnSingleResult = false;
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

	if (Mode == EEnvQueryRunMode::SingleResult && NumTests == 1)
	{
		UEnvQueryTest* DefTestOb = Cast<UEnvQueryTest>(OptionInstance.TestDelegates[0].GetUObject());
		if ((DefTestOb->TestPurpose != EEnvTestPurpose::Score) && // We are filtering and...
			((DefTestOb->TestPurpose == EEnvTestPurpose::Filter) ||			// Either we are NOT scoring at ALL or...
			 (DefTestOb->ScoringEquation == EEnvTestScoreEquation::Constant)// We are giving a constant score value for passing.
			)
		   )
		{
			OnFinalCondition();
		}
	}
}

void FEnvQueryInstance::FinalizeTest()
{
	FEnvQueryOptionInstance& OptionInstance = Options[OptionIndex];
	const int32 NumTests = OptionInstance.TestDelegates.Num();

	UEnvQueryTest* DefTestOb = (UEnvQueryTest*)(OptionInstance.TestDelegates[CurrentTest].GetUObject());
	DefTestOb->NormalizeItemScores(*this);

#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		DebugData.PerformedTestNames.Add(UEnvQueryTypes::GetShortTypeName(DefTestOb).ToString());
	}
#endif

	// check if next test is the last one
	if (Mode == EEnvQueryRunMode::SingleResult && CurrentTest == NumTests - 2)
	{
		DefTestOb = (UEnvQueryTest*)(OptionInstance.TestDelegates[NumTests - 1].GetUObject());
		if ((DefTestOb->TestPurpose != EEnvTestPurpose::Score) && // We are filtering and...
			((DefTestOb->TestPurpose == EEnvTestPurpose::Filter) ||			// Either we are NOT scoring at ALL or...
			 (DefTestOb->ScoringEquation == EEnvTestScoreEquation::Constant)// We are giving a constant score value for passing.
			)
		   )
		{
			OnFinalCondition();
		}
	}
}

void FEnvQueryInstance::OnFinalCondition()
{
	// set flag for item iterator
	bPassOnSingleResult = true;

	UEnvQueryTest* DefTestOb = (UEnvQueryTest*)(Options[OptionIndex].TestDelegates[0].GetUObject());
	const bool bShouldSort = (DefTestOb->WeightModifier < EEnvTestWeight::Constant);

#if USE_EQS_DEBUGGER
	if (bStoreDebugInfo)
	{
		DebugData.Store(this);

		// randomize before sorting, so items with the same score can be chosen randomly
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++)
		{
			const int32 Idx1 = FMath::RandHelper(Items.Num());
			const int32 Idx2 = FMath::RandHelper(Items.Num());
			Items.Swap(Idx1, Idx2);
			ItemDetails.Swap(Idx1, Idx2);
		}

		// keep ItemDetails in sync for debugger purposes
		if (bShouldSort)
		{
			ItemDetails.Sort(FItemSortPredicate(&Items));
		}
	}
	else
	{
#endif
		// don't update ItemDetails here - it won't be needed anymore, just reset it
		ItemDetails.Reset();

		// randomize before sorting, so items with the same score can be chosen randomly
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++)
		{
			Items.Swap(FMath::RandHelper(Items.Num()), FMath::RandHelper(Items.Num()));
		}
#if USE_EQS_DEBUGGER
	}
#endif	

	if (bShouldSort)
	{
		Items.Sort(TGreater<FEnvQueryItem>());
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