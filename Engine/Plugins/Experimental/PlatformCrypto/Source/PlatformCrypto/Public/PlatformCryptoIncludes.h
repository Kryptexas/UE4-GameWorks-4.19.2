// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_XBOXONE
	#include "EncryptionContextBCrypt.h"
#else
	#include "EncryptionContextOpenSSL.h"
#endif
