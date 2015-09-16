// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PacketHandler.h"
#include "ModuleManager.h"
#include "Core.h"

#if HAS_OODLE_SDK == 1
#include "oodle.h"

DECLARE_LOG_CATEGORY_EXTERN(OodleHandlerComponentLog, Log, All);

// Symmetric Stream cipher
class OODLEHANDLERCOMPONENT_API OodleHandlerComponent : public HandlerComponent
{
public:
	/* Initializes default data */
	OodleHandlerComponent();

	/* Default Destructor */
	~OodleHandlerComponent();

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

	/* File to log packets to */
	IFileHandle* LogFile;

	/* 
	* Modes of the component
	* Specify this in .ini by
	* [OodleHandlerComponent]
	* Mode=Training
	*/
	enum {Training, Release}Mode;

	/* Dictionary */
	void* Dictionary; 

	/* Shared Compressor */
	OodleNetwork1_Shared* SharedDictionaryData;

	/* State of Oodle */
	OodleNetwork1UDP_State* CompressorState;
};

/* Reliability Module Interface */
class FOodleComponentModuleInterface : public FPacketHandlerComponentModuleInterface
{
public:
	virtual HandlerComponent* CreateComponentInstance() override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Oodle Bytes Raw"), STAT_Oodle_BytesRaw, STATGROUP_Net, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Oodle Bytes Compressed"), STAT_Oodle_BytesCompressed, STATGROUP_Net, );
#endif