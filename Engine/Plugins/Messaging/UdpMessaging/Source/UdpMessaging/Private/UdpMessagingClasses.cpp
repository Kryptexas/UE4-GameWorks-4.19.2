// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UdpMessagingPrivatePCH.h"


UUdpMessagingSettings::UUdpMessagingSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, EnableTransport(true)
	, MulticastTimeToLive(1)
	, EnableTunnel(false)
{ }
