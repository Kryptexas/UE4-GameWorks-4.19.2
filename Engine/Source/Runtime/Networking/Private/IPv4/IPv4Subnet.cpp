// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IPv4Subnet.h: Implements the FIPv4Subnet class.
=============================================================================*/

#include "NetworkingPrivatePCH.h"


/* FIPv4Subnet interface
 *****************************************************************************/

FText FIPv4Subnet::ToText( ) const
{
	return FText::Format( FText::FromString( TEXT("{0}/{1}") ), Address.ToText(), Mask.ToText() );
}


/* FIPv4Subnet static interface
 *****************************************************************************/

bool FIPv4Subnet::Parse( const FString& SubnetString, FIPv4Subnet& OutSubnet )
{
	TArray<FString> Tokens;

	if (SubnetString.ParseIntoArray(&Tokens, TEXT("/"), false) == 2)
	{
		return (FIPv4Address::Parse(Tokens[0], OutSubnet.Address) && FIPv4SubnetMask::Parse(Tokens[1], OutSubnet.Mask));
	}

	return false;
}
