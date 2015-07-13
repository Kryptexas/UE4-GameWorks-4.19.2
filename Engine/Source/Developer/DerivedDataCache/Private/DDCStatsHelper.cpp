#include "Core.h"

#include "DDCStatsHelper.h"
#include "CookingStatsModule.h"
#include "ModuleManager.h"


uint32 GetValidTLSSlot()
{
	uint32 TLSSlot = FPlatformTLS::AllocTlsSlot();
	check(FPlatformTLS::IsValidTlsSlot(TLSSlot));
	FPlatformTLS::SetTlsValue(TLSSlot, nullptr);
	return TLSSlot;
}

ICookingStats* CookingStats = nullptr;

struct FDDCStatsTLSStore
{

	FDDCStatsTLSStore() : TransactionGuid(NAME_None), CurrentIndex(0), RootScope(NULL) { }


	FName GenerateNewTransactionName()
	{
		++CurrentIndex;
		if (CurrentIndex <= 1)
		{
			FString TransactionGuidString = TEXT("DDCTransactionId");
			TransactionGuidString += FGuid::NewGuid().ToString();
			TransactionGuid = FName(*TransactionGuidString);
			CurrentIndex = 1;
		}
		check(TransactionGuid != NAME_None);

		return FName(TransactionGuid, CurrentIndex);
	}

	FName TransactionGuid;
	int32 CurrentIndex;
	FDDCScopeStatHelper *RootScope;
};

uint32 CookStatsFDDCStatsTLSStore = GetValidTLSSlot();

FDDCScopeStatHelper::FDDCScopeStatHelper(const TCHAR* CacheKey, const FName& FunctionName ) : bHasParent(false)
{
	FDDCStatsTLSStore* TLSStore = (FDDCStatsTLSStore*)FPlatformTLS::GetTlsValue(CookStatsFDDCStatsTLSStore);
	if (TLSStore == nullptr)
	{
		TLSStore = new FDDCStatsTLSStore();
		FPlatformTLS::SetTlsValue(CookStatsFDDCStatsTLSStore, TLSStore);
	}

	TransactionGuid = TLSStore->GenerateNewTransactionName();

	if (CookingStats == nullptr)
	{
		FCookingStatsModule& CookingStatsModule = FModuleManager::LoadModuleChecked<FCookingStatsModule>(TEXT("CookingStats"));
		CookingStats = &CookingStatsModule.Get();
	}
	FString Temp;
	AddTag(FunctionName, Temp);

	if (TLSStore->RootScope == nullptr)
	{
		TLSStore->RootScope = this;
		bHasParent = false;

		static const FName NAME_CacheKey = FName(TEXT("CacheKey"));
		AddTag(NAME_CacheKey, FString(CacheKey));
	}
	else
	{
		static const FName NAME_Parent = FName(TEXT("Parent"));
		AddTag(NAME_Parent, TLSStore->RootScope->TransactionGuid.ToString());
		
		bHasParent = true;
	}


	static const FName NAME_StartTime(TEXT("StartTime"));
	AddTag(NAME_StartTime, FDateTime::Now().ToString());

	StartTime = FPlatformTime::Seconds();

	// if there is a root scope then we want to put out stuff under a child key of the parent and link to the parent scope
}


FDDCScopeStatHelper::~FDDCScopeStatHelper()
{
	double EndTime = FPlatformTime::Seconds();
	float DurationMS = (EndTime - StartTime) * 1000.0f;

	static const FName NAME_Duration = FName(TEXT("Duration"));
	AddTag(NAME_Duration, FString::Printf(TEXT("%fms"), DurationMS));

	FDDCStatsTLSStore* TLSStore = (FDDCStatsTLSStore*)FPlatformTLS::GetTlsValue(CookStatsFDDCStatsTLSStore);
	check(TLSStore);

	if (TLSStore->RootScope == this)
	{
		TLSStore->RootScope = nullptr;
	}
}

void FDDCScopeStatHelper::AddTag(const FName& Tag, const FString& Value)
{
	CookingStats->AddTagValue(TransactionGuid, Tag, Value);
}