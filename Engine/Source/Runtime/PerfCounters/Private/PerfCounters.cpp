// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PerfCounters.h"

#define JSON_ARRAY_NAME					TEXT("PerfCounters")
#define JSON_PERFCOUNTER_NAME			TEXT("Name")
#define JSON_PERFCOUNTER_SIZE_IN_BYTES	TEXT("SizeInBytes")

FPerfCounters::~FPerfCounters()
{
	if (SharedMemory)
	{
		FPlatformMemory::UnmapNamedSharedMemoryRegion(SharedMemory);
		SharedMemory = nullptr;
	}
}

bool FPerfCounters::Initialize(const FString& JsonText)
{
	// first load and create all the perf counters, keeping track of the used memory, and only
	// then initialize the memory object

	if (JsonText.IsEmpty())
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : empty JSON configuration."));
		return false;
	}

	TSharedPtr<FJsonObject> JsonObjectPtr;
	TSharedRef<TJsonReader<>> JsonReaderRef = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(JsonReaderRef, JsonObjectPtr) || !JsonObjectPtr.IsValid())
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : could not deserialize performance counters from JSON."));
		return false;
	}

	if (!JsonObjectPtr->HasTypedField<EJson::Array>(JSON_ARRAY_NAME))
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : configuration file does not have %s array."), JSON_ARRAY_NAME);
		return false;
	}

	PerfCounterMap.Empty();
	SIZE_T TotalPerfCountersSizeInBytes = 0;

	auto PerfCounterArrayJson = JsonObjectPtr->GetArrayField(JSON_ARRAY_NAME);
	for (const auto & PerfCounterJsonIt : PerfCounterArrayJson)
	{
		TSharedPtr<FJsonObject> PerfCounterJsonConfig = PerfCounterJsonIt->AsObject();
		FPerfCounter New;
		if (!PerfCounterJsonConfig.IsValid() || !New.Initialize(*PerfCounterJsonConfig.Get()))
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : could not initialize a perfcounter instance (invalid configuration file?)."));
			return false;
		}

		TotalPerfCountersSizeInBytes += New.SizeOf;
		PerfCounterMap.Add(New.Name, New);
	}

	UE_LOG(LogPerfCounters, Log, TEXT("Initialized %d performance counters taking total %d bytes."), PerfCounterMap.Num(), static_cast<int32>(TotalPerfCountersSizeInBytes));

	// if there are no performance counters, just return
	if (PerfCounterMap.Num() == 0 || TotalPerfCountersSizeInBytes == 0)
	{
		return true;	// success?
	}

	// allocate shared memory. Note that size will be adjusted to match page size internally
	SharedMemory = FPlatformMemory::MapNamedSharedMemoryRegion(*UniqueInstanceId, true,
		FPlatformMemory::ESharedMemoryAccess::Read | FPlatformMemory::ESharedMemoryAccess::Write, TotalPerfCountersSizeInBytes);

	if (SharedMemory == nullptr)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : could not allocate shared memory of at least %d bytes size."), TotalPerfCountersSizeInBytes);
		return false;
	}

	// now assign the pointers sequentially
	char * SharedMemAddress = reinterpret_cast<char *>(SharedMemory->GetAddress());
	FMemory::Memzero(SharedMemAddress, TotalPerfCountersSizeInBytes);

	for (auto & Counter : PerfCounterMap)
	{
		Counter.Value.Pointer = SharedMemAddress;
		SharedMemAddress += Counter.Value.SizeOf;
	}

	return true;
}

bool FPerfCounters::FPerfCounter::Initialize(const FJsonObject& JsonObject)
{
	if (!JsonObject.HasTypedField<EJson::String>(JSON_PERFCOUNTER_NAME))
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : perfcounter in the configuration file does not have %s string field."), JSON_PERFCOUNTER_NAME);
		return false;
	}

	const TSharedPtr<FJsonValue>& NameValue = JsonObject.GetField<EJson::String>(JSON_PERFCOUNTER_NAME);
	Name = NameValue->AsString();

	if (!JsonObject.HasTypedField<EJson::Number>(JSON_PERFCOUNTER_SIZE_IN_BYTES))
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : perfcounter '%s' in the configuration file does not have %s number field."), *Name, JSON_PERFCOUNTER_SIZE_IN_BYTES);
		return false;
	}

	const TSharedPtr<FJsonValue>& SizeOfValue = JsonObject.GetField<EJson::Number>(JSON_PERFCOUNTER_SIZE_IN_BYTES);
	SizeOf = static_cast<SIZE_T>(SizeOfValue->AsNumber());

	if (SizeOf == 0)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : perfcounter '%s' in the configuration file has 0 size in bytes."), *Name);
		return false;
	}

	if (SizeOf != 4 && SizeOf != 8)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Initialize : perfcounter '%s' size is neither 4 nor 8 (but %d) -> we don't support non-atomic writes"), *Name, SizeOf);
		return false;
	}

	// cannot assign pointers at this point
	Pointer = nullptr;

	return true;
}

void FPerfCounters::SetPOD(const FString& Name, const void* Ptr, SIZE_T Size)
{
	const FPerfCounter* Counter = PerfCounterMap.Find(Name);
	if (Counter == nullptr)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Set(POD) : perfcounter '%s' could not be found"), *Name);
		return;
	}

	if (Counter->Pointer == nullptr)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Set(POD) : perfcounter '%s' was not initialized properly (not alloted a shared memory)"), *Name);
		return;
	}

	if (Counter->SizeOf != Size)
	{
		UE_LOG(LogPerfCounters, Warning, TEXT("FPerfCounters::Set(POD) : perfcounter '%s' size is different from write (configured as %d bytes, trying to write %d bytes)"), *Name, Counter->SizeOf, Size);
		// while we could allow writing smaller size than configured, this seems like a potential source of hard-to-track bugs... let's better be strict
		return;
	}

	// note - cannot use Memcpy here, write has to be atomic - checked at config time
	check(Counter->SizeOf != 4 || Counter->SizeOf != 8);

	// TODO: check if really atomic, especially 64-bit writes (on 32-bit systems). Compiler can f..k this up.
	if (Counter->SizeOf == 4)
	{
		const int32* SrcPtr = reinterpret_cast<const int32*>(Ptr);
		int32* DstPtr = reinterpret_cast<int32*>(Counter->Pointer);
		*DstPtr = *SrcPtr;
	}
	else if (Counter->SizeOf == 8)
	{
		const int64 * SrcPtr = reinterpret_cast<const int64*>(Ptr);
		int64 * DstPtr = reinterpret_cast<int64*>(Counter->Pointer);
		*DstPtr = *SrcPtr;
	}
};
