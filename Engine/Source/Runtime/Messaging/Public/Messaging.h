// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Messaging.h: Messaging module public header file.
=============================================================================*/

#pragma once


/* Public Dependencies
 *****************************************************************************/

#include "CoreUObject.h"
#include "ModuleManager.h"
#include "TaskGraphInterfaces.h"


/* Public Interfaces
 *****************************************************************************/

#include "IMessageAttachment.h"
#include "IMessageData.h"
#include "IMessageContext.h"
#include "IInterceptMessages.h"
#include "IMessageHandler.h"
#include "IReceiveMessages.h"
#include "ISendMessages.h"
#include "IMessageSubscription.h"
#include "IAuthorizeMessageRecipients.h"
#include "IMessageTracerBreakpoint.h"
#include "IMessageTracer.h"
#include "IMessageBus.h"
#include "IMutableMessageContext.h"
#include "ISerializeMessages.h"
#include "ITransportMessages.h"
#include "IMessageBridge.h"
#include "IMessagingModule.h"


/* Public Common
 *****************************************************************************/

#include "FileMessageAttachment.h"
#include "MessageBridgeBuilder.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
