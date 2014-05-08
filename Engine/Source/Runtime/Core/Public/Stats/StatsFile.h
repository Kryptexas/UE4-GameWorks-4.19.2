// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StatsFile.cpp: Declares stats file related functionality.
=============================================================================*/

#pragma once

#if	STATS

/**
* Magic numbers for stats streams, this is for the first version.
*/
namespace EStatMagicNoHeader
{
	enum Type
	{
		MAGIC = 0x7E1B83C1,
		MAGIC_SWAPPED = 0xC1831B7E,
		NO_VERSION = 0,
	};
}

/**
* Magic numbers for stats streams, this is for the second and later versions.
* Allows more advanced options.
*/
namespace EStatMagicWithHeader
{
	enum Type
	{
		MAGIC = 0x10293847,
		MAGIC_SWAPPED = 0x47382910,
		VERSION_2 = 2,
		VERSION_3 = 3,
	};
}

/*-----------------------------------------------------------------------------
	Stats file writing functionality
-----------------------------------------------------------------------------*/

/** Header for a stats file. */
struct FStatsStreamHeader
{
	/** Default constructor. */
	FStatsStreamHeader()
		: Version( 0 )
		, FrameTableOffset( 0 )
		, FNameTableOffset( 0 )
		, NumFNames( 0 )
		, MetadataMessagesOffset( 0 )
		, NumMetadataMessages( 0 )
		, bRawStatFile( false )
	{}

	/**
	*	Version number to detect version mismatches
	*	1 - Initial version for supporting raw ue4 stats files
	*/
	uint32	Version;

	/** Platform that this file was captured on. */
	FString	PlatformName;

	/** Offset in the file for the frame table. Serialized as TArray<int64>. */
	uint64	FrameTableOffset;

	/** Offset in the file for the FName table. Serialized with WriteFName/ReadFName. */
	uint64	FNameTableOffset;

	/** Number of the FNames. */
	uint64	NumFNames;

	/** Offset in the file for the metadata messages. Serialized with WriteMessage/ReadMessage. */
	uint64	MetadataMessagesOffset;

	/** Number of the metadata messages. */
	uint64	NumMetadataMessages;

	/** Whether this stats file uses raw data, required for thread view. */
	bool bRawStatFile;

	/** Whether this stats file has been finalized, so we have the whole data and can load file a unordered access. */
	bool IsFinalized() const
	{
		return NumMetadataMessages > 0 && MetadataMessagesOffset > 0 && FrameTableOffset > 0;
	}

	/** Serialization operator. */
	friend FArchive& operator << (FArchive& Ar, FStatsStreamHeader& Header)
	{
		Ar << Header.Version;

		if( Ar.IsSaving() )
		{
			Header.PlatformName.SerializeAsANSICharArray( Ar, 255 );
		}
		else if( Ar.IsLoading() )
		{
			Ar << Header.PlatformName;
		}

		Ar << Header.FrameTableOffset;

		Ar << Header.FNameTableOffset
			<< Header.NumFNames;

		Ar << Header.MetadataMessagesOffset
			<< Header.NumMetadataMessages;

		Ar << Header.bRawStatFile;

		return Ar;
	}
};

/**
 * Contains basic information about one frame of the stats.
 * This data is used to generate ultra-fast preview of the stats history without the requirement of reading the whole file.
 */
struct FStatsFrameInfo
{
	/** Empty constructor. */
	FStatsFrameInfo()
		: FrameFileOffset(0)
	{}

	/** Initialization constructor. */
	FStatsFrameInfo( int64 InFrameFileOffset, TMap<uint32, int64>& InThreadCycles )
		: FrameFileOffset(InFrameFileOffset)
		, ThreadCycles(InThreadCycles)
	{}

	/** Serialization operator. */
	friend FArchive& operator << (FArchive& Ar, FStatsFrameInfo& Data)
	{
		Ar << Data.ThreadCycles << Data.FrameFileOffset;
		return Ar;
	}

	/** Frame offset in the stats file. */
	int64 FrameFileOffset;

	/** Thread cycles for this frame. */
	TMap<uint32, int64> ThreadCycles;
};

/**
 *	Helper class used to save the capture stats data via the background thread.
 *	CAUTION!! Can exist only one instance at the same time.
 */
class FAsyncWriteWorker : public FNonAbandonableTask
{
public:
	/** File To write to **/
	FArchive *File;
	/** Data for the file **/
	TArray<uint8> Data;
	/** Offset of the data, -1 means append at the end of file. */
	int64 DataOffset;

