// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProfilerClientModule.cpp: Implements the FProfilerClientModule class.
=============================================================================*/

#include "ProfilerClientPrivatePCH.h"
#include "SecureHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogProfile, Log, All);

DECLARE_CYCLE_STAT( TEXT("HandleDataReceived"),	STAT_PC_HandleDataReceived,			STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("ReadStatMessages"),	STAT_PC_ReadStatMessages,			STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("AddStatMessages"),	STAT_PC_AddStatMessages,			STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("GenerateDataFrame"),	STAT_PC_GenerateDataFrame,			STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("AddStatFName"),		STAT_PC_AddStatFName,				STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("AddGroupFName"),		STAT_PC_AddGroupFName,				STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("GenerateCycleGraph"),	STAT_PC_GenerateCycleGraph,			STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("GenerateAccumulator"),STAT_PC_GenerateAccumulator,		STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("FindOrAddStat"),		STAT_PC_FindOrAddStat,				STATGROUP_Profiler );
DECLARE_CYCLE_STAT( TEXT("FindOrAddThread"),	STAT_PC_FindOrAddThread,			STATGROUP_Profiler );

/* FProfilerClientManager structors
 *****************************************************************************/

FProfilerClientManager::FProfilerClientManager( const IMessageBusRef& InMessageBus )
{
#if STATS
	MessageBus = InMessageBus;
	MessageEndpoint = FMessageEndpoint::Builder("FProfilerClientModule", InMessageBus)
		.Handling<FProfilerServiceAuthorize>(this, &FProfilerClientManager::HandleServiceAuthorizeMessage)
		.Handling<FProfilerServiceAuthorize2>(this, &FProfilerClientManager::HandleServiceAuthorize2Message)
		.Handling<FProfilerServiceData2>(this, &FProfilerClientManager::HandleServiceData2Message)
		.Handling<FProfilerServicePreviewAck>(this, &FProfilerClientManager::HandleServicePreviewAckMessage)
		.Handling<FProfilerServiceMetaData>(this, &FProfilerClientManager::HandleServiceMetaDataMessage)
		.Handling<FProfilerServiceFileChunk>(this, &FProfilerClientManager::HandleServiceFileChunk)  
		.Handling<FProfilerServicePing>(this, &FProfilerClientManager::HandleServicePingMessage);

	if (MessageEndpoint.IsValid())
	{
		InMessageBus->OnShutdown().AddRaw(this, &FProfilerClientManager::HandleMessageBusShutdown);
		MessageEndpoint->Subscribe<FProfilerServicePing>();
	}

	TickDelegate = FTickerDelegate::CreateRaw(this, &FProfilerClientManager::HandleTicker);
	MessageDelegate = FTickerDelegate::CreateRaw(this, &FProfilerClientManager::HandleMessagesTicker);
	LastPingTime = FDateTime::Now();
	RetryTime = 5.f;

#if PROFILER_THREADED_LOAD
	LoadTask = nullptr;
#endif
	LoadConnection = nullptr;
	FTicker::GetCoreTicker().AddTicker(MessageDelegate, 0.05f);
#endif
}

FProfilerClientManager::~FProfilerClientManager()
{
#if STATS
	// Delete all active file writers and remove temporary files.
	for( auto It = ActiveTransfers.CreateIterator(); It; ++It )
	{
		FReceivedFileInfo& ReceivedFileInfo = It.Value();
		
		delete ReceivedFileInfo.FileWriter;
		ReceivedFileInfo.FileWriter = nullptr;

		IFileManager::Get().Delete( *ReceivedFileInfo.DestFilepath );

		UE_LOG(LogProfile, Log, TEXT( "File service-client transfer aborted: %s" ), *It.Key() );
	}

	FTicker::GetCoreTicker().RemoveTicker(MessageDelegate);
	FTicker::GetCoreTicker().RemoveTicker(TickDelegate);

	Unsubscribe();

	if (MessageBus.IsValid())
	{
		MessageBus->OnShutdown().RemoveAll(this);
	}

	LoadConnection = nullptr;
#endif
}


/* IProfilerClient interface
 *****************************************************************************/

void FProfilerClientManager::Subscribe( const FGuid& Session )
{
#if STATS
	FGuid OldSessionId = ActiveSessionId;
	PendingSessionId = Session;
	if (MessageEndpoint.IsValid())
	{
		if (OldSessionId.IsValid())
		{
			TArray<FGuid> Instances;
			Connections.GenerateKeyArray(Instances);
			for (int32 i = 0; i < Instances.Num(); ++i)
			{
				MessageEndpoint->Publish(new FProfilerServiceUnsubscribe(OldSessionId, Instances[i]), EMessageScope::Network);

				// fire the disconnection delegate
				ProfilerClientDisconnectedDelegate.Broadcast(ActiveSessionId, Instances[i]);
			}

			ActiveSessionId.Invalidate();
		}
		ActiveSessionId = PendingSessionId;
	}

	Connections.Reset();
#endif
}

void FProfilerClientManager::Track( const FGuid& Instance )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid() && !PendingInstances.Contains(Instance))
	{
		PendingInstances.Add(Instance);

		MessageEndpoint->Publish(new FProfilerServiceSubscribe(ActiveSessionId, Instance), EMessageScope::Network);

		RetryTime = 5.f;
		FTicker::GetCoreTicker().AddTicker(TickDelegate, RetryTime);
	}
#endif
}

