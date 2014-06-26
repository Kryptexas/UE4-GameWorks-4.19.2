
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitAttributeChange.h"

#include "AbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayEffectExtension.h"

UAbilityTask_WaitAttributeChange::UAbilityTask_WaitAttributeChange(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAbilityTask_WaitAttributeChange* UAbilityTask_WaitAttributeChange::WaitForAttributeChange(UObject* WorldContextObject, FGameplayAttribute InAttribute, FGameplayTag InWithTag, FGameplayTag InWithoutTag)
{
	check(WorldContextObject);
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	if (ThisAbility)
	{
		UAbilityTask_WaitAttributeChange * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_WaitAttributeChange>();
		MyObj->InitTask(ThisAbility);
		MyObj->WithTag = InWithTag;
		MyObj->WithoutTag = InWithoutTag;
		MyObj->Attribute = InAttribute;

		return MyObj;
	}
	return NULL;
}

void UAbilityTask_WaitAttributeChange::Activate()
{
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RegisterGameplayAttributeEvent(Attribute).AddUObject(this, &UAbilityTask_WaitAttributeChange::OnAttributeChange);
	}
}

void UAbilityTask_WaitAttributeChange::OnAttributeChange(float NewValue, const FGameplayEffectModCallbackData* Data)
{
	if (Data == nullptr)
	{
		// There may be no execution data associated with this change, for example a GE being removed. 
		// In this case, we auto fail any WithTag requirement and auto pass any WithoutTag requirement
		if (WithTag.IsValid())
		{
			return;
		}
	}
	else
	{
		if ((WithTag.IsValid() && !Data->EvaluatedData.Tags.HasTag(WithTag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit)) ||
			(WithoutTag.IsValid() && Data->EvaluatedData.Tags.HasTag(WithoutTag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit)))
		{
			// Failed tag check
			return;
		}
	}
	
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RegisterGameplayAttributeEvent(Attribute).RemoveUObject(this, &UAbilityTask_WaitAttributeChange::OnAttributeChange);
	}

	OnChange.Broadcast();
}