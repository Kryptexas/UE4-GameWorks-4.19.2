// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SocketSubsystem.h"
#include "Sockets.h"

/** These data types must match up on the other side! */
enum EDataType
{
	DT_TouchBegan=0,
	DT_TouchMoved=1,
	DT_TouchEnded=2,
	DT_Tilt=3,
	DT_Motion=4,
	DT_Gyro=5,
	DT_Ping=6,
};

/** Possible device orientations */
enum EDeviceOrientation
{
	DO_Unknown,
	DO_Portrait,
	DO_PortraitUpsideDown,
	DO_LandscapeLeft,
	DO_LandscapeRight,
	DO_FaceUp,
	DO_FaceDown,
};

// magic number that must match UDKRemote
#define MESSAGE_MAGIC_ID 0xAB

// versioning information for future expansion
#define CURRENT_MESSAGE_VERSION 1

/** A single event message to send over the network */
struct FMessage
{
	/** A byte that must match to what we expect */
	uint8 MagicTag;

	/** What version of message this is from UDK Remote */
	uint8 MessageVersion;

	/** Unique Id for the message, used for detecting lost/duplicate packets, etc (only duplicate currently handled) */
	uint16 MessageID;

	/** What type of message is this? */
	uint8 DataType;

	/** Unique identifier for the touch/finger */
	uint8 Handle;

	/** The current orientation of the device */
	uint8 DeviceOrientation;

	/** The current orientation of the UI */
	uint8 UIOrientation;

	/** X/Y or Pitch/Yaw data or CoreMotion data */
	float Data[12];
};


class FRemoteSlateServer
{
protected:
	/** The socket to listen on, will be initialized in first tick */
	FSocket* ServerSocket;

	/** The socket to send replies on, will be initialized in first tick */
	FSocket* ReplySocket;

	/** The socket to send image data on, will be initialized in first tick */
	FSocket* ImageSocket;

	/** The address of the most recent UDKRemote to talk to us, this is who we reply to */
	TSharedRef<FInternetAddr> ReplyAddr;

	/** Ever increasing timestamp to send to the input system */
	double Timestamp;

	/** Highest message ID (must handle wrapping around at 16 bits) */
	uint16 HighestMessageReceived;

	float TimeSinceLastPing;

//	/** Event used to */
//	FEvent* CompressionCompleteEvent;
//	/** An event used to signal 
//	FEvent* EncoderIsReadyEvent;

public:
	FRemoteSlateServer()
		: ServerSocket(NULL)
		, ReplySocket(NULL)
		, ReplyAddr(ISocketSubsystem::Get()->CreateInternetAddr())
		, Timestamp(0.0)
		, HighestMessageReceived(0xFFFF)
		, TimeSinceLastPing(200.0f)
	{
	}

	/**
	 * Pure virtual that must be overloaded by the inheriting class. It will
	 * be called from within UnLevTic.cpp after ticking all actors or from
	 * the rendering thread (depending on bIsRenderingThreadObject)
	 *
	 * @param DeltaTime	Game time passed since the last call.
	 */
	void Tick();

	FWeakWidgetPath GameViewportWidgetPath;
};
