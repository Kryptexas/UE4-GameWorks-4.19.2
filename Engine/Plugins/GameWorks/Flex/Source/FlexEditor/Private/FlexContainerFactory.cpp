#include "FlexContainerFactory.h"
#include "FlexContainer.h"

/*------------------------------------------------------------------------------
UFlexContainerFactory
------------------------------------------------------------------------------*/
UFlexContainerFactory::UFlexContainerFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UFlexContainer::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UFlexContainerFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UFlexContainer>(InParent, Class, Name, Flags);
}
