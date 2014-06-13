// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"


checkAtCompileTime( sizeof( FInt128 ) == 16, FInt128_sizeof_must_be_16 );
checkAtCompileTime( ALIGNOF( FInt128 ) == 16, FInt128_alignment_must_be_16 );
