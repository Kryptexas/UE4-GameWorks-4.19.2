// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NetworkReplayStreaming.h"
#include "Stats/Stats.h"
#include "Tickable.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/ArrayWriter.h"
#include "Serialization/MemoryReader.h"

class FNetworkReplayVersion;
class FLocalFileNetworkReplayStreamer;

enum class ELocalFileChunkType : uint32
{
	Header,
	ReplayData,
	Checkpoint,
	Event,
	Unknown = 0xFFFFFFFF
};

/** Struct to hold chunk metadata */
struct FLocalFileChunkInfo
{
	FLocalFileChunkInfo() : 
		ChunkType(ELocalFileChunkType::Unknown),
		SizeInBytes(0), 
		Offset(0) 
	{}

	ELocalFileChunkType ChunkType;
	int32 SizeInBytes;
	int64 Offset;
};

/** Struct to hold replay data chunk metadata */
struct FLocalFileReplayDataInfo
{
	FLocalFileReplayDataInfo() :
		ChunkIndex(INDEX_NONE),
		ReplayOffset(0)
	{}

	int32 ChunkIndex;
	int64 ReplayOffset;
};

/** Struct to hold event metadata */
struct FLocalFileEventInfo
{
	FLocalFileEventInfo() : 
		ChunkIndex(INDEX_NONE),
		Time1(0),
		Time2(0),
		SizeInBytes(0),
		Offset(0)
	{}
	
	int32 ChunkIndex;

	FString Id;
	FString Group;
	FString Metadata;
	uint32 Time1;
	uint32 Time2;

	int32 SizeInBytes;
	int64 Offset;
};

/** Struct to hold metadata about an entire replay */
struct FLocalFileReplayInfo
{
	FLocalFileReplayInfo() : 
		LengthInMS(0), 
		NetworkVersion(0),
		Changelist(0),
		TotalDataSizeInBytes(0),
		bIsLive(false),
		bIsValid(false),
		HeaderChunkIndex(INDEX_NONE)
	{}

	int32 LengthInMS;
	uint32 NetworkVersion;
	uint32 Changelist;
	FString	FriendlyName;
	int64 TotalDataSizeInBytes;
	bool bIsLive;
	bool bIsValid;

	int32 HeaderChunkIndex;

	TArray<FLocalFileChunkInfo> Chunks;
	TArray<FLocalFileEventInfo> Checkpoints;
	TArray<FLocalFileEventInfo> Events;
	TArray<FLocalFileReplayDataInfo> DataChunks;
};

/** Archive to wrap the file reader and respect chunk boundaries */
class LOCALFILENETWORKREPLAYSTREAMING_API FLocalFileStreamFArchive : public FArchive
{
public:
	FLocalFileStreamFArchive(FLocalFileNetworkReplayStreamer* InStreamer) :
		Streamer(InStreamer), 
		Pos(0), 
		CurrentChunkIndex(0), 
		CurrentChunkPos(0) 
	{
	}

	/** FArchive overrides */
	virtual void	Serialize(void* V, int64 Length) override;
	virtual int64	Tell() override;
	virtual int64	TotalSize() override;
	virtual void	Seek(int64 InPos) override;
	virtual bool	AtEnd() override;

	/** Seek to the beginning of specified chunk */
	void			SetChunk(int32 ChunkIndex);

	FLocalFileNetworkReplayStreamer* Streamer;
	TUniquePtr<FArchive>	LocalFileAr;
	int64					Pos;
	int32					CurrentChunkIndex;
	TArray<uint8>			CurrentChunkBuffer;
	int32					CurrentChunkPos;
};

/** Archive to wrap header writes that respects FArchive::Flush */
class LOCALFILENETWORKREPLAYSTREAMING_API FLocalFileHeaderFArchive : public FArrayWriter
{
public:
	FLocalFileHeaderFArchive(FLocalFileNetworkReplayStreamer* InStreamer) : Streamer(InStreamer) {}

	virtual void Flush() override;

	FLocalFileNetworkReplayStreamer* Streamer;
};

/** Local file streamer that supports playback/recording to a single file on disk */
class LOCALFILENETWORKREPLAYSTREAMING_API FLocalFileNetworkReplayStreamer : public INetworkReplayStreamer, public FTickableGameObject
{
public:
	FLocalFileNetworkReplayStreamer() :
		StreamerState(EStreamerState::Idle)
	{}

