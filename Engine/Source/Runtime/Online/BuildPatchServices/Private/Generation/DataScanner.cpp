// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#if WITH_BUILDPATCHGENERATION

#include "BuildPatchServicesPrivatePCH.h"

#include "DataScanner.h"

#include "ThreadingBase.h"
#include "Async.h"
#include "../BuildPatchHash.h"
#include "../BuildPatchChunk.h"
#include "../BuildPatchManifest.h"
#include "StatsCollector.h"

namespace FBuildPatchServices
{
	const uint32 WindowSize = FBuildPatchData::ChunkDataSize;

	struct FScopeCounter
	{
	public:
		FScopeCounter(FThreadSafeCounter* Counter);
		~FScopeCounter();

	private:
		FThreadSafeCounter* Counter;
	};

	class FDataStructure
	{
	public:
		FDataStructure(const uint64 DataOffset);
		~FDataStructure();

		FORCEINLINE const FGuid& GetCurrentChunkId() const;

		FORCEINLINE void PushKnownChunk(const FGuid& MatchId, uint32 DataSize);
		FORCEINLINE void PushUnknownByte();

		FORCEINLINE void RemapCurrentChunk(const FGuid& NewId);
		FORCEINLINE void CompleteCurrentChunk();

		FORCEINLINE TArray<FChunkPart> GetFinalDataStructure();

	private:
		FGuid NewChunkGuid;
		TArray<FChunkPart> DataStructure;
	};

	class FDataScannerImpl
		: public FDataScanner
	{
	public:
		FDataScannerImpl(const uint64 DataOffset, const TArray<uint8>& Data, const FCloudEnumerationRef& CloudEnumeration, const FDataMatcherRef& DataMatcher, const FStatsCollectorRef& StatsCollector);
		virtual ~FDataScannerImpl();

		virtual bool IsComplete() override;
		virtual FDataScanResult GetResultWhenComplete() override;

	private:
		FDataScanResult ScanData();
		bool ProcessCurrentWindow();

	private:
		const uint64 DataStartOffset;
		TArray<uint8> Data;
		FCloudEnumerationRef CloudEnumeration;
		FDataMatcherRef DataMatcher;
		FStatsCollectorRef StatsCollector;
		FThreadSafeBool bIsComplete;
		FThreadSafeBool bShouldAbort;
		TFuture<FDataScanResult> FutureResult;

	public:
		static FThreadSafeCounter NumIncompleteScanners;
		static FThreadSafeCounter NumRunningScanners;
	};

	FScopeCounter::FScopeCounter(FThreadSafeCounter* InCounter)
		: Counter(InCounter)
	{
		Counter->Increment();
	}

	FScopeCounter::~FScopeCounter()
	{
		Counter->Decrement();
	}

	FDataStructure::FDataStructure(const uint64 DataOffset)
	{
		DataStructure.AddZeroed();
		DataStructure.Top().DataOffset = DataOffset;
		DataStructure.Top().ChunkGuid = NewChunkGuid = FGuid::NewGuid();
	}

	FDataStructure::~FDataStructure()
	{
	}

	const FGuid& FDataStructure::GetCurrentChunkId() const
	{
		return NewChunkGuid;
	}

