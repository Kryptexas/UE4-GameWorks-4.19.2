// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AlembicLibraryPublicPCH.h"

#include "Logging/TokenizedMessage.h"

class FAbcImportLogger
{
protected:
	FAbcImportLogger();
public:
	/** Adds an import message to the stored array for later output*/
	static void AddImportMessage(const TSharedRef<FTokenizedMessage> Message);
	/** Outputs the messages to a message log*/
	ALEMBICLIBRARY_API static void OutputMessages();
private:
	/** Error messages **/
	static TArray<TSharedRef<FTokenizedMessage>> TokenizedErrorMessages;
	static FCriticalSection MessageLock;
};

