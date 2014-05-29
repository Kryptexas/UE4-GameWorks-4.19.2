// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetHost

UWidgetHost::UWidgetHost(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
//	PrimaryComponentTick.bCanEverTick = true;

	bIsVariable = true;

	//bAutoActivate = true;
	//bTickInEditor = true;
}

int32 UWidgetHost::GetChildrenCount() const
{
	return Widget != NULL ? 1 : 0;
}

USlateWrapperComponent* UWidgetHost::GetChildAt(int32 Index) const
{
	if ( Widget )
	{
		return Widget->GetRootWidgetComponent();
	}
	
	return NULL;
}

TSharedRef<SWidget> UWidgetHost::RebuildWidget()
{
	if ( Widget )
	{
		//Widget = GetWorld()->SpawnActor<UUserWidget>(WidgetClass);
		return Widget->GetRootWidget();
	}

	return SNew(SSpacer);
}

#if WITH_EDITOR
void UWidgetHost::ConnectEditorData()
{

}

void UWidgetHost::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{

}
#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
