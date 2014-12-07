// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Niagara/NiagaraConstantSet.h"

template<>
struct TStructOpsTypeTraits<FNiagaraConstantMap> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithSerializer = true
	};
};
IMPLEMENT_STRUCT(NiagaraConstantMap);
