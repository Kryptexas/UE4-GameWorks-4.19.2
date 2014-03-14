// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageAddressBook.h: Declares the FMessageAddressBook class.
=============================================================================*/

#pragma once


/**
 * Implements an address book that maps message addresses to transport nodes.
 */
class FMessageAddressBook
{
public:

	/**
	 * Default constructor.
	 */
	FMessageAddressBook( )
	{
		CriticalSection = new FCriticalSection();
	}

	/**
	 * Destructor.
	 */
	~FMessageAddressBook( )
	{
		delete CriticalSection;
	}

public:

	/**
	 * Adds an address to the address book.
	 *
	 * @Address - The message address to add.
	 * @NodeId - The identifier of the transport node that handles the message address.
	 */
	void Add( const FMessageAddress& Address, const FGuid& NodeId )
	{
		FScopeLock Lock(CriticalSection);

		Entries.FindOrAdd(Address) = NodeId;
	}

	/**
	 * Clears the address book.
	 */
	void Clear( )
	{
		FScopeLock Lock(CriticalSection);

		Entries.Reset();
	}

	/**
	 * Checks whether this address book contains the given address.
	 *
	 * @param Address - The address to check.
	 *
	 * @return true if the address is known, false otherwise.
	 */
	bool Contains( const FMessageAddress& Address )
	{
		FScopeLock Lock(CriticalSection);

		return Entries.Contains(Address);
	}

	/**
	 * Gets the transport node identifiers for the specified list of message addresses.
	 *
	 * @param Addresses - The address list to retrieve the node identifiers for.
	 *
	 * @return The list of node identifiers.
	 */
	TArray<FGuid> GetNodesFor( const TArray<FMessageAddress>& Addresses )
	{
		TArray<FGuid> FoundNodes;

		FScopeLock Lock(CriticalSection);

		for (int32 AddressIndex = 0; AddressIndex < Addresses.Num(); ++AddressIndex)
		{
			FGuid* NodeId = Entries.Find(Addresses[AddressIndex]);

			if (NodeId != nullptr)
			{
				FoundNodes.AddUnique(*NodeId);
			}
		}

		return FoundNodes;
	}

	/**
	 * Removes all known message addresses.
	 *
	 * To remove only the addresses for a specific transport node, use RemoveNode().
	 * If you are not interested in the removed addresses, use Clear().
	 *
	 * @param OutRemovedRecipients - Will hold a list of recipients that were removed.
	 *
	 * @see Clear
	 * @see RemoveNode
	 */
	void RemoveAllNodes( TArray<FMessageAddress>& OutRemovedAddresses )
	{
		OutRemovedAddresses.Reset();

		FScopeLock Lock(CriticalSection);

		Entries.GenerateKeyArray(OutRemovedAddresses);
		Entries.Reset();
	}

	/**
	 * Removes all known message addresses for the specified transport node identifier.
	 *
	 * @param NodeId - The identifier of the transport node to remove.
	 * @param OutRemovedRecipients - Will hold a list of recipients that were removed.
	 *
	 * @see Clear
	 * @see RemoveAllNodes
	 */
	void RemoveNode( const FGuid& NodeId, TArray<FMessageAddress>& OutRemovedAddresses )
	{
		OutRemovedAddresses.Reset();

		FScopeLock Lock(CriticalSection);

		for (TMap<FMessageAddress, FGuid>::TConstIterator It(Entries); It; ++It)
		{
			if (It.Value() == NodeId)
			{
				OutRemovedAddresses.Add(It.Key());
			}
		}

		for (int32 Index = 0; Index < OutRemovedAddresses.Num(); ++Index)
		{
			Entries.Remove(OutRemovedAddresses[Index]);
		}
	}

private:

	// Holds a critical section to serialize access to the address book entries.
	FCriticalSection* CriticalSection;

	// Holds the collection of known addresses and their transport node identifiers.
	TMap<FMessageAddress, FGuid> Entries;
};
