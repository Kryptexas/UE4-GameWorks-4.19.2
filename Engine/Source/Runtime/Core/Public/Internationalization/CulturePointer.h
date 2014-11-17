// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SharedPointer.h"

class FCulture;

typedef TSharedPtr<FCulture, ESPMode::ThreadSafe> FCulturePtr;
typedef TSharedRef<FCulture, ESPMode::ThreadSafe> FCultureRef;