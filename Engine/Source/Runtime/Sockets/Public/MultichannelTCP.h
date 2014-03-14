// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SocketSubsystem.h"
#include "Sockets.h"


SOCKETS_API DECLARE_LOG_CATEGORY_EXTERN(LogMultichannelTCP, Log, All);


/** Magic number used to verify packet header **/
enum
{
	MultichannelMagic= 0xa692339f
};


#include "NetworkMessage.h"
#include "MultiChannelTcpReceiver.h"
#include "MultiChannelTcpSender.h"
#include "MultichannelTcpSocket.h"