	/** Constructor. */
	FAsyncWriteWorker( FArchive *InFile, TArray<uint8>* InData, int64 InDataOffset = -1 )
		: File( InFile )
		, DataOffset( InDataOffset )
	{
		Exchange( Data, *InData );
	}

	/** Write the file  */
	void DoWork()
	{
		check( Data.Num() );
		if( DataOffset != -1 )
		{
			File->Seek( DataOffset );
		}
		else
		{
			// Seek to the end of the file.
			File->Seek( File->TotalSize() );
		}
		File->Serialize( Data.GetTypedData(), Data.Num() );
	}

	/** 
	* @return	the name to display in external event viewers
	*/
	static const TCHAR *Name()
	{
		return TEXT( "FAsyncStatsWriteWorker" );
	}
};

struct CORE_API FStatsWriteFile : public TSharedFromThis<FStatsWriteFile, ESPMode::ThreadSafe>
{
protected:
	/** Stats stream header. */
	FStatsStreamHeader Header;

	/** Set of names already sent **/
	TSet<int32> FNamesSent;

	/** Array of stats frames info already captured. */
	TArray<FStatsFrameInfo> FramesInfo;

	/** Data to write through the async task. **/
	TArray<uint8> OutData;

	/** Current position in the file, emulated to calculate the offsets. */
	int64 Filepos;

	/** Filename of the archive that we are writing to. */
	FString ArchiveFilename;

	/** Stats file archive. */
	FArchive* File;

	/** Async task used to offload saving the capture data. */
	FAsyncTask<FAsyncWriteWorker>* AsyncTask;

public:
	/** Constructor. **/
	FStatsWriteFile();

	/** Virtual destructor. */
	virtual ~FStatsWriteFile()
	{}

	/** Starts writing the stats data into the specified file. */
	void Start( FString const& InFilename );

	/** Stops writing the stats data. */
	void Stop();

	bool IsValid() const;

	const TArray<uint8>& GetOutData() const
	{
		return OutData;
	}

	void ResetData()
	{
		OutData.Reset();
	}

	/** Writes magic value, dummy header and initial metadata. */
	virtual void WriteHeader();

	/** Grabs a frame from the local FStatsThreadState and adds it to the output **/
	virtual void WriteFrame( int64 TargetFrame, bool bNeedFullMetadata = false );

protected:
	/** Finalizes writing to the file, called directly form the same thread. */
	void Finalize( FArchive* File );

	void NewFrame( int64 TargetFrame );
	void SendTask( int64 InDataOffset = -1 );

	void AddNewFrameDelegate();
	void RemoveNewFrameDelegate();

	/** Writes metadata messages into the stream. */
	void WriteMetadata( FArchive& Ar )
	{
		const FStatsThreadState& Stats = FStatsThreadState::GetLocalState();
		for( const auto& It : Stats.ShortNameToLongName )
		{
			const FStatMessage& StatMessage = It.Value;
			WriteMessage( Ar, StatMessage );
		}
	}

	/** Sends an FName, and the string it represents if we have not sent that string before. **/
	FORCEINLINE_STATS void WriteFName( FArchive& Ar, FStatNameAndInfo NameAndInfo )
	{
		FName RawName = NameAndInfo.GetRawName();
		bool bSendFName = !FNamesSent.Contains( RawName.GetIndex() );
		int32 Index = RawName.GetIndex();
		Ar << Index;
		int32 Number = NameAndInfo.GetRawNumber();
		if( bSendFName )
		{
			FNamesSent.Add( RawName.GetIndex() );
			Number |= EStatMetaFlags::SendingFName << (EStatMetaFlags::Shift + EStatAllFields::StartShift);
		}
		Ar << Number;
		if( bSendFName )
		{
			FString Name = RawName.ToString();
			Ar << Name;
		}
	}

	/** Write a stat message. **/
	FORCEINLINE_STATS void WriteMessage( FArchive& Ar, FStatMessage const& Item )
	{
		WriteFName( Ar, Item.NameAndInfo );
		switch( Item.NameAndInfo.GetField<EStatDataType>() )
		{
			case EStatDataType::ST_int64:
			{
				int64 Payload = Item.GetValue_int64();
				Ar << Payload;
			}
				break;
			case EStatDataType::ST_double:
			{
				double Payload = Item.GetValue_double();
				Ar << Payload;
			}
				break;
			case EStatDataType::ST_FName:
				WriteFName( Ar, FStatNameAndInfo( Item.GetValue_FName(), false ) );
				break;
		}
	}
};

