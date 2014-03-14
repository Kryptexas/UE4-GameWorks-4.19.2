// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessagingClasses.cpp: Implements the module's script classes.
=============================================================================*/

#include "UdpMessagingPrivatePCH.h"
#include "UdpMessaging.generated.inl"


UUdpMessagingSettings::UUdpMessagingSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, EnableTransport(true)
	, MulticastTimeToLive(1)
	, EnableTunnel(false)
{ }
