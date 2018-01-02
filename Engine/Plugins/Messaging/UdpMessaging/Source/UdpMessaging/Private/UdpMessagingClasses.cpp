// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Shared/UdpMessagingSettings.h"


UUdpMessagingSettings::UUdpMessagingSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EnableTransport(true)
	, MulticastTimeToLive(1)
	, EnableTunnel(false)
{ }