/** Struct used to save unprocessed stats data as raw messages. */
struct FRawStatsWriteFile : public FStatsWriteFile
{
public:
	/** Virtual destructor. */
	virtual ~FRawStatsWriteFile()
	{}

protected:
	/** Writes magic value, dummy header and initial metadata. */
	virtual void WriteHeader() OVERRIDE;

	/** Grabs a frame of raw message from the local FStatsThreadState and adds it to the output. */
	virtual void WriteFrame( int64 TargetFrame, bool bNeedFullMetadata = false ) OVERRIDE;

	/** Write a stat packed into the specified archive. */
	void WriteStatPacket( FArchive& Ar, /*const*/ FStatPacket& StatPacket )
	{
		Ar << StatPacket.Frame;
		Ar << StatPacket.ThreadId;
		int32 MyThreadType = (int32)StatPacket.ThreadType;
		Ar << MyThreadType;

		Ar << StatPacket.bBrokenCallstacks;
		// We must handle stat messages in a different way.
		int32 NumMessages = StatPacket.StatMessages.Num();
		Ar << NumMessages;
		for( int32 MessageIndex = 0; MessageIndex < NumMessages; ++MessageIndex )
		{
			WriteMessage( Ar, StatPacket.StatMessages[MessageIndex] );
		}
	}
};

/*-----------------------------------------------------------------------------
	Stats file reading functionality
-----------------------------------------------------------------------------*/

/**
* Class for maintaining state of receiving a stream of stat messages
*/
struct CORE_API FStatsReadStream
{
public:
	/** Stats stream header. */
	FStatsStreamHeader Header;

	/** FNames have a different index on each machine, so we translate via this map. **/
	TMap<int32, int32> FNamesIndexMap;

	/** Array of stats frame info. */
	TArray<FStatsFrameInfo> FramesInfo;

	/** Reads a stats stream header, returns true if the header is valid and we can continue reading. */
	bool ReadHeader( FArchive& Ar )
	{
		bool bStatWithHeader = false;

		uint32 Magic = 0;
		Ar << Magic;
		if( Magic == EStatMagicNoHeader::MAGIC )
		{

		}
		else if( Magic == EStatMagicNoHeader::MAGIC_SWAPPED )
		{
			Ar.SetByteSwapping( true );
		}
		else if( Magic == EStatMagicWithHeader::MAGIC )
		{
			bStatWithHeader = true;
		}
		else if( Magic == EStatMagicWithHeader::MAGIC_SWAPPED )
		{
			bStatWithHeader = true;
			Ar.SetByteSwapping( true );
		}
		else
		{
			return false;
		}

		// We detected a header for a stats file, read it.
		if( bStatWithHeader )
		{
			Ar << Header;
		}

		return true;
	}

	/** Reads a stat packed from the specified archive. */
	void ReadStatPacket( FArchive& Ar, FStatPacket& StatPacked, bool bHasFNameMap )
	{
		Ar << StatPacked.Frame;
		Ar << StatPacked.ThreadId;
		int32 MyThreadType = 0;
		Ar << MyThreadType;
		StatPacked.ThreadType = (EThreadType::Type)MyThreadType;

		Ar << StatPacked.bBrokenCallstacks;
		// We must handle stat messages in a different way.
		int32 NumMessages = 0;
		Ar << NumMessages;
		StatPacked.StatMessages.Reserve( NumMessages );
		for( int32 MessageIndex = 0; MessageIndex < NumMessages; ++MessageIndex )
		{
			new(StatPacked.StatMessages) FStatMessage( ReadMessage( Ar, bHasFNameMap ) );
		}
	}