void FProfilerClientManager::Track( const TArray<ISessionInstanceInfoPtr>& Instances )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		TArray<FGuid> ActiveInstances;
		Connections.GenerateKeyArray(ActiveInstances);

		for (int32 i = 0; i < Instances.Num(); ++i)
		{
			if (Connections.Find(Instances[i]->GetInstanceId()) == nullptr)
			{
				Track(Instances[i]->GetInstanceId());
			}
			else
			{
				ActiveInstances.Remove(Instances[i]->GetInstanceId());
			}
		}

		for (int32 i = 0; i < ActiveInstances.Num(); ++i)
		{
			Untrack(ActiveInstances[i]);
		}
	}
#endif
}

void FProfilerClientManager::Untrack( const FGuid& Instance )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		MessageEndpoint->Publish(new FProfilerServiceUnsubscribe(ActiveSessionId, Instance), EMessageScope::Network);
		Connections.Remove(Instance);

		// fire the disconnection delegate
		ProfilerClientDisconnectedDelegate.Broadcast(ActiveSessionId, Instance);
	}
#endif
}

void FProfilerClientManager::Unsubscribe()
{
#if STATS
	PendingSessionId.Invalidate();
	Subscribe(PendingSessionId);
#endif
}

void FProfilerClientManager::SetCaptureState( const bool bRequestedCaptureState, const FGuid& InstanceId /*= FGuid()*/  )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		if( !InstanceId.IsValid() )
		{
			TArray<FMessageAddress> Instances;
			for (auto It = Connections.CreateConstIterator(); It; ++It)
			{
				Instances.Add(It.Value().ServiceAddress);
			}
			MessageEndpoint->Send(new FProfilerServiceCapture(bRequestedCaptureState), Instances);
		}
		else
		{
			const FMessageAddress* MessageAddress = &Connections.Find(InstanceId)->ServiceAddress;
			if( MessageAddress )
			{
				MessageEndpoint->Send(new FProfilerServiceCapture(bRequestedCaptureState), *MessageAddress);
			}
		}
	}
#endif
}

void FProfilerClientManager::SetPreviewState( const bool bRequestedPreviewState, const FGuid& InstanceId /*= FGuid()*/ )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		if( !InstanceId.IsValid() )
		{
			TArray<FMessageAddress> Instances;
			for (auto It = Connections.CreateConstIterator(); It; ++It)
			{
				Instances.Add(It.Value().ServiceAddress);
			}
			MessageEndpoint->Send(new FProfilerServicePreview(bRequestedPreviewState), Instances);
		}
		else
		{
			const FMessageAddress* MessageAddress = &Connections.Find(InstanceId)->ServiceAddress;
			if( MessageAddress )
			{
				MessageEndpoint->Send(new FProfilerServicePreview(bRequestedPreviewState), *MessageAddress);
			}
		}
	}
#endif
}

void FProfilerClientManager::LoadCapture( const FString& DataFilepath, const FGuid& ProfileId )
{
#if STATS
	// start an async load
	LoadConnection = &Connections.FindOrAdd(ProfileId);
	LoadConnection->InstanceId = ProfileId;
	LoadConnection->MetaData.CriticalSection = &LoadConnection->CriticalSection;
	LoadConnection->MetaData.SecondsPerCycle = FPlatformTime::GetSecondsPerCycle(); // fix this by adding a message which specifies this

	int64 Size = IFileManager::Get().FileSize(*DataFilepath);
	if (Size < 4)
	{
		return;
	}
#if PROFILER_THREADED_LOAD
	FArchive* FileReader = IFileManager::Get().CreateFileReader(*DataFilepath);
#else
	FileReader = IFileManager::Get().CreateFileReader(*DataFilepath);
#endif
	if (!FileReader)
	{
		return;
	}

#if PROFILER_THREADED_LOAD
	LoadTask = new FAsyncTask<FAsyncReadWorker>(LoadConnection, FileReader);
	LoadTask->StartBackgroundTask();
#else

	uint32 Magic = 0;
	*FileReader << Magic; // deal with header magic next
#endif

	RetryTime = 0.05f;
	FTicker::GetCoreTicker().AddTicker(TickDelegate, RetryTime);
	ProfilerLoadStartedDelegate.Broadcast(ProfileId);
#endif
}


void FProfilerClientManager::RequestMetaData()
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		TArray<FMessageAddress> Instances;
		for (auto It = Connections.CreateConstIterator(); It; ++It)
		{
			Instances.Add(It.Value().ServiceAddress);
		}
		MessageEndpoint->Send(new FProfilerServiceRequest(EProfilerRequestType::PRT_MetaData), Instances);
	}
#endif
}

void FProfilerClientManager::RequestLastCapturedFile( const FGuid& InstanceId /*= FGuid()*/ )
{
#if STATS
	if (MessageEndpoint.IsValid() && ActiveSessionId.IsValid())
	{
		if( !InstanceId.IsValid() )
		{
			TArray<FMessageAddress> Instances;
			for (auto It = Connections.CreateConstIterator(); It; ++It)
			{
				Instances.Add(It.Value().ServiceAddress);
			}
			MessageEndpoint->Send(new FProfilerServiceRequest(EProfilerRequestType::PRT_SendLastCapturedFile), Instances);
		}
		else
		{
			const FMessageAddress* MessageAddress = &Connections.Find(InstanceId)->ServiceAddress;
			if( MessageAddress )
			{
				MessageEndpoint->Send(new FProfilerServiceRequest(EProfilerRequestType::PRT_SendLastCapturedFile), *MessageAddress);
			}
		}
	}
#endif
}