	/** INetworkReplayStreamer implementation */
	virtual void StartStreaming(const FString& CustomName, const FString& FriendlyName, const TArray< FString >& UserNames, bool bRecord, const FNetworkReplayVersion& ReplayVersion, const FOnStreamReadyDelegate& Delegate) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetHeaderArchive() override;
	virtual FArchive* GetStreamingArchive() override;
	virtual FArchive* GetCheckpointArchive() override;
	virtual void FlushCheckpoint(const uint32 TimeInMS) override;
	virtual void GotoCheckpointIndex(const int32 CheckpointIndex, const FOnCheckpointReadyDelegate& Delegate) override;
	virtual void GotoTimeInMS(const uint32 TimeInMS, const FOnCheckpointReadyDelegate& Delegate) override;
	virtual void UpdateTotalDemoTime(uint32 TimeInMS) override;
	virtual uint32 GetTotalDemoTime() const override { return ReplayInfo.LengthInMS; }
	virtual bool IsDataAvailable() const override;
	virtual void SetHighPriorityTimeRange(const uint32 StartTimeInMS, const uint32 EndTimeInMS) override {}
	virtual bool IsDataAvailableForTimeRange(const uint32 StartTimeInMS, const uint32 EndTimeInMS) override { return true; }
	virtual bool IsLoadingCheckpoint() const override { return false; }
	virtual bool IsLive() const override;
	virtual void DeleteFinishedStream(const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate) const override;
	virtual void EnumerateStreams(const FNetworkReplayVersion& ReplayVersion, const FString& UserString, const FString& MetaString, const FOnEnumerateStreamsComplete& Delegate) override;
	virtual void EnumerateStreams(const FNetworkReplayVersion& InReplayVersion, const FString& UserString, const FString& MetaString, const TArray< FString >& ExtraParms, const FOnEnumerateStreamsComplete& Delegate) override;
	virtual void EnumerateRecentStreams(const FNetworkReplayVersion& ReplayVersion, const FString& RecentViewer, const FOnEnumerateStreamsComplete& Delegate) override {}
	virtual ENetworkReplayError::Type GetLastError() const override { return ENetworkReplayError::None; }
	virtual void AddUserToReplay(const FString& UserString) override;
	virtual void AddEvent(const uint32 TimeInMS, const FString& Group, const FString& Meta, const TArray<uint8>& Data) override;
	virtual void AddOrUpdateEvent(const FString& Name, const uint32 TimeInMS, const FString& Group, const FString& Meta, const TArray<uint8>& Data) override;
	virtual void EnumerateEvents(const FString& Group, const FEnumerateEventsCompleteDelegate& EnumerationCompleteDelegate) override;
	virtual void EnumerateEvents(const FString& ReplayName, const FString& Group, const FEnumerateEventsCompleteDelegate& EnumerationCompleteDelegate) override;
	virtual void RequestEventData(const FString& EventID, const FOnRequestEventDataComplete& RequestEventDataComplete) override;
	virtual void SearchEvents(const FString& EventGroup, const FOnEnumerateStreamsComplete& Delegate) override;
	virtual void KeepReplay(const FString& ReplayName, const bool bKeep) override {}
	virtual FString	GetReplayID() const override { return TEXT(""); }
	virtual void SetTimeBufferHintSeconds(const float InTimeBufferHintSeconds) override {}
	virtual void RefreshHeader() override {};
	virtual void DownloadHeader(const FOnDownloadHeaderComplete& Delegate = FOnDownloadHeaderComplete()) override {}

	virtual bool SupportsCompression() const { return false; }
	virtual int32 GetDecompressedSize(FArchive& InCompressed) const { return 0; }
	virtual bool DecompressBuffer(const TArray<uint8>& InCompressed, TArray< uint8 >& OutBuffer) const { return false; }
	virtual bool CompressBuffer(const TArray< uint8 >& InBuffer, TArray< uint8 >& OutCompressed) const { return false; }

	/** FTickableObjectBase implementation */
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

	/** FTickableGameObject implementation */
	virtual bool IsTickableWhenPaused() const override { return true; }

	void FlushHeader();

	/** Currently playing or recording replay metadata */
	FLocalFileReplayInfo ReplayInfo;

protected:
	FLocalFileReplayInfo ReadReplayInfo(const FString& StreamName) const;
	void WriteReplayInfo(const FString& StreamName, FLocalFileReplayInfo& ReplayInfo);

	bool IsNamedStreamLive(const FString& StreamName) const;

	/** Handles the details of loading a checkpoint */
	void GotoCheckpointIndexInternal(int32 CheckpointIndex, const FOnCheckpointReadyDelegate& Delegate, int32 TimeInMS);

	void FlushStream();

	/** Handle to the archive that will read/write the demo header */
	TUniquePtr<FArchive> HeaderPlaybackAr;
	TUniquePtr<FLocalFileHeaderFArchive> HeaderRecordingAr;

	/** Handle to the archive that will read/write network packets */
	TUniquePtr<FLocalFileStreamFArchive> FilePlaybackAr;
	TUniquePtr<FArrayWriter> FileRecordingAr;

	/* Handle to the archive that will read/write checkpoint files */
	TUniquePtr<FArrayReader> CheckpointPlaybackAr;
	TUniquePtr<FArrayWriter> CheckpointRecordingAr;

	/** EStreamerState - Overall state of the streamer */
	enum class EStreamerState
	{
		Idle,					// The streamer is idle. Either we haven't started streaming yet, or we are done
		Recording,				// We are in the process of recording a replay to disk
		Playback,				// We are in the process of playing a replay from disk
	};

	/** Overall state of the streamer */
	EStreamerState StreamerState;

	/** Remember the name of the current stream, if any. */
	FString CurrentStreamName;
};

class LOCALFILENETWORKREPLAYSTREAMING_API FLocalFileNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();
};
