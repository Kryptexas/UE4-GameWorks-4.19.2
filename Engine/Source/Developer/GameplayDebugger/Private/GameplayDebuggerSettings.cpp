// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "Misc/CoreMisc.h"
#include "GameplayDebuggingComponent.h"
#include "GameplayDebuggingHUDComponent.h"
#include "GameplayDebuggingReplicator.h"
#include "GameplayDebuggingControllerComponent.h"
#include "AISystem.h"
#include "GameplayDebuggerSettings.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "ISettingsModule.h"
#endif // WITH_EDITOR

UGameplayDebuggerSettings::UGameplayDebuggerSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UGameplayDebuggerSettings::PostInitProperties()
{
	Super::PostInitProperties();

#define UPDATE_GAMEVIEW_DISPLAYNAME(__GameView__) \
	{\
		UProperty* Prop = FindField<UProperty>(UGameplayDebuggerSettings::StaticClass(), TEXT(#__GameView__));\
		if (Prop)\
		{\
			Prop->SetPropertyFlags(CPF_Edit);\
			Prop->SetMetaData(TEXT("DisplayName"), CustomViewNames.__GameView__.Len() > 0 ? *CustomViewNames.__GameView__ : TEXT(#__GameView__));\
		}\
	}
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView1);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView2);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView3);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView4);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView5);
#undef UPDATE_GAMEVIEW_DISPLAYNAME
}

void UGameplayDebuggerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

#define UPDATE_GAMEVIEW_DISPLAYNAME(__GameView__) \
	if (MemberPropertyName == (TEXT("CustomViewNames"))) \
	{\
		if (Name == FName(TEXT(#__GameView__)) ) \
		{ \
			UProperty* Prop = FindField<UProperty>(UGameplayDebuggerSettings::StaticClass(), *Name.ToString());\
			check(Prop);\
			Prop->SetPropertyFlags(CPF_Edit); \
			Prop->SetMetaData(TEXT("DisplayName"), CustomViewNames.__GameView__.Len() > 0 ? *CustomViewNames.__GameView__ : *Name.ToString());\
		}\
	}

	UPDATE_GAMEVIEW_DISPLAYNAME(GameView1);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView2);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView3);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView4);
	UPDATE_GAMEVIEW_DISPLAYNAME(GameView5);
#undef UPDATE_GAMEVIEW_DISPLAYNAME

	GEditor->SaveEditorUserSettings();
}
#endif