	void FDataStructure::PushKnownChunk(const FGuid& PotentialMatch, uint32 NumDataInWindow)
	{
		if (DataStructure.Top().PartSize > 0)
		{
			// Add for matched
			DataStructure.AddUninitialized();
			// Add for next
			DataStructure.AddUninitialized();

			// Fill out info
			FChunkPart& PreviousChunkPart = DataStructure[DataStructure.Num() - 3];
			FChunkPart& MatchedChunkPart = DataStructure[DataStructure.Num() - 2];
			FChunkPart& NextChunkPart = DataStructure[DataStructure.Num() - 1];

			MatchedChunkPart.DataOffset = PreviousChunkPart.DataOffset + PreviousChunkPart.PartSize;
			MatchedChunkPart.PartSize = NumDataInWindow;
			MatchedChunkPart.ChunkOffset = 0;
			MatchedChunkPart.ChunkGuid = PotentialMatch;

			NextChunkPart.DataOffset = MatchedChunkPart.DataOffset + MatchedChunkPart.PartSize;
			NextChunkPart.ChunkGuid = PreviousChunkPart.ChunkGuid;
			NextChunkPart.ChunkOffset = PreviousChunkPart.ChunkOffset + PreviousChunkPart.PartSize;
			NextChunkPart.PartSize = 0;
		}
		else
		{
			// Add for next
			DataStructure.AddZeroed();

			// Fill out info
			FChunkPart& MatchedChunkPart = DataStructure[DataStructure.Num() - 2];
			FChunkPart& NextChunkPart = DataStructure[DataStructure.Num() - 1];

			MatchedChunkPart.PartSize = NumDataInWindow;
			MatchedChunkPart.ChunkOffset = 0;
			MatchedChunkPart.ChunkGuid = PotentialMatch;

			NextChunkPart.DataOffset = MatchedChunkPart.DataOffset + MatchedChunkPart.PartSize;
			NextChunkPart.ChunkGuid = NewChunkGuid;
		}
	}

	void FDataStructure::PushUnknownByte()
	{
		DataStructure.Top().PartSize++;
	}

	void FDataStructure::RemapCurrentChunk(const FGuid& NewId)
	{
		for (auto& DataPiece : DataStructure)
		{
			if (DataPiece.ChunkGuid == NewChunkGuid)
			{
				DataPiece.ChunkGuid = NewId;
			}
		}
		NewChunkGuid = NewId;
	}

	void FDataStructure::CompleteCurrentChunk()
	{
		DataStructure.AddZeroed();
		FChunkPart& PreviousPart = DataStructure[DataStructure.Num() - 2];

		// Create next chunk
		NewChunkGuid = FGuid::NewGuid();
		DataStructure.Top().DataOffset = PreviousPart.DataOffset + PreviousPart.PartSize;
		DataStructure.Top().ChunkGuid = NewChunkGuid;
	}

	TArray<FChunkPart> FDataStructure::GetFinalDataStructure()
	{
		if (DataStructure.Top().PartSize == 0)
		{
			DataStructure.Pop(false);
		}
		return MoveTemp(DataStructure);
	}

	FDataScannerImpl::FDataScannerImpl(const uint64 InDataOffset, const TArray<uint8>& InData, const FCloudEnumerationRef& InCloudEnumeration, const FDataMatcherRef& InDataMatcher, const FStatsCollectorRef& InStatsCollector)
		: DataStartOffset(InDataOffset)
		, Data(InData)
		, CloudEnumeration(InCloudEnumeration)
		, DataMatcher(InDataMatcher)
		, StatsCollector(InStatsCollector)
		, bIsComplete(false)
		, bShouldAbort(false)
	{
		NumIncompleteScanners.Increment();
		TFunction<FDataScanResult()> Task = [this]()
		{
			FDataScanResult Result = ScanData();
			FDataScannerImpl::NumIncompleteScanners.Decrement();
			return MoveTemp(Result);
		};
		FutureResult = Async(EAsyncExecution::ThreadPool, Task);
	}

	FDataScannerImpl::~FDataScannerImpl()
	{
		// Make sure the task is complete
		bShouldAbort = true;
		FutureResult.Wait();
	}

	bool FDataScannerImpl::IsComplete()
	{
		return bIsComplete;
	}

	FDataScanResult FDataScannerImpl::GetResultWhenComplete()
	{
		return MoveTemp(FutureResult.Get());
	}

