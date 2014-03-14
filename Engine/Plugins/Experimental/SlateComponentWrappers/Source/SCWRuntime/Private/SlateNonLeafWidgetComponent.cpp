// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#if WITH_EDITOR
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// USlateNonLeafWidgetComponent

USlateNonLeafWidgetComponent::USlateNonLeafWidgetComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;

	bAutoActivate = true;
	bTickInEditor = true;

	bCanHaveMultipleChildren = true;
}

void USlateNonLeafWidgetComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	bool bNeedRebuild = false;
	TSet<USlateWrapperComponent*> RemainingChildren;
	for (auto SetIt = KnownChildren.CreateIterator(); SetIt; ++SetIt)
	{
		if (USlateWrapperComponent* Wrapper = (*SetIt).Get())
		{
			if (Wrapper->IsRegistered())
			{
				RemainingChildren.Add(Wrapper);
			}
		}
	}

	for (int32 ComponentIndex = 0; (ComponentIndex < AttachChildren.Num()) && !bNeedRebuild; ++ComponentIndex)
	{
		if (USlateWrapperComponent* Wrapper = Cast<USlateWrapperComponent>(AttachChildren[ComponentIndex]))
		{
			if (Wrapper->IsRegistered())
			{
				if (KnownChildren.Contains(Wrapper))
				{
					RemainingChildren.Remove(Wrapper);
				}
				else
				{
					bNeedRebuild = true;
				}
			}
		}
	}

	bNeedRebuild = bNeedRebuild || (RemainingChildren.Num() > 0);

	if (bNeedRebuild)
	{
		// Recreate the list of known children
		KnownChildren.Empty();
		for (int32 ComponentIndex = 0; ComponentIndex < AttachChildren.Num(); ++ComponentIndex)
		{
			if (USlateWrapperComponent* Wrapper = Cast<USlateWrapperComponent>(AttachChildren[ComponentIndex]))
			{
				if (Wrapper->IsRegistered())
				{
					KnownChildren.Add(Wrapper);
				}
			}
		}

		// Let derived classes do something with the list
		OnKnownChildrenChanged();
	}
}

USlateWrapperComponent* USlateNonLeafWidgetComponent::GetFirstWrappedChild() const
{
	if (auto It = KnownChildren.CreateConstIterator())
	{
		return It->Get();
	}
	else
	{
		return NULL;
	}
}

TSharedRef<SWidget> USlateNonLeafWidgetComponent::GetFirstChildWidget() const
{
	if (USlateWrapperComponent* WrappedChild = GetFirstWrappedChild())
	{
		return WrappedChild->GetWidget();
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

#if WITH_EDITOR

void USlateNonLeafWidgetComponent::CheckForErrors()
{
	Super::CheckForErrors();

	if ((KnownChildren.Num() > 1) && !bCanHaveMultipleChildren)
	{
		AActor* Owner = GetOwner();
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ComponentName"), FText::FromString(GetName()));
		Arguments.Add(TEXT("OwnerName"), FText::FromString(Owner->GetName()));
		FText Message = FText::Format( LOCTEXT( "MapCheck_Message_TooManyChildren", "{ComponentName}::{OwnerName} cannot have more than one child attached" ), Arguments );
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(Owner))
			->AddToken(FTextToken::Create(Message) );
	}
}

#endif

bool USlateNonLeafWidgetComponent::CanAttachAsChild(USceneComponent* ChildComponent, FName SocketName) const
{
	const bool bIsSlateWidgetWrapper = ChildComponent->IsA(USlateWrapperComponent::StaticClass());
	return bIsSlateWidgetWrapper && (bCanHaveMultipleChildren ? true : (AttachChildren.Num() == 0));
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
