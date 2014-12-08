// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Visibility.h"
#include "SlateBasics.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "WorkflowOrientedApp/SContentReference.h"
#include "SCheckBox.h"
#include "SNameComboBox.h"
#include "AssetThumbnail.h"
#include "SNiagaraEffectEditorViewport.h"
#include "Components/NiagaraComponent.h"
#include "Engine/NiagaraEffect.h"
#include "Engine/NiagaraSimulation.h"
#include "Engine/NiagaraEffectRenderer.h"
#include "ComponentReregisterContext.h"

#define NGED_SECTION_BORDER SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder")).Padding(4.0f).HAlign(HAlign_Left)
#define NGED_SECTION_LIGHTBORDER SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.LightGroupBorder")).Padding(4.0f).HAlign(HAlign_Left)
#define NGED_SECTION_DARKBORDER SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder")).Padding(4.0f).HAlign(HAlign_Left)


class SVectorConstantWidget : public SCompoundWidget, public FNotifyHook
{
private:	
	FName ConstantName;
	TSharedPtr<FNiagaraSimulation> Emitter;

	TSharedPtr<SEditableTextBox> XText, YText, ZText, WText;

public:
	SLATE_BEGIN_ARGS(SVectorConstantWidget) :
	_Emitter(nullptr),
	_ConstantName("Undefined")
	{
	}
	SLATE_ARGUMENT(TSharedPtr<FNiagaraSimulation>, Emitter)
	SLATE_ARGUMENT(FName, ConstantName)
	SLATE_END_ARGS()


	void OnConstantChanged(const FText &InText)
	{
		FVector4 Vec;
		FString Strns[4];
		Strns[0] = XText->GetText().ToString();
		Strns[1] = YText->GetText().ToString();
		Strns[2] = ZText->GetText().ToString();
		Strns[3] = WText->GetText().ToString();
		for (uint32 i = 0; i < 4; i++)
		{
			if (Strns[i]!="")
			{
				Vec[i] = FCString::Atof(*Strns[i]);
			}
			else
			{
				Vec[i] = 0.0f;
			}
		}
		Emitter->GetProperties()->ExternalConstants.SetOrAdd(ConstantName, Vec);
	}

	void Construct(const FArguments& InArgs)
	{
		Emitter = InArgs._Emitter;
		ConstantName = InArgs._ConstantName;

		FVector4 *ConstVal = Emitter->GetProperties()->ExternalConstants.FindVector(ConstantName);
		FString ConstText[4] = {"","","",""};
		if (ConstVal)
		{
			for (uint32 i = 0; i < 4; i++)
			{
				ConstText[i] = FString::Printf(TEXT("%.4f"), (*ConstVal)[i]);
			}
		}
		
		this->ChildSlot
		[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().Padding(4)[SNew(STextBlock).Text(FText::FromName(ConstantName))]
		+ SHorizontalBox::Slot().Padding(4)[SAssignNew(XText, SEditableTextBox).Text(FText::FromString(ConstText[0])).OnTextChanged(this, &SVectorConstantWidget::OnConstantChanged)]
		+ SHorizontalBox::Slot().Padding(4)[SAssignNew(YText, SEditableTextBox).Text(FText::FromString(ConstText[1])).OnTextChanged(this, &SVectorConstantWidget::OnConstantChanged)]
		+ SHorizontalBox::Slot().Padding(4)[SAssignNew(ZText, SEditableTextBox).Text(FText::FromString(ConstText[2])).OnTextChanged(this, &SVectorConstantWidget::OnConstantChanged)]
		+ SHorizontalBox::Slot().Padding(4)[SAssignNew(WText, SEditableTextBox).Text(FText::FromString(ConstText[3])).OnTextChanged(this, &SVectorConstantWidget::OnConstantChanged)]
		];
		
	}
};


class SEmitterWidget : public SCompoundWidget, public FNotifyHook
{
private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	FNiagaraEffectInstance *EffectInstance;
	TSharedPtr<STextBlock> UpdateComboText, SpawnComboText;
	TSharedPtr<SComboButton> UpdateCombo, SpawnCombo;
	TSharedPtr<SContentReference> UpdateScriptSelector, SpawnScriptSelector;
	TSharedPtr<SWidget> ThumbnailWidget;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<FAssetThumbnail> MaterialThumbnail;
	UNiagaraScript *CurUpdateScript, *CurSpawnScript;
	UMaterial *CurMaterial;

	TArray<TSharedPtr<FString> > RenderModuleList;