/* FProfilerClientManager event handlers
 *****************************************************************************/

void FProfilerClientManager::HandleMessageBusShutdown()
{
#if STATS
	Unsubscribe();

	MessageEndpoint.Reset();
	MessageBus.Reset();
#endif
}


void FProfilerClientManager::HandleServiceAuthorizeMessage( const FProfilerServiceAuthorize& Message, const IMessageContextRef& Context )
{
#if STATS
	if (ActiveSessionId == Message.SessionId && PendingInstances.Contains(Message.InstanceId))
	{
		PendingInstances.Remove(Message.InstanceId);
		FServiceConnection& Connection = Connections.FindOrAdd(Message.InstanceId);
		Connection.ServiceAddress = Context->GetSender();
		Connection.InstanceId = Message.InstanceId;
		Connection.CurrentData.Frame = 0;
	}

	// Fire the client connection event
	ProfilerClientConnectedDelegate.Broadcast(ActiveSessionId, Message.InstanceId);
#endif
}


void FProfilerClientManager::HandleServiceAuthorize2Message( const FProfilerServiceAuthorize2& Message, const IMessageContextRef& Context )
{
#if STATS
	if (ActiveSessionId == Message.SessionId && PendingInstances.Contains(Message.InstanceId))
	{
		PendingInstances.Remove(Message.InstanceId);
		FServiceConnection& Connection = Connections.FindOrAdd(Message.InstanceId);
		Connection.Initialize(Message, Context);

		// Fire a meta data update message
		ProfilerMetaDataUpdatedDelegate.Broadcast(Message.InstanceId);

		// Fire the client connection event
		ProfilerClientConnectedDelegate.Broadcast(ActiveSessionId, Message.InstanceId);
	}
#endif
}

void FServiceConnection::Initialize( const FProfilerServiceAuthorize2& Message, const IMessageContextRef& Context )
{
#if STATS
	ServiceAddress = Context->GetSender();
	InstanceId = Message.InstanceId;
	CurrentData.Frame = 0;

	// add the supplied meta data
	FArrayReader ArrayReader(true);
	ArrayReader.Append(Message.Data);

	MetaData.CriticalSection = &CriticalSection;
	int64 Size = ArrayReader.TotalSize();

	// read in the magic, at some point may need to know what it actually is
	uint32 Magic = 0;
	ArrayReader << Magic;

	// read in the data
	TArray<FStatMessage> StatMessages;
	{
		SCOPE_CYCLE_COUNTER(STAT_PC_ReadStatMessages);
		while(ArrayReader.Tell() < Size)
		{
			// read the message
			new (StatMessages) FStatMessage(Stream.ReadMessage(ArrayReader));
		}
		static FStatNameAndInfo Adv(NAME_AdvanceFrame, "", TEXT(""), EStatDataType::ST_int64, true, false);
		new (StatMessages) FStatMessage(Adv.GetEncodedName(), EStatOperation::AdvanceFrameEventGameThread, 1LL, false);
	}

	// generate a thread state from the data
	{
		SCOPE_CYCLE_COUNTER(STAT_PC_AddStatMessages);
		CurrentThreadState.AddMessages(StatMessages);
	}

	UpdateMetaData();
#endif
}

void FProfilerClientManager::HandleServiceMetaDataMessage( const FProfilerServiceMetaData& Message, const IMessageContextRef& Context )
{
#if STATS
	if (ActiveSessionId.IsValid() && Connections.Find(Message.InstanceId) != nullptr)
	{
		FServiceConnection& Connection = *Connections.Find(Message.InstanceId);

		FArrayReader ArrayReader(true);
		ArrayReader.Append(Message.Data);

		FStatMetaData NewData;
		ArrayReader << NewData;

		// add in the new data
		if (NewData.StatDescriptions.Num() > 0)
		{
			Connection.MetaData.StatDescriptions.Append(NewData.StatDescriptions);
		}
		if (NewData.GroupDescriptions.Num() > 0)
		{
			Connection.MetaData.GroupDescriptions.Append(NewData.GroupDescriptions);
		}
		if (NewData.ThreadDescriptions.Num() > 0)
		{
			Connection.MetaData.ThreadDescriptions.Append(NewData.ThreadDescriptions);
		}
		Connection.MetaData.SecondsPerCycle = NewData.SecondsPerCycle;

		// Fire a meta data update message
		ProfilerMetaDataUpdatedDelegate.Broadcast(Message.InstanceId);
	}
#endif
}


bool FProfilerClientManager::CheckHashAndWrite( const FProfilerServiceFileChunk& FileChunk, const FProfilerFileChunkHeader& FileChunkHeader, FArchive* Writer )
{
#if STATS
	const int32 HashSize = 20;
	uint8 LocalHash[HashSize]={0};
	
	// Hash file chunk data. 
	FSHA1 Sha;
	Sha.Update( FileChunk.Data.GetData(), FileChunkHeader.ChunkSize );
	// Hash file chunk header.
	Sha.Update( FileChunk.Header.GetData(), FileChunk.Header.Num() );
	Sha.Final();
	Sha.GetHash( LocalHash );

	const int32 MemDiff = FMemory::Memcmp( FileChunk.ChunkHash.GetData(), LocalHash, HashSize );

	bool bResult = false;

	if( MemDiff == 0 )
	{
		// Write the data to the archive.
		Writer->Seek( FileChunkHeader.ChunkOffset );
		Writer->Serialize( (void*)FileChunk.Data.GetData(), FileChunkHeader.ChunkSize );

		bResult = true;
	}

	return bResult;
#else
	return false;
#endif
}

