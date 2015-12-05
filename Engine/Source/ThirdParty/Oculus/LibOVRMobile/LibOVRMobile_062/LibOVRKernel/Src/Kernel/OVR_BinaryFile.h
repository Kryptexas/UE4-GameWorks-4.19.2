/************************************************************************************

Filename    :   OVR_BinaryFile.h
Content     :   Simple helper class to read a binary file.
Created     :   Jun, 2014
Authors     :   J.M.P. van Waveren

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#ifndef OVR_BinaryFile_h
#define OVR_BinaryFile_h

#include "OVR_Types.h"
#include "OVR_Array.h"

/*
	This is a simple helper class to read binary data next to a JSON file.
*/

namespace OVR
{

class BinaryReader
{
public:
	BinaryReader( const uint8_t * binData, const int binSize ) :
		Data( binData ),
		Size( binSize ),
		Offset( 0 ),
		Allocated( false ) {}
	~BinaryReader();

	BinaryReader( const char * path, const char ** perror );

	uint32_t ReadUInt32() const
	{
		const int bytes = sizeof( unsigned int );
		if ( Data == NULL || bytes > Size - Offset )
		{
			return 0;
		}
		Offset += bytes;
		return *(uint32_t *)( Data + Offset - bytes );
	}

	template< typename _type_ >
	bool ReadArray( Array< _type_ > & out, const int numElements ) const
	{
		const int bytes = numElements * sizeof( out[0] );
		if ( Data == NULL || bytes > Size - Offset )
		{
			out.Resize( 0 );
			return false;
		}
		out.Resize( numElements );
		memcpy( &out[0], &Data[Offset], bytes );
		Offset += bytes;
		return true;
	}

	bool IsAtEnd() const
	{
		return ( Offset == Size );
	}

private:
	const uint8_t *	Data;
	int32_t			Size;
	mutable int32_t	Offset;
	bool			Allocated;
};

} // namespace OVR

#endif // OVR_BinaryFile_h
