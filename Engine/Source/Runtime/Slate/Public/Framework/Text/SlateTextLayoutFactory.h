// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Framework/Text/SlateTextLayout.h"

DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<FSlateTextLayout>, FCreateSlateTextLayout, FTextBlockStyle);