void FProfilerClientManager::HandleServiceFileChunk( const FProfilerServiceFileChunk& FileChunk, const IMessageContextRef& Context )
{
#if STATS
	const TCHAR* StrTmp = TEXT(".tmp");

	// Read file chunk header.
	FMemoryReader Reader(FileChunk.Header);
	FProfilerFileChunkHeader FileChunkHeader;
	Reader << FileChunkHeader;
	FileChunkHeader.Validate();

	const bool bValidFileChunk = !FailedTransfer.Contains( FileChunk.Filename );

	// TODO: @JarekS 2013-10-01, 14:48 At this moment received file chunks are handled on the main thread, asynchronous file receiving is planned for the future release.
	if (ActiveSessionId.IsValid() && Connections.Find(FileChunk.InstanceId) != nullptr && bValidFileChunk)
	{
		FReceivedFileInfo* ReceivedFileInfo = ActiveTransfers.Find( FileChunk.Filename );
		if( !ReceivedFileInfo )
		{
			const FString PathName = FPaths::ProfilingDir() + TEXT("UnrealStats/Received/");
			const FString StatFilepath = PathName + FileChunk.Filename + StrTmp;

			UE_LOG(LogProfile, Log, TEXT( "Opening stats file for service-client sending: %s" ), *StatFilepath );

			FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*StatFilepath);
			if( !FileWriter )
			{
				UE_LOG(LogProfile, Error, TEXT( "Could not open: %s" ), *StatFilepath );
				return;
			}

			ReceivedFileInfo = &ActiveTransfers.Add( FileChunk.Filename, FReceivedFileInfo(FileWriter,0,StatFilepath) );
			ProfilerFileTransferDelegate.Broadcast( FileChunk.Filename, ReceivedFileInfo->Progress, FileChunkHeader.FileSize );
		}

		const bool bSimulateBadFileChunk = true;//FMath::Rand() % 10 != 0;
		const bool bSuccess = CheckHashAndWrite( FileChunk, FileChunkHeader, ReceivedFileInfo->FileWriter ) && bSimulateBadFileChunk;	
		if( bSuccess )
		{
			ReceivedFileInfo->Progress += FileChunkHeader.ChunkSize;
			ReceivedFileInfo->Update();

			if( ReceivedFileInfo->Progress == FileChunkHeader.FileSize )
			{
				// File has been successfully sent, so send this information to the profiler service.
				if( MessageEndpoint.IsValid() )
				{
					MessageEndpoint->Send( new FProfilerServiceFileChunk( FGuid(),FileChunk.Filename,FProfilerFileChunkHeader(0,0,0,EProfilerFileChunkType::FinalizeFile).AsArray() ), Context->GetSender() );
					ProfilerFileTransferDelegate.Broadcast( FileChunk.Filename, ReceivedFileInfo->Progress, FileChunkHeader.FileSize );
				}
				
				// Delete the file writer.
				delete ReceivedFileInfo->FileWriter;
				ReceivedFileInfo->FileWriter = nullptr;

				// Rename the stats file.
				IFileManager::Get().Move( *ReceivedFileInfo->DestFilepath.Replace( StrTmp, TEXT("") ), *ReceivedFileInfo->DestFilepath );

				ActiveTransfers.Remove( FileChunk.Filename );

				UE_LOG(LogProfile, Log, TEXT( "File service-client received successfully: %s" ), *FileChunk.Filename );
			}
			else
			{
				ProfilerFileTransferDelegate.Broadcast( FileChunk.Filename, ReceivedFileInfo->Progress, FileChunkHeader.FileSize );			
			}
		}
		else
		{
			// This chunk is a bad chunk, so ask for resending it.
			if( MessageEndpoint.IsValid() )
			{
				MessageEndpoint->Send( new FProfilerServiceFileChunk(FileChunk,FProfilerServiceFileChunk::FNullTag()), Context->GetSender() );
				UE_LOG(LogProfile, Log, TEXT("Received a bad chunk of file, resending: %5i, %6u, %10u, %s"), FileChunk.Data.Num(), ReceivedFileInfo->Progress, FileChunkHeader.FileSize, *FileChunk.Filename );
			}
		}
	}
#endif
}

void FProfilerClientManager::HandleServicePingMessage( const FProfilerServicePing& Message, const IMessageContextRef& Context )
{
#if STATS
	if (MessageEndpoint.IsValid())
	{
		TArray<FMessageAddress> Instances;
		for (auto It = Connections.CreateConstIterator(); It; ++It)
		{
			Instances.Add(It.Value().ServiceAddress);
		}
		MessageEndpoint->Send(new FProfilerServicePong(), Instances);
	}
#endif
}

