#include "FlexFluidSurfaceFactory.h"
#include "FlexFluidSurface.h"

/*------------------------------------------------------------------------------
UFlexFluidSurfaceFactory
------------------------------------------------------------------------------*/
UFlexFluidSurfaceFactory::UFlexFluidSurfaceFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	SupportedClass = UFlexFluidSurface::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UFlexFluidSurfaceFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UFlexFluidSurface>(InParent, Class, Name, Flags);
}
