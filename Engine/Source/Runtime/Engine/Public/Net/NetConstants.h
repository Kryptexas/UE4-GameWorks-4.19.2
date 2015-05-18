// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Up to this many reliable channel bunches may be buffered.
enum { RELIABLE_BUFFER = 256 }; // Power of 2 >= 1.
enum { MAX_PACKETID = 16384 }; // Power of 2 >= 1, covering guaranteed loss/misorder time.
enum { MAX_CHSEQUENCE = 1024 }; // Power of 2 >RELIABLE_BUFFER, covering loss/misorder time.
enum { MAX_BUNCH_HEADER_BITS = 64 };
enum { MAX_PACKET_HEADER_BITS = 16 };
enum { MAX_PACKET_TRAILER_BITS = 1 };
enum { MAX_PACKET_SIZE = 1000 }; // MTU for the connection