	FDataScanResult FDataScannerImpl::ScanData()
	{
		// Count running scanners
		FScopeCounter ScopeCounter(&NumRunningScanners);

		// Create statistics
		auto* StatRunningScanners = StatsCollector->CreateStat(TEXT("Scanner: Running Scanners"), EStatFormat::Value);
		auto* StatMatchedData = StatsCollector->CreateStat(TEXT("Scanner: Matched Data"), EStatFormat::DataSize);
		FStatsCollector::Accumulate(StatRunningScanners, 1);

		// Init data
		FRollingHash<WindowSize> RollingHash;
		FChunkWriter ChunkWriter(FBuildPatchServicesModule::GetCloudDirectory());
		FDataStructure DataStructure(DataStartOffset);
		TMap<FGuid, FChunkInfo> ChunkInfoLookup;
		TArray<uint8> ChunkBuffer;
		TArray<uint8> NewChunkBuffer;
		uint32 PaddedZeros = 0;
		ChunkInfoLookup.Reserve(Data.Num() / WindowSize);
		ChunkBuffer.SetNumUninitialized(WindowSize);
		NewChunkBuffer.Reserve(WindowSize);

		// Get a copy of the chunk inventory
		TMap<uint64, TSet<FGuid>> ChunkInventory = CloudEnumeration->GetChunkInventory();
		TMap<FGuid, int64> ChunkFileSizes = CloudEnumeration->GetChunkFileSizes();

		// Loop over and process all data
		for (int32 idx = 0; (idx < Data.Num() || PaddedZeros < WindowSize) && !bShouldAbort; ++idx)
		{
			// Consume data
			const uint32 NumDataNeeded = RollingHash.GetNumDataNeeded();
			if (NumDataNeeded > 0)
			{
				uint32 NumConsumedBytes = 0;
				if (idx < Data.Num())
				{
					NumConsumedBytes = FMath::Min<uint32>(NumDataNeeded, Data.Num() - idx);
					RollingHash.ConsumeBytes(&Data[idx], NumConsumedBytes);
					idx += NumConsumedBytes - 1;
				}
				// Zero Pad?
				if (NumConsumedBytes < NumDataNeeded)
				{
					TArray<uint8> Zeros;
					Zeros.AddZeroed(NumDataNeeded - NumConsumedBytes);
					RollingHash.ConsumeBytes(Zeros.GetData(), Zeros.Num());
					PaddedZeros = Zeros.Num();
				}
				check(RollingHash.GetNumDataNeeded() == 0);
				continue;
			}

			const uint64 NumDataInWindow = WindowSize - PaddedZeros;
			const uint64 WindowHash = RollingHash.GetWindowHash();
			// Try find match
			bool bFoundChunkMatch = false;
			if (ChunkInventory.Contains(WindowHash))
			{
				RollingHash.GetWindowData().Serialize(ChunkBuffer.GetData());
				for (FGuid& PotentialMatch : ChunkInventory.FindRef(WindowHash))
				{
					if (DataMatcher->CompareData(PotentialMatch, WindowHash, ChunkBuffer))
					{
						bFoundChunkMatch = true;
						DataStructure.PushKnownChunk(PotentialMatch, NumDataInWindow);
						FChunkInfo& ChunkInfo = ChunkInfoLookup.FindOrAdd(PotentialMatch);
						ChunkInfo.Hash = WindowHash;
						ChunkInfo.IsNew = false;
						break;
					}
				}
			}

			if (bFoundChunkMatch)
			{
				FStatsCollector::Accumulate(StatMatchedData, NumDataInWindow);
				// Clear matched window
				RollingHash.Clear();
				// Decrement idx to include current byte in next window
				--idx;
			}
			else
			{
				// Collect unrecognized bytes
				NewChunkBuffer.Add(RollingHash.GetWindowData().Bottom());
				DataStructure.PushUnknownByte();
				if (NumDataInWindow == 1)
				{
					NewChunkBuffer.AddZeroed(WindowSize - NewChunkBuffer.Num());
				}
				if (NewChunkBuffer.Num() == WindowSize)
				{
					const uint64 NewChunkHash = FRollingHash<WindowSize>::GetHashForDataSet(NewChunkBuffer.GetData());
					for (FGuid& PotentialMatch : ChunkInventory.FindRef(NewChunkHash))
					{
						if (DataMatcher->CompareData(PotentialMatch, NewChunkHash, NewChunkBuffer))
						{
							bFoundChunkMatch = true;
							DataStructure.RemapCurrentChunk(PotentialMatch);
							FChunkInfo& ChunkInfo = ChunkInfoLookup.FindOrAdd(PotentialMatch);
							ChunkInfo.Hash = NewChunkHash;
							ChunkInfo.IsNew = false;
							break;
						}
					}
					if (!bFoundChunkMatch)
					{
						const FGuid& NewChunkGuid = DataStructure.GetCurrentChunkId();
						ChunkWriter.QueueChunk(NewChunkBuffer.GetData(), NewChunkGuid, NewChunkHash);
						FChunkInfo& ChunkInfo = ChunkInfoLookup.FindOrAdd(NewChunkGuid);
						ChunkInfo.Hash = NewChunkHash;
						ChunkInfo.IsNew = true;
					}
					else
					{
						FStatsCollector::Accumulate(StatMatchedData, WindowSize);
					}
					DataStructure.CompleteCurrentChunk();
					NewChunkBuffer.Empty(WindowSize);
				}

				// Roll byte into window
				if (idx < Data.Num())
				{
					RollingHash.RollForward(Data[idx]);
				}
				else
				{
					RollingHash.RollForward(0);
					++PaddedZeros;
				}
			}
		}

		// Collect left-overs
		if (NewChunkBuffer.Num() > 0)
		{
			NewChunkBuffer.AddZeroed(WindowSize - NewChunkBuffer.Num());
			bool bFoundChunkMatch = false;
			const uint64 NewChunkHash = FRollingHash<WindowSize>::GetHashForDataSet(NewChunkBuffer.GetData());
			for (FGuid& PotentialMatch : ChunkInventory.FindRef(NewChunkHash))
			{
				if (DataMatcher->CompareData(PotentialMatch, NewChunkHash, NewChunkBuffer))
				{
					bFoundChunkMatch = true;
					DataStructure.RemapCurrentChunk(PotentialMatch);
					FChunkInfo& ChunkInfo = ChunkInfoLookup.FindOrAdd(PotentialMatch);
					ChunkInfo.Hash = NewChunkHash;
					ChunkInfo.IsNew = false;
					break;
				}
			}
			if (!bFoundChunkMatch)
			{
				const FGuid& NewChunkGuid = DataStructure.GetCurrentChunkId();
				ChunkWriter.QueueChunk(NewChunkBuffer.GetData(), NewChunkGuid, NewChunkHash);
				FChunkInfo& ChunkInfo = ChunkInfoLookup.FindOrAdd(NewChunkGuid);
				ChunkInfo.Hash = NewChunkHash;
				ChunkInfo.IsNew = true;
			}
		}

		// Wait for the chunk writer to finish, and fill out chunk file sizes
		ChunkWriter.NoMoreChunks();
		ChunkWriter.WaitForThread();
		ChunkWriter.GetChunkFilesizes(ChunkFileSizes);
		for (auto& ChunkInfo : ChunkInfoLookup)
		{
			ChunkInfo.Value.ChunkFileSize = ChunkFileSizes[ChunkInfo.Key];
		}

		// Empty data to save RAM
		Data.Empty();

		FStatsCollector::Accumulate(StatRunningScanners, -1);
		bIsComplete = true;
		return FDataScanResult(
			MoveTemp(DataStructure.GetFinalDataStructure()),
			MoveTemp(ChunkInfoLookup));
	}

	FThreadSafeCounter FDataScannerImpl::NumIncompleteScanners;
	FThreadSafeCounter FDataScannerImpl::NumRunningScanners;

	int32 FDataScannerCounter::GetNumIncompleteScanners()
	{
		return FDataScannerImpl::NumIncompleteScanners.GetValue();
	}

	int32 FDataScannerCounter::GetNumRunningScanners()
	{
		return FDataScannerImpl::NumRunningScanners.GetValue();
	}

	FDataScannerRef FDataScannerFactory::Create(const uint64 DataOffset, const TArray<uint8>& Data, const FCloudEnumerationRef& CloudEnumeration, const FDataMatcherRef& DataMatcher, const FStatsCollectorRef& StatsCollector)
	{
		return MakeShareable(new FDataScannerImpl(DataOffset, Data, CloudEnumeration, DataMatcher, StatsCollector));
	}
}

#endif
