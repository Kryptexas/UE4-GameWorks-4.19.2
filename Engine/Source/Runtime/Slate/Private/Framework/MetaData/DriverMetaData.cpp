// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DriverMetaData.h"
#include "DriverIdMetaData.h"

TSharedRef<ISlateMetaData> FDriverMetaData::Id(FName InTag)
{
	return MakeShareable(new FDriverIdMetaData(MoveTemp(InTag)));
}
