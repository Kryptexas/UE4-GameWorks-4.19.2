// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Visibility.h"
#include "SlateBasics.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "WorkflowOrientedApp/SContentReference.h"
#include "SCheckBox.h"
#include "SNameComboBox.h"
#include "AssetThumbnail.h"
#include "SNiagaraEffectEditorViewport.h"



#define NGED_SECTION_BORDER SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder")).Padding(4.0f).HAlign(HAlign_Left)


class SEmitterWidget : public SCompoundWidget, public FNotifyHook
{
private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	UNiagaraEffect *Effect;
	TSharedPtr<STextBlock> UpdateComboText, SpawnComboText;
	TSharedPtr<SComboButton> UpdateCombo, SpawnCombo;
	TSharedPtr<SContentReference> UpdateScriptSelector, SpawnScriptSelector;
	TSharedPtr<SWidget> ThumbnailWidget;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<FAssetThumbnail> MaterialThumbnail;
	UNiagaraScript *CurUpdateScript, *CurSpawnScript;
	UMaterial *CurMaterial;

	TArray<TSharedPtr<FString> > RenderModuleList;

public:
	SLATE_BEGIN_ARGS(SEmitterWidget) :
		_Emitter(nullptr)
	{
	}
	SLATE_ARGUMENT(TSharedPtr<FNiagaraSimulation>, Emitter)
	SLATE_ARGUMENT(UNiagaraEffect*, Effect)
	SLATE_END_ARGS()

	void OnUpdateScriptSelectedFromPicker(UObject *Asset)
	{
		// Close the asset picker
		//AssetPickerAnchor->SetIsOpen(false);

		// Set the object found from the asset picker
		CurUpdateScript = Cast<UNiagaraScript>(Asset);
		Emitter->SetUpdateScript(CurUpdateScript);
	}

	void OnSpawnScriptSelectedFromPicker(UObject *Asset)
	{
		// Close the asset picker
		//AssetPickerAnchor->SetIsOpen(false);

		// Set the object found from the asset picker
		CurSpawnScript = Cast<UNiagaraScript>(Asset);
		Emitter->SetSpawnScript(CurSpawnScript);
	}

	void OnMaterialSelected(UObject *Asset)
	{
		MaterialThumbnail->SetAsset(Asset);
		MaterialThumbnail->RefreshThumbnail();
		MaterialThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
		ThumbnailPool->Tick(0);
		CurMaterial = Cast<UMaterial>(Asset);
		Emitter->GetEffectRenderer()->SetMaterial(CurMaterial, ERHIFeatureLevel::SM5);
	}

	UObject *GetMaterial() const
	{
		return CurMaterial;
	}

	UObject *GetUpdateScript() const
	{
		return CurUpdateScript;
	}

	UObject *GetSpawnScript() const
	{
		return CurSpawnScript;
	}

	void OnEmitterEnabledChanged(ESlateCheckBoxState::Type NewCheckedState)
	{
		const bool bNewEnabledState = (NewCheckedState == ESlateCheckBoxState::Checked);
		Emitter->SetEnabled(bNewEnabledState);
	}

	ESlateCheckBoxState::Type IsEmitterEnabled() const
	{
		return Emitter->IsEnabled() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	void OnRenderModuleChanged(TSharedPtr<FString> ModName, ESelectInfo::Type SelectInfo)
	{
		EEmitterRenderModuleType Type = RMT_Sprites;
		for (int32 i = 0; i < RenderModuleList.Num(); i++)
		{
			if (*RenderModuleList[i] == *ModName)
			{
				Type = (EEmitterRenderModuleType)(i+1);
				break;
			}
		}


		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FChangeNiagaraRenderModule,
			UNiagaraEffect*, InEffect, this->Effect,
			TSharedPtr<FNiagaraSimulation>, InEmitter, this->Emitter,
			EEmitterRenderModuleType, InType, Type,
		{
			InEmitter->SetRenderModuleType(InType, InEffect->GetComponent()->GetWorld()->FeatureLevel);
			InEffect->RenderModuleupdate();
		});
		TComponentReregisterContext<UNiagaraComponent> ComponentReregisterContext;
	}

	void OnSpawnRateChanged(const FText &InText)
	{
		Emitter->SetSpawnRate(FCString::Atof(*InText.ToString()) );
	}

	FText GetSpawnRateText() const
	{
		TCHAR Txt[32];
		FCString::Snprintf(Txt, 32, TEXT("%f"), Emitter->GetSpawnRate() );

		return FText::FromString(Txt);
	}

	FString GetRenderModuleText() const
	{
		if (Emitter->GetRenderModuleType() == RMT_None)
		{
			return FString("None");
		}

		return *RenderModuleList[Emitter->GetRenderModuleType()-1];
	}

	TSharedRef<SWidget>GenerateRenderModuleComboWidget(TSharedPtr<FString> WidgetString )
	{
		FString Txt = *WidgetString;
		return SNew(STextBlock).Text(Txt);
	}


	FString GetStatsText() const
	{
		TCHAR Txt[128];
		FCString::Snprintf(Txt, 128, TEXT("%i Particles"), Emitter->GetNumParticles());
		return FString(Txt);
	}

	void Construct(const FArguments& InArgs);




};




class SNiagaraEffectEditorWidget : public SCompoundWidget, public FNotifyHook
{
private:
	UNiagaraEffect *EffectObj;

	/** Notification list to pass messages to editor users  */
	TSharedPtr<SNotificationList> NotificationListPtr;
	SOverlay::FOverlaySlot* DetailsViewSlot;
	TSharedPtr<SNiagaraEffectEditorViewport>	Viewport;
	
	TSharedPtr< SListView<TSharedPtr<FNiagaraSimulation> > > ListView;

public:
	SLATE_BEGIN_ARGS(SNiagaraEffectEditorWidget)
		: _EffectObj(nullptr)
	{
	}
	SLATE_ARGUMENT(UNiagaraEffect*, EffectObj)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, TitleBar)
	SLATE_END_ARGS()


	/** Function to check whether we should show read-only text in the panel */
	EVisibility ReadOnlyVisibility() const
	{
		return EVisibility::Hidden;
	}

	void UpdateList()
	{
		ListView->RequestListRefresh();
	}

	TSharedPtr<SNiagaraEffectEditorViewport> GetViewport()	{ return Viewport; }

	TSharedRef<ITableRow> OnGenerateWidgetForList(TSharedPtr<FNiagaraSimulation> InItem, const TSharedRef< STableViewBase >& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
			.Content()
			[
				SNew(SBorder)
				//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(2.0f)
				[
					SNew(SEmitterWidget).Emitter(InItem).Effect(EffectObj)
				]
			];
	}



	void Construct(const FArguments& InArgs);

};

