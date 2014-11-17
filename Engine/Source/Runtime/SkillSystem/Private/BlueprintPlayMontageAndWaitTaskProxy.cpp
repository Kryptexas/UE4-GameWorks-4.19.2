
#include "SkillSystemModulePrivatePCH.h"

UBlueprintPlayMontageAndWaitTaskProxy::UBlueprintPlayMontageAndWaitTaskProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UBlueprintPlayMontageAndWaitTaskProxy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
	}
	else
	{
		OnComplete.Broadcast();
	}
}