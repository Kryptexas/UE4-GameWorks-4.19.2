// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageAttachment.h: Declares the IMessageAttachment interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageAttachment.
 */
typedef TSharedPtr<class IMessageAttachment, ESPMode::ThreadSafe> IMessageAttachmentPtr;

/**
 * Type definition for shared references to instances of IMessageAttachment.
 */
typedef TSharedRef<class IMessageAttachment, ESPMode::ThreadSafe> IMessageAttachmentRef;


/**
 * Interface for message attachments.
 */
class IMessageAttachment
{
public:

	/**
	 * Creates an archive reader to the data.
	 *
	 * The caller is responsible for deleting the returned object.
	 *
	 * @return An archive reader.
	 */
	virtual FArchive* CreateReader( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageAttachment( ) { }
};
