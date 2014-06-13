// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Messaging.h"


/* Private dependencies
 *****************************************************************************/

#include "Json.h"
#include "Sockets.h"
#include "SocketSubsystem.h"


/* Private includes
 *****************************************************************************/

#include "MessageContext.h"
#include "MessageSubscription.h"
#include "MessageTracer.h"
#include "MessageDispatchTask.h"
#include "MessageRouter.h"
#include "MessageBus.h"

#include "BsonMessageSerializer.h"
#include "JsonMessageSerializer.h"
#include "XmlMessageSerializer.h"

#include "MessageAddressBook.h"
#include "MessageData.h"
#include "MessageDeserializeTask.h"
#include "MessageForwardTask.h"
#include "MessageSerializeTask.h"
#include "MessageBridge.h"
