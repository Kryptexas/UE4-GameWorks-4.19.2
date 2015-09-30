// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PacketHandler.h"
#include "ModuleManager.h"
#include "Core.h"

DECLARE_LOG_CATEGORY_EXTERN(OodleHandlerComponentLog, Log, All);


#if HAS_OODLE_SDK
#include "OodleArchives.h"

#include "oodle.h"


// Whether or not to utilize the Oodle-example-code based training and packet capture code (in the process of deprecation)
#define EXAMPLE_CAPTURE_FORMAT 0

#if EXAMPLE_CAPTURE_FORMAT
	#define CAPTURE_EXT TEXT(".bin")
#else
	#define CAPTURE_EXT TEXT(".ucap")
#endif


/** Globals */
extern FString GOodleSaveDir;


/**
 * The mode that the Oodle packet handler should operate in
 */
enum EOodleHandlerMode
{
	Training,	// Stores packet captures for the server
	Release		// Compresses packet data, based on the dictionary file
};


/**
 * Stores dictionary data from an individual file (typically one dictionary each, for server/client)
 *
 * @todo #JohnB: In its current form, this is temporary/stopgap, based on the example code
 */
struct FOodleDictionary
{
	/** Base constructor */
	FOodleDictionary()
		: Dictionary(NULL)
		, SharedDictionaryData(NULL)
		, CompressorState(NULL)
	{
	}

	/* Dictionary */
	void* Dictionary; 

	/* Shared Compressor */
	OodleNetwork1_Shared* SharedDictionaryData;

	/* State of Oodle */
	OodleNetwork1UDP_State* CompressorState;
};


/**
 * PacketHandler component for implementing Oodle support.
 *
 * Implementation uses trained/dictionary-based UDP compression.
 */
class OODLEHANDLERCOMPONENT_API OodleHandlerComponent : public HandlerComponent
{
public:
	/* Initializes default data */
	OodleHandlerComponent();

	/* Default Destructor */
	~OodleHandlerComponent();

	/**
	 * Initializes first-run config settings
	 */
	static void InitFirstRunConfig();


	/**
	 * @todo #JohnB
	 */
	void InitializeDictionary(FString FilePath, FOodleDictionary& OutDictionary);

	/* Initializes the handler component */
	virtual void Initialize() override;

	/* Whether the handler component is valid */
	virtual bool IsValid() const override;

	/* Handles any incoming packets */
	virtual void Incoming(FBitReader& Packet) override;

	/* Handles any outgoing packets */
	virtual void Outgoing(FBitWriter& Packet) override;

protected:
	/* How many bytes have been saved from compressing the data */
	uint32 BytesSaved;

#if EXAMPLE_CAPTURE_FORMAT
	/* File to log packets to */
	IFileHandle* PacketLogFile;

	/** Whether or not to append data to a statically named log file */
	bool bPacketLogAppend;
#else
	/* File to log input packets to */
	FPacketCaptureArchive* InPacketLog;

	/* File to log output packets to */
	FPacketCaptureArchive* OutPacketLog;
#endif


	/* 
	* Modes of the component
	* Specify this in .ini by
	* [OodleHandlerComponent]
	* Mode=Training
	*/
	EOodleHandlerMode Mode;


	/** Server (Outgoing) dictionary data */
	FOodleDictionary ServerDictionary;

	/** Client (Incoming - relative to server) dictionary data */
	FOodleDictionary ClientDictionary;
};

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Oodle Bytes Raw"), STAT_Oodle_BytesRaw, STATGROUP_Net, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Oodle Bytes Compressed"), STAT_Oodle_BytesCompressed, STATGROUP_Net, );
#endif

/* Reliability Module Interface */
class FOodleComponentModuleInterface : public FPacketHandlerComponentModuleInterface
{
private:
	/** Reference to the Oodle library handle */
	void* OodleDllHandle;

public:
	FOodleComponentModuleInterface()
		: OodleDllHandle(NULL)
	{
	}

	virtual TSharedPtr<HandlerComponent> CreateComponentInstance(FString& Options) override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
