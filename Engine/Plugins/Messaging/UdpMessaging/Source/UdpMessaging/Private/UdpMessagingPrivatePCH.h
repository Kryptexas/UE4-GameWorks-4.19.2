// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessagingPrivatePCH.h: Pre-compiled header file for the UdpMessaging module.
=============================================================================*/

#pragma once


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "Json.h"
#include "Messaging.h"
#include "ModuleManager.h"
#include "Settings.h"
#include "Sockets.h"
#include "SocketSubsystem.h"


/* Constants
 *****************************************************************************/

/**
 * Defines the default IP endpoint for multicast traffic.
 */
#define UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT FIPv4Endpoint(FIPv4Address(230, 0, 0, 1), 6666)

/**
 * Defines the protocol version of the UDP message transport.
 */
#define UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION 9


/* Private includes
 *****************************************************************************/

// shared
#include "UdpMessageSegment.h"
#include "UdpMessagingSettings.h"

// transport
#include "UdpMessageBeacon.h"
#include "ReassembledUdpMessage.h"
#include "UdpMessageResequencer.h"
#include "UdpMessageSegmenter.h"
#include "UdpMessageProcessor.h"
#include "UdpMessageTransport.h"

// tunnel
#include "UdpMessageTunnelConnection.h"
#include "UdpMessageTunnel.h"