	/** Read and translate or create an FName. **/
	FORCEINLINE_STATS FStatNameAndInfo ReadFName( FArchive& Ar, bool bHasFNameMap )
	{
		// If we read the whole FNames translation map, we don't want to add the FName again.
		// This is a bit tricky, even if we have the FName translation map, we still need to read the FString.
		// CAUTION!! This is considered to be thread safe in this case.
		int32 Index = 0;
		Ar << Index;
		int32 Number = 0;
		Ar << Number;
		FName TheFName;
		
		if( !bHasFNameMap )
		{
			if( Number & (EStatMetaFlags::SendingFName << (EStatMetaFlags::Shift + EStatAllFields::StartShift)) )
			{
				FString Name;
				Ar << Name;

				TheFName = FName( *Name );
				FNamesIndexMap.Add( Index, TheFName.GetIndex() );
				Number &= ~(EStatMetaFlags::SendingFName << (EStatMetaFlags::Shift + EStatAllFields::StartShift));
			}
			else
			{
				if( FNamesIndexMap.Contains( Index ) )
				{
					int32 MyIndex = FNamesIndexMap.FindChecked( Index );
					TheFName = FName( EName( MyIndex ) );
				}
				else
				{
					TheFName = FName( TEXT( "Unknown FName" ) );
					Number = 0;
					UE_LOG( LogTemp, Warning, TEXT( "Missing FName Indexed: %d, %d" ), Index, Number );
				}
			}
		}
		else
		{
			if( Number & (EStatMetaFlags::SendingFName << (EStatMetaFlags::Shift + EStatAllFields::StartShift)) )
			{
				FString Name;
				Ar << Name;
				Number &= ~(EStatMetaFlags::SendingFName << (EStatMetaFlags::Shift + EStatAllFields::StartShift));
			}
			if( FNamesIndexMap.Contains( Index ) )
			{
				int32 MyIndex = FNamesIndexMap.FindChecked( Index );
				TheFName = FName( EName( MyIndex ) );
			}
			else
			{
				TheFName = FName( TEXT( "Unknown FName" ) );
				Number = 0;
				UE_LOG( LogTemp, Warning, TEXT( "Missing FName Indexed: %d, %d" ), Index, Number );
			}
		}

		FStatNameAndInfo Result( TheFName, false );
		Result.SetNumberDirect( Number );
		return Result;
	}

	/** Read a stat message. **/
	FORCEINLINE_STATS FStatMessage ReadMessage( FArchive& Ar, bool bHasFNameMap = false )
	{
		FStatMessage Result( ReadFName( Ar, bHasFNameMap ) );
		Result.Clear();
		switch( Result.NameAndInfo.GetField<EStatDataType>() )
		{
			case EStatDataType::ST_int64:
			{
				int64 Payload = 0;
				Ar << Payload;
				Result.GetValue_int64() = Payload;
			}
				break;
			case EStatDataType::ST_double:
			{
				double Payload = 0;
				Ar << Payload;
				Result.GetValue_double() = Payload;
			}
				break;
			case EStatDataType::ST_FName:
				FStatNameAndInfo Payload( ReadFName( Ar, bHasFNameMap ) );
				Result.GetValue_FName() = Payload.GetRawName();
				break;
		}
		return Result;
	}

	/**
	 *	Reads stats frames info from the specified archive, only valid for finalized stats files.
	 *	Allows unordered file access and whole data mini-view.
	 */
	void ReadFramesOffsets( FArchive& Ar )
	{
		Ar.Seek( Header.FrameTableOffset );
		Ar << FramesInfo;
	}

	/**
	 *	Reads FNames and metadata messages from the specified archive, only valid for finalized stats files.
	 *	Allow unordered file access.
	 */
	void ReadFNamesAndMetadataMessages( FArchive& Ar, TArray<FStatMessage>& out_MetadataMessages )
	{
		// Read FNames.
		Ar.Seek( Header.FNameTableOffset );
		out_MetadataMessages.Reserve( Header.NumFNames );
		for( int32 Index = 0; Index < Header.NumFNames; Index++ )
		{
			ReadFName( Ar, false );
		}

		// Read metadata messages.
		Ar.Seek( Header.MetadataMessagesOffset );
		out_MetadataMessages.Reserve( Header.NumMetadataMessages );
		for( int32 Index = 0; Index < Header.NumMetadataMessages; Index++ )
		{
			new(out_MetadataMessages)FStatMessage( ReadMessage( Ar, false ) );
		}
	}
};

/*-----------------------------------------------------------------------------
	Commands functionality
-----------------------------------------------------------------------------*/

/** Implements 'Stat Start/StopFile' functionality. */
struct FCommandStatsFile
{
	/** Filename of the last saved stats file. */
	static FString LastFileSaved;

	/** First frame. */
	static int64 FirstFrame;

	/** Stats file that is currently being saved. */
	static TSharedPtr<FStatsWriteFile, ESPMode::ThreadSafe> CurrentStatFile;

	/** Stat StartFile. */
	static void Start( const TCHAR* Cmd );

	/** Stat StartFileRaw. */
	static void StartRaw( const TCHAR* Cmd );

	/** Stat StopFile. */
	static void Stop();
};


#endif // STATS