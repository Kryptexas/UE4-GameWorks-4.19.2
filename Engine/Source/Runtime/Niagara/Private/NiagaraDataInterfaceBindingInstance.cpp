// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceBindingInstance.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"

FNiagaraDataInterfaceBindingInstance::FNiagaraDataInterfaceBindingInstance(const FNiagaraScriptDataInterfaceInfo& InSource, FNiagaraScriptDataInterfaceInfo& InDestination, ENiagaraScriptUsage InUsage, bool bInDirectPointerCopy)
	: Source(InSource)
	, Destination(InDestination)
	, Usage(InUsage)
	, bDirectPointerCopy(bInDirectPointerCopy)
{
}

bool FNiagaraDataInterfaceBindingInstance::Tick()
{
	if (bDirectPointerCopy && Destination.DataInterface != Source.DataInterface)
	{
		Destination.DataInterface = Source.DataInterface;
		return true;
	}
	else if (!bDirectPointerCopy && Destination.DataInterface != nullptr && Source.DataInterface != nullptr && false == Source.DataInterface->Equals(Destination.DataInterface))
	{
		UE_LOG(LogNiagara, Warning, TEXT("Copying %s to %s, Type %d"), *Source.Name.ToString(), *Destination.Name.ToString(), (int32)Usage);
		Source.DataInterface->CopyTo(Destination.DataInterface);
		return true;
	}
	return false;
}