// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FileMessageAttachment.h: Declares the FFileMessageAttachment class.
=============================================================================*/

#pragma once


/**
 * Implements a message attachment whose data is held in a file.
 */
class FFileMessageAttachment
	: public IMessageAttachment
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param Filename - The full name and path of the file.
	 */
	FFileMessageAttachment( const FString& InFilename )
		: AutoDeleteFile(false)
		, Filename(InFilename)
	{ }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param Filename - The full name and path of the file.
	 * @param AutoDeleteFile - Whether to delete the file when this attachment is destroyed.
	 */
	FFileMessageAttachment( const FString& InFilename, bool InAutoDeleteFile )
		: AutoDeleteFile(InAutoDeleteFile)
		, Filename(InFilename)
	{ }

	/**
	 * Destructor.
	 */
	~FFileMessageAttachment( )
	{
		if (AutoDeleteFile)
		{
			IFileManager::Get().Delete(*Filename);
		}
	}

public:

	// Begin IMessageAttachment interface

	virtual FArchive* CreateReader( ) OVERRIDE
	{
		return IFileManager::Get().CreateFileReader(*Filename);
	}

	// End IMessageAttachment interface

private:

	// Holds a flag indicating whether the file should be deleted.
	bool AutoDeleteFile;

	// Holds the name of the file that holds the attached data.
	FString Filename;
};