#if PROFILER_THREADED_LOAD
bool FProfilerClientManager::AsyncLoad()
{
#if STATS
	while (LoadConnection->DataFrames.Num() > 0)
	{
		// Fire a meta data update message
		if (LoadConnection->DataFrames[0].MetaDataUpdated)
		{
			ProfilerMetaDataUpdatedDelegate.Broadcast(LoadConnection->InstanceId);
			ProfilerLoadedMetaDataDelegate.Broadcast(LoadConnection->InstanceId);
		}

		FScopeLock ScopeLock(&(LoadConnection->CriticalSection));
		FProfilerDataFrame& DataFrame = LoadConnection->DataFrames[0];
		ProfilerDataDelegate.Broadcast(LoadConnection->InstanceId, DataFrame, LoadConnection->DataLoadingProgress);
		LoadConnection->DataFrames.RemoveAt(0);
	}

	if (LoadTask->IsDone())
	{
		ProfilerLoadCompletedDelegate.Broadcast(LoadConnection->InstanceId);
		LoadConnection = nullptr;
		delete LoadTask;
		LoadTask = nullptr;
		RetryTime = 5.f;
		return false;
	}
#endif
	return true;
}
#else
bool FProfilerClientManager::SyncLoad()
{
#if STATS
	SCOPE_CYCLE_COUNTER(STAT_PC_ReadStatMessages);

	uint64 ReadMessages = 0;

	while(FileReader->Tell() < FileReader->TotalSize() && LoadConnection->DataFrames.Num() < FProfilerClientManager::MaxFramesPerTick)
	{
		// read the message
		FStatMessage Message(LoadConnection->Stream.ReadMessage(*FileReader));
		ReadMessages++;

		if (Message.NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread && ReadMessages > 2 )
		{
			new (LoadConnection->Messages) FStatMessage(Message);
			SCOPE_CYCLE_COUNTER(STAT_PC_AddStatMessages);
			LoadConnection->CurrentThreadState.AddMessages(LoadConnection->Messages);
			LoadConnection->Messages.Reset(LoadConnection->Messages.Num());

			// convert to the old format
			// create an old format data frame from the data
			FProfilerDataFrame& DataFrame = LoadConnection->CurrentData;
			{
				SCOPE_CYCLE_COUNTER(STAT_PC_GenerateDataFrame);
				DataFrame.Frame = LoadConnection->CurrentThreadState.CurrentGameFrame;
				DataFrame.FrameStart = 0.0;
				DataFrame.CountAccumulators.Reset(DataFrame.CountAccumulators.Num());
				DataFrame.CycleGraphs.Reset();
				DataFrame.FloatAccumulators.Reset(DataFrame.FloatAccumulators.Num());

				// get the stat stack root and the non frame stats
				FRawStatStackNode Stack;
				TArray<FStatMessage> NonFrameStats;
				LoadConnection->CurrentThreadState.UncondenseStackStats(LoadConnection->CurrentThreadState.CurrentGameFrame, Stack, nullptr, &NonFrameStats );

				// @todo updating metadata here is not thread safe?
				// cycle graphs
				LoadConnection->GenerateCycleGraphs(Stack, DataFrame.CycleGraphs);

				// accumulators
				LoadConnection->GenerateAccumulators(NonFrameStats, DataFrame.CountAccumulators, DataFrame.FloatAccumulators);

				// add the frame to the work list
				LoadConnection->DataFrames.Add(LoadConnection->CurrentData);
				LoadConnection->DataLoadingProgress = (double)FileReader->Tell() / (double)FileReader->TotalSize();
			}
		}

		new (LoadConnection->Messages) FStatMessage(Message);
	}

	while (LoadConnection->DataFrames.Num() > 0)
	{
		// Fire a meta data update message
		if (LoadConnection->DataFrames[0].MetaDataUpdated)
		{
			ProfilerMetaDataUpdatedDelegate.Broadcast(LoadConnection->InstanceId);
			ProfilerLoadedMetaDataDelegate.Broadcast(LoadConnection->InstanceId);
		}

		// update the frames
		FProfilerDataFrame& DataFrame = LoadConnection->DataFrames[0];
		ProfilerDataDelegate.Broadcast(LoadConnection->InstanceId, DataFrame, LoadConnection->DataLoadingProgress);
		LoadConnection->DataFrames.RemoveAt(0);
	}

	if (FileReader->Tell() >= FileReader->TotalSize())
	{
		ProfilerLoadCompletedDelegate.Broadcast(LoadConnection->InstanceId);
		LoadConnection = nullptr;
		delete FileReader;
		FileReader = nullptr;
		RetryTime = 5.f;
		return false;
	}
#endif
	return true;
}
#endif

bool FProfilerClientManager::HandleTicker( float DeltaTime )
{
#if STATS
	if (PendingInstances.Num() > 0 && FDateTime::Now() > LastPingTime + DeltaTime)
	{
		TArray<FGuid> Instances;
		Instances.Append(PendingInstances);

		PendingInstances.Reset();

		for (int32 i = 0; i < Instances.Num(); ++i)
		{
			Track(Instances[i]);
		}
		LastPingTime = FDateTime::Now();
	}
	else if (LoadConnection)
	{
#if PROFILER_THREADED_LOAD
		return AsyncLoad();
#else
		return SyncLoad();
#endif
	}
#endif
	return false;
}

