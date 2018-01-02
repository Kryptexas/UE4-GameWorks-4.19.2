// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DDCNotifications.h"
#include "EngineBuildSettings.h"
#include "Logging/LogMacros.h"
#include "Misc/CoreMisc.h"
#include "Misc/App.h"
#include "Internationalization.h"
#include "SNotificationList.h"
#include "SHyperlink.h"
#include "NotificationManager.h"
#include "DerivedDataCacheInterface.h"
#include "Editor/EditorPerformanceSettings.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "DDCNotifications"

DEFINE_LOG_CATEGORY_STATIC(DDCNotifications, Log, All);

FDDCNotifications::FDDCNotifications() :
	bShowSharedDDCNotification(true)
{
	bool bSubscribe = false;

	// If preference is enabled and we're not using a shared DDC
	if (GetDefault<UEditorPerformanceSettings>()->bEnableSharedDDCPerformanceNotifications && !GetDerivedDataCacheRef().GetUsingSharedDDC())
	{
		// If we're a perforce build, or using the perforce source control provider and not an internal build
		bSubscribe = !FEngineBuildSettings::IsInternalBuild() &&
			(FEngineBuildSettings::IsPerforceBuild() || ISourceControlModule::Get().GetProvider().GetName() == FName(TEXT("Perforce")));
	}

	if (bSubscribe)
	{
		UE_LOG(DDCNotifications, Log, TEXT("Shared DDC not detected, performance notification enabled"));
		UpdateDDCSubscriptions(true);
	}
}

FDDCNotifications::~FDDCNotifications()
{
	UpdateDDCSubscriptions();
}

void FDDCNotifications::ClearSharedDDCNotification()
{
	// Don't call back into slate if already exiting
	if (GIsRequestingExit)
	{
		return;
	}

	if (SharedDDCNotification.IsValid())
	{
		SharedDDCNotification.Get()->SetCompletionState(SNotificationItem::CS_None);
		SharedDDCNotification.Get()->ExpireAndFadeout();
		SharedDDCNotification.Reset();
	}

}

void FDDCNotifications::OnDDCNotificationEvent(FDerivedDataCacheInterface::EDDCNotification DDCNotification)
{
	// Early out if we have turned off notifications
	if (!GetDefault<UEditorPerformanceSettings>()->bEnableSharedDDCPerformanceNotifications)
	{
		return;
	}

	if (!bShowSharedDDCNotification || DDCNotification != FDerivedDataCacheInterface::SharedDDCPerformanceNotification)
	{
		return;
	}

	// Only show DDC notifications once per session
	bShowSharedDDCNotification = false;

	// Unsubscribe
	UpdateDDCSubscriptions();

	FNotificationInfo Info(NSLOCTEXT("SharedDDCNotification", "SharedDDCNotificationMessage", "Shared Data Cache not in use, performance is impacted."));
	Info.bFireAndForget = false;
	Info.bUseThrobber = false;
	Info.FadeOutDuration = 0.0f;
	Info.ExpireDuration = 0.0f;

	Info.Hyperlink = FSimpleDelegate::CreateLambda([this]() {
		const FString DerivedDataCacheUrl = TEXT("https://docs.unrealengine.com/latest/INT/Engine/Basics/DerivedDataCache/");
		ClearSharedDDCNotification();
		FPlatformProcess::LaunchURL(*DerivedDataCacheUrl, nullptr, nullptr);
	});

	Info.HyperlinkText = LOCTEXT("SharedDDCNotificationHyperlink", "View Shared Data Cache Documentation");
		
	Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("SharedDDCNotificationDismiss", "Dismiss"), FText(), FSimpleDelegate::CreateLambda([this]() {
		ClearSharedDDCNotification();
	})));
			
	SharedDDCNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (SharedDDCNotification.IsValid())
	{
		SharedDDCNotification.Get()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FDDCNotifications::UpdateDDCSubscriptions(bool bSubscribe)
{
	
	FDerivedDataCacheInterface::FOnDDCNotification& DDCNotificationEvent = GetDerivedDataCacheRef().GetDDCNotificationEvent();

	if (bSubscribe)
	{
		DDCNotificationEvent.AddRaw(this, &FDDCNotifications::OnDDCNotificationEvent);		
	}
	else
	{
		DDCNotificationEvent.RemoveAll(this);
	}
}

#undef LOCTEXT_NAMESPACE