	TSharedPtr< SListView<TSharedPtr<EditorExposedVectorConstant>>> UpdateScriptConstantList;
	SVerticalBox::FSlot *UpdateScriptConstantListSlot;
	TSharedPtr< SListView<TSharedPtr<EditorExposedVectorConstant>>> SpawnScriptConstantList;
	SVerticalBox::FSlot *SpawnScriptConstantListSlot;

public:
	SLATE_BEGIN_ARGS(SEmitterWidget) :
		_Emitter(nullptr)
	{
	}
	SLATE_ARGUMENT(TSharedPtr<FNiagaraSimulation>, Emitter)
	SLATE_ARGUMENT(FNiagaraEffectInstance*, Effect)
	SLATE_END_ARGS()

	TSharedRef<ITableRow> OnGenerateConstantListRow(TSharedPtr<EditorExposedVectorConstant> InItem, const TSharedRef< STableViewBase >& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Content()
		[
			SNew(SVectorConstantWidget).ConstantName(InItem->ConstName).Emitter(Emitter)
		];
	}


	void OnUpdateScriptSelectedFromPicker(UObject *Asset)
	{
		// Close the asset picker
		//AssetPickerAnchor->SetIsOpen(false);

		// Set the object found from the asset picker
		CurUpdateScript = Cast<UNiagaraScript>(Asset);
		Emitter->GetProperties()->UpdateScript = CurUpdateScript;

		UpdateScriptConstantListSlot->DetachWidget();
		(*UpdateScriptConstantListSlot)
			[
				SAssignNew(UpdateScriptConstantList, SListView<TSharedPtr<EditorExposedVectorConstant>>)
				.ItemHeight(20).ListItemsSource(&CurUpdateScript->Source->ExposedVectorConstants).OnGenerateRow(this, &SEmitterWidget::OnGenerateConstantListRow)
			];
	}

	void OnSpawnScriptSelectedFromPicker(UObject *Asset)
	{
		// Close the asset picker
		//AssetPickerAnchor->SetIsOpen(false);

		// Set the object found from the asset picker
		CurSpawnScript = Cast<UNiagaraScript>(Asset);
		Emitter->GetProperties()->SpawnScript = CurSpawnScript;
	}

	void OnMaterialSelected(UObject *Asset)
	{
		MaterialThumbnail->SetAsset(Asset);
		MaterialThumbnail->RefreshThumbnail();
		MaterialThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
		ThumbnailPool->Tick(0);
		CurMaterial = Cast<UMaterial>(Asset);
		Emitter->GetEffectRenderer()->SetMaterial(CurMaterial, ERHIFeatureLevel::SM5);
		Emitter->GetProperties()->Material = CurMaterial;
	}

	UObject *GetMaterial() const
	{
		return Emitter->GetProperties()->Material;
	}

	UObject *GetUpdateScript() const
	{
		return Emitter->GetProperties()->UpdateScript;
	}

	UObject *GetSpawnScript() const
	{
		return Emitter->GetProperties()->SpawnScript;
	}

	void OnEmitterEnabledChanged(ESlateCheckBoxState::Type NewCheckedState)
	{
		const bool bNewEnabledState = (NewCheckedState == ESlateCheckBoxState::Checked);
		Emitter->GetProperties()->bIsEnabled = bNewEnabledState;
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
			FNiagaraEffectInstance*, InEffect, this->EffectInstance,
			TSharedPtr<FNiagaraSimulation>, InEmitter, this->Emitter,
			EEmitterRenderModuleType, InType, Type,
		{
			InEmitter->SetRenderModuleType(InType, InEffect->GetComponent()->GetWorld()->FeatureLevel);
			InEmitter->GetProperties()->RenderModuleType = InType;
			InEffect->RenderModuleupdate();
		});
		TComponentReregisterContext<UNiagaraComponent> ComponentReregisterContext;
	}

	void OnSpawnRateChanged(const FText &InText)
	{
		float Rate = FCString::Atof(*InText.ToString());
		Emitter->GetProperties()->SpawnRate = Rate;
	}

	FText GetSpawnRateText() const
	{
		TCHAR Txt[32];
		//FCString::Snprintf(Txt, 32, TEXT("%f"), Emitter->GetSpawnRate() );
		FCString::Snprintf(Txt, 32, TEXT("%f"), Emitter->GetProperties()->SpawnRate );

		return FText::FromString(Txt);
	}

	FString GetRenderModuleText() const
	{
		if (Emitter->GetProperties()->RenderModuleType == RMT_None)
		{
			return FString("None");
		}

		return *RenderModuleList[Emitter->GetProperties()->RenderModuleType-1];
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
	FNiagaraEffectInstance *EffectInstance;

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
	SLATE_ARGUMENT(FNiagaraEffectInstance*, EffectInstance)
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
					SNew(SEmitterWidget).Emitter(InItem).Effect(EffectInstance)
				]
			];
	}



	void Construct(const FArguments& InArgs);

};