bool FProfilerClientManager::HandleMessagesTicker( float DeltaTime )
{
#if STATS
	for (auto It = Connections.CreateIterator(); It; ++It)
	{
		FServiceConnection& Connection = It.Value();

		while (Connection.PendingMessages.Contains(Connection.CurrentFrame+1))
		{
			TArray<uint8>& Data = *Connection.PendingMessages.Find(Connection.CurrentFrame+1);
			Connection.CurrentFrame++;

			// pass the data to the visualization code
			FArrayReader Reader(true);
			Reader.Append(Data);

			int64 Size = Reader.TotalSize();
			FStatsReadStream& Stream = Connection.Stream;

			// read in the data and post if we reach a 
			{
				SCOPE_CYCLE_COUNTER(STAT_PC_ReadStatMessages);
				while(Reader.Tell() < Size)
				{
					// read the message
					FStatMessage Message(Stream.ReadMessage(Reader));

					if (Message.NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread)
					{
						new (Connection.Messages) FStatMessage(Message);
						SCOPE_CYCLE_COUNTER(STAT_PC_AddStatMessages);
						Connection.CurrentThreadState.AddMessages(Connection.Messages);
						Connection.Messages.Reset();
					}

					new (Connection.Messages) FStatMessage(Message);
				}
			}

			// create an old format data frame from the data
			{
				SCOPE_CYCLE_COUNTER(STAT_PC_GenerateDataFrame);
				FProfilerDataFrame& DataFrame = Connection.CurrentData;
				DataFrame.Frame = Connection.CurrentThreadState.CurrentGameFrame;
				DataFrame.FrameStart = 0.0;
				DataFrame.CountAccumulators.Reset(DataFrame.CountAccumulators.Num());
				DataFrame.CycleGraphs.Reset();
				DataFrame.FloatAccumulators.Reset(DataFrame.FloatAccumulators.Num());

				// get the stat stack root and the non frame stats
				FRawStatStackNode Stack;
				TArray<FStatMessage> NonFrameStats;
				Connection.CurrentThreadState.UncondenseStackStats(Connection.CurrentThreadState.CurrentGameFrame, Stack, nullptr, &NonFrameStats );

				// cycle graphs
				Connection.GenerateCycleGraphs(Stack, DataFrame.CycleGraphs);

				// accumulators
				Connection.GenerateAccumulators(NonFrameStats, DataFrame.CountAccumulators, DataFrame.FloatAccumulators);
			}

			// Fire a meta data update message
			if (Connection.CurrentData.MetaDataUpdated)
			{
				ProfilerMetaDataUpdatedDelegate.Broadcast(Connection.InstanceId);
			}

			// send the data out
			ProfilerDataDelegate.Broadcast(Connection.InstanceId, Connection.CurrentData,0.0f);

			Connection.PendingMessages.Remove(Connection.CurrentFrame);
		}
	}

	// Remove any active transfer that timed out.
	for( auto It = ActiveTransfers.CreateIterator(); It; ++It )
	{
		FReceivedFileInfo& ReceivedFileInfo = It.Value();
		const FString& Filename = It.Key();

		if( ReceivedFileInfo.IsTimedOut() )
		{
			UE_LOG(LogProfile, Log, TEXT( "File service-client timed out, aborted: %s" ), *Filename );
			FailedTransfer.Add( Filename );

			delete ReceivedFileInfo.FileWriter;
			ReceivedFileInfo.FileWriter = nullptr;

			IFileManager::Get().Delete( *ReceivedFileInfo.DestFilepath );
			ProfilerFileTransferDelegate.Broadcast( Filename, -1, -1 );
			It.RemoveCurrent();	
		}
	}
#endif

	return true;
}

void FProfilerClientManager::HandleServicePreviewAckMessage( const FProfilerServicePreviewAck& Message, const IMessageContextRef& Context )
{
#if STATS
	if (ActiveSessionId.IsValid() && Connections.Find(Message.InstanceId) != NULL)
	{
		FServiceConnection& Connection = *Connections.Find(Message.InstanceId);
		Connection.CurrentFrame = Message.Frame;
	}
#endif
}

void FProfilerClientManager::HandleServiceData2Message( const FProfilerServiceData2& Message, const IMessageContextRef& Context )
{
#if STATS
	SCOPE_CYCLE_COUNTER(STAT_PC_HandleDataReceived);
	if (ActiveSessionId.IsValid() && Connections.Find(Message.InstanceId) != nullptr)
	{
		FServiceConnection& Connection = *Connections.Find(Message.InstanceId);

		// add the message to the connections queue
		TArray<uint8> Data;
		Data.Append(Message.Data);
		Connection.PendingMessages.Add(Message.Frame, Data);
	}
#endif
}

void FProfilerClientManager::FAsyncReadWorker::DoWork()
{
#if STATS
	uint32 Magic = 0;
	*FileReader << Magic; // deal with header magic next
/*	if (Magic == EStatMagic::MAGIC)
	{

	}
	else if (Magic == EStatMagic::MAGIC_SWAPPED)
	{
		FileReader->SetByteSwapping(true);
	}
	else
	{
		delete FileReader;
		return;
	}*/

	FStatsReadStream& Stream = Connection.Stream;

	// read in the data and post if we reach a 
	{
		uint64 ReadMessages = 0;

		SCOPE_CYCLE_COUNTER(STAT_PC_ReadStatMessages);
		while(FileReader->Tell() < FileReader->TotalSize())
		{
			// read the message
			FStatMessage Message(Stream.ReadMessage(*FileReader));
			ReadMessages++;

			if (Message.NameAndInfo.GetShortName() != TEXT("Unknown FName"))
			{
				if (Message.NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread && ReadMessages > 2)
				{
					new (Connection.Messages) FStatMessage(Message);
					SCOPE_CYCLE_COUNTER(STAT_PC_AddStatMessages);
					Connection.CurrentThreadState.AddMessages(Connection.Messages);
					Connection.Messages.Reset(Connection.Messages.Num());

					// convert to the old format
					// create an old format data frame from the data
					FProfilerDataFrame& DataFrame = Connection.CurrentData;
					{
						SCOPE_CYCLE_COUNTER(STAT_PC_GenerateDataFrame);
						DataFrame.Frame = Connection.CurrentThreadState.CurrentGameFrame;
						DataFrame.FrameStart = 0.0;
						DataFrame.CountAccumulators.Reset(DataFrame.CountAccumulators.Num());
						DataFrame.CycleGraphs.Reset();
						DataFrame.FloatAccumulators.Reset(DataFrame.FloatAccumulators.Num());

						// get the stat stack root and the non frame stats
						FRawStatStackNode Stack;
						TArray<FStatMessage> NonFrameStats;
						Connection.CurrentThreadState.UncondenseStackStats(Connection.CurrentThreadState.CurrentGameFrame, Stack, nullptr, &NonFrameStats );

						// @todo updating metadata here is not thread safe?
						// cycle graphs
						Connection.GenerateCycleGraphs(Stack, DataFrame.CycleGraphs);

						// accumulators
						Connection.GenerateAccumulators(NonFrameStats, DataFrame.CountAccumulators, DataFrame.FloatAccumulators);

						// add the frame to the work list
						FScopeLock ScopeLock(&Connection.CriticalSection);
						Connection.DataFrames.Add(Connection.CurrentData);
						Connection.DataLoadingProgress = (double)FileReader->Tell() / (double)FileReader->TotalSize();
					}

					if (Connection.DataFrames.Num() > FProfilerClientManager::MaxFramesPerTick)
					{
						while(Connection.DataFrames.Num())
						{
							FPlatformProcess::Sleep(0.001f);
						}
					}
				}

				new (Connection.Messages) FStatMessage(Message);
			}
			else
			{
				break;
			}
		}

		FileReader->Close();
		delete FileReader;
	}
#endif
}

