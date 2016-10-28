// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Runtime/Core/Public/Core.h"
#include "Runtime/Core/Public/Misc/AutomationTest.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Runtime/CoreUObject/Public/CoreUObject.h"
#include "Runtime/Messaging/Public/Deprecated/Messaging.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Runtime/Serialization/Public/StructDeserializer.h"
#include "Runtime/Serialization/Public/StructSerializer.h"
#include "Runtime/Serialization/Public/Backends/JsonStructDeserializerBackend.h"
#include "Runtime/Serialization/Public/Backends/JsonStructSerializerBackend.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"

#if WITH_EDITOR
	#include "Developer/Settings/Public/ISettingsModule.h"
	#include "Developer/Settings/Public/ISettingsSection.h"
#endif


/** Declares a log category for this module. */
DECLARE_LOG_CATEGORY_EXTERN(LogUdpMessaging, Log, All);


/** Defines the default IP endpoint for multicast traffic. */
#define UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT FIPv4Endpoint(FIPv4Address(230, 0, 0, 1), 6666)

/** Defines the maximum number of annotations a message can have. */
#define UDP_MESSAGING_MAX_ANNOTATIONS 128

/** Defines the maximum number of recipients a message can have. */
#define UDP_MESSAGING_MAX_RECIPIENTS 1024

/** Defines the desired size of socket receive buffers (in bytes). */
#define UDP_MESSAGING_RECEIVE_BUFFER_SIZE 2 * 1024 * 1024

/** Defines the protocol version of the UDP message transport. */
#define UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION 10