#if STATS
void FServiceConnection::UpdateMetaData()
{
	// loop through the stats meta data messages
	for (auto It = CurrentThreadState.ShortNameToLongName.CreateConstIterator(); It; ++It)
	{
		FStatMessage const& LongName = It.Value();
		uint32 StatType = STATTYPE_Error;
		if (LongName.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
		{
			if (LongName.NameAndInfo.GetFlag(EStatMetaFlags::IsCycle))
			{
				StatType = STATTYPE_CycleCounter;
			}
			else
			{
				StatType = STATTYPE_AccumulatorDWORD;
			}
		}
		else if (LongName.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
		{
			StatType = STATTYPE_AccumulatorFLOAT;
		}
		if (StatType != STATTYPE_Error)
		{
			FindOrAddStat(LongName.NameAndInfo, StatType);
		}
	}
}

int32 FServiceConnection::FindOrAddStat( const FStatNameAndInfo& StatNameAndInfo, uint32 StatType)
{
	SCOPE_CYCLE_COUNTER(STAT_PC_FindOrAddStat);
	const FName LongName = StatNameAndInfo.GetRawName();
	int32* const StatIDPtr = LongNameToStatID.Find( LongName );
	int32 StatID = StatIDPtr != nullptr ? *StatIDPtr : -1;

	if (!StatIDPtr)
	{
		// meta data has been updated
		CurrentData.MetaDataUpdated = true;

		const FName StatName = StatNameAndInfo.GetShortName();
		FName GroupName = StatNameAndInfo.GetGroupName();
		FString Description;
		StatNameAndInfo.GetDescription( Description );

		// do some special stats first
		if (StatName == TEXT("STAT_FrameTime"))
		{
			StatID = LongNameToStatID.Add(LongName, 2);
		}
		else if (StatName == FStatConstants::NAME_ThreadRoot)
		{
			StatID = LongNameToStatID.Add(LongName, 1);
		}
		else
		{
			StatID = LongNameToStatID.Add(LongName, LongNameToStatID.Num()+10);
		}
		check(StatID != -1);

		// add a new stat description to the meta data
		FStatDescription StatDescription;
		StatDescription.ID = StatID;
		StatDescription.Name = Description.Len() > 0 ? Description : StatName.ToString();
		if( StatDescription.Name.Contains( TEXT("STAT_") ) )
		{
			StatDescription.Name = StatDescription.Name.RightChop(FString(TEXT("STAT_")).Len());
		}
		StatDescription.StatType = StatType;

		if( GroupName == NAME_None )
		{	
			// @todo Add more ways to group the stats.
			const int32 Thread_Pos = StatDescription.Name.Find( TEXT("Thread_") );
			const int32 _0Pos = StatDescription.Name.Find( TEXT("_0") );
			const bool bIsThread = Thread_Pos != INDEX_NONE && _0Pos > Thread_Pos;
			// Add a special group for all threads.
			if( bIsThread || StatName == FStatConstants::NAME_ThreadRoot )
			{
				GroupName = TEXT("Threads");
			}
			// Add a special group for all objects.
			else
			{
				GroupName = TEXT("Objects");
			}
		}

		int32* const GroupIDPtr = GroupNameArray.Find( GroupName );
		int32 GroupID = GroupIDPtr != nullptr ? *GroupIDPtr : -1;
		if( !GroupIDPtr )
		{
			// add a new group description to the meta data
			GroupID = GroupNameArray.Add(GroupName, GroupNameArray.Num()+10);
			check( GroupID != -1 );

			FStatGroupDescription GroupDescription;
			GroupDescription.ID = GroupID;
			GroupDescription.Name = GroupName.ToString();
			if( GroupDescription.Name.Contains( TEXT("STATGROUP_") ) )
			{
				GroupDescription.Name = GroupDescription.Name.RightChop(FString(TEXT("STATGROUP_")).Len());
			}

			// add to the meta data
			FScopeLock ScopeLock(&CriticalSection);
			MetaData.GroupDescriptions.Add(GroupDescription.ID, GroupDescription);
		}
		{
			StatDescription.GroupID = GroupID;
			FScopeLock ScopeLock(&CriticalSection);
			MetaData.StatDescriptions.Add(StatDescription.ID, StatDescription);
		}
	}
	// return the stat id
	return StatID;
}

int32 FServiceConnection::FindOrAddThread(const FStatNameAndInfo& Thread)
{
	SCOPE_CYCLE_COUNTER(STAT_PC_FindOrAddThread);

	const FName LongName = Thread.GetRawName();
	int32* const ThreadIDPtr = ThreadNameArray.Find( LongName );
	int32 ThreadID = ThreadIDPtr != nullptr ? *ThreadIDPtr : -1;
	if (!ThreadIDPtr)
	{
		// meta data has been updated
		CurrentData.MetaDataUpdated = true;

		// get the thread description
		FString Desc;
		Thread.GetDescription(Desc);

		// Extract out the thread id and add to the descriptions
		FString ID = Desc.Replace(TEXT("Thread_"), TEXT(""));
		ID = ID.Replace(TEXT("_0"), TEXT(""));
		ThreadID = FParse::HexNumber(*ID);
		ThreadNameArray.Add(LongName, ThreadID);

		// add to the meta data
		FScopeLock ScopeLock(&CriticalSection);
		MetaData.ThreadDescriptions.Add(ThreadID, Thread.GetShortName().ToString());
	}

	return ThreadID;
}

void FServiceConnection::GenerateAccumulators(TArray<FStatMessage>& Stats, TArray<FProfilerCountAccumulator>& CountAccumulators, TArray<FProfilerFloatAccumulator>& FloatAccumulators)
{
	SCOPE_CYCLE_COUNTER(STAT_PC_GenerateAccumulator)
	for (int32 Index = 0; Index < Stats.Num(); ++Index)
	{
		FStatMessage& Stat = Stats[Index];
		if (Stat.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
		{
			// add a count accumulator
			FProfilerCountAccumulator Data;
			Data.StatId = FindOrAddStat(Stat.NameAndInfo, STATTYPE_AccumulatorDWORD);
			Data.Value = Stat.GetValue_int64();
			CountAccumulators.Add(Data);
		}
		else if (Stat.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
		{
			// add a float accumulator
			FProfilerFloatAccumulator Data;
			Data.StatId = FindOrAddStat(Stat.NameAndInfo, STATTYPE_AccumulatorFLOAT);
			Data.Value = Stat.GetValue_double();
			FloatAccumulators.Add(Data);

			const FName StatName = Stat.NameAndInfo.GetShortName();
			if (StatName == TEXT("STAT_SecondsPerCycle"))
			{
				MetaData.SecondsPerCycle = Stat.GetValue_double();
			}
		}
	}
}

void FServiceConnection::CreateGraphRecursively(const FRawStatStackNode* Root, FProfilerCycleGraph& Graph, uint32 InStartCycles)
{
	Graph.FrameStart = InStartCycles;
	Graph.StatId = FindOrAddStat(Root->Meta.NameAndInfo, STATTYPE_CycleCounter);

	// add the data
	if (Root->Meta.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
	{
		if (Root->Meta.NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration))
		{
			Graph.CallsPerFrame = FromPackedCallCountDuration_CallCount(Root->Meta.GetValue_int64());
			Graph.Value = FromPackedCallCountDuration_Duration(Root->Meta.GetValue_int64());
		}
		else
		{
			Graph.CallsPerFrame = 1;
			Graph.Value = Root->Meta.GetValue_int64();
		}
	}

	uint32 ChildStartCycles = InStartCycles;
	TArray<FRawStatStackNode*> ChildArray;
	Root->Children.GenerateValueArray(ChildArray);
	ChildArray.Sort( FStatDurationComparer<FRawStatStackNode>() );
	for( int32 Index = 0; Index < ChildArray.Num(); ++Index )
	{
		const FRawStatStackNode* ChildStat = ChildArray[Index];

		// create the child graph
		FProfilerCycleGraph ChildGraph;
		ChildGraph.ThreadId = Graph.ThreadId;
		CreateGraphRecursively(ChildStat, ChildGraph, ChildStartCycles);

		// add to the graph
		Graph.Children.Add(ChildGraph);

		// update the start cycles
		ChildStartCycles += ChildGraph.Value;
	}
}

void FServiceConnection::GenerateCycleGraphs(const FRawStatStackNode& Root, TMap<uint32, FProfilerCycleGraph>& CycleGraphs)
{
	SCOPE_CYCLE_COUNTER(STAT_PC_GenerateCycleGraph);

	// Initialize the root stat.
	FindOrAddStat(Root.Meta.NameAndInfo, STATTYPE_CycleCounter);

	// get the cycle graph from each child of the stack root
	TArray<FRawStatStackNode*> ChildArray;
	Root.Children.GenerateValueArray(ChildArray);
	for (int32 Index = 0; Index < ChildArray.Num(); ++Index)
	{
		FRawStatStackNode* ThreadNode = ChildArray[Index];
		FProfilerCycleGraph Graph;

		// determine the thread id
		Graph.ThreadId = FindOrAddThread(ThreadNode->Meta.NameAndInfo);

		// create the thread graph
		CreateGraphRecursively(ThreadNode, Graph, 0);

		// add to the map
		CycleGraphs.Add(Graph.ThreadId, Graph);
	}
}
#endif