// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "MessageLog.h"
#include "BlueprintPaletteFavorites.h"

#define LOCTEXT_NAMESPACE "EditorUserSettings"

UEditorUserSettings::UEditorUserSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Sadness
	DataPinStyle = BPST_VariantA;

	// Blueprint editor graph node pin type colors
	DefaultPinTypeColor = FLinearColor(0.750000f, 0.6f, 0.4f, 1.0f);
	ExecutionPinTypeColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	BooleanPinTypeColor = FLinearColor(0.300000f, 0.0f, 0.0f, 1.0f);
	BytePinTypeColor = FLinearColor(0.0f, 0.160000f, 0.131270f, 1.0f);
	ClassPinTypeColor = FLinearColor(0.1f, 0.0f, 0.5f, 1.0f);
	IntPinTypeColor = FLinearColor(0.013575f, 0.770000f, 0.429609f, 1.0f);
	FloatPinTypeColor = FLinearColor(0.357667f, 1.0f, 0.060000f, 1.0f);
	NamePinTypeColor = FLinearColor(0.607717f, 0.224984f, 1.0f, 1.0f);
	DelegatePinTypeColor = FLinearColor(1.0f, 0.04f, 0.04f, 1.0f);
	ObjectPinTypeColor = FLinearColor(0.0f, 0.4f, 0.910000f, 1.0f);
	StringPinTypeColor = FLinearColor(1.0f, 0.0f, 0.660537f, 1.0f);
	TextPinTypeColor = FLinearColor(0.8f, 0.2f, 0.4f, 1.0f);
	StructPinTypeColor = FLinearColor(0.0f, 0.1f, 0.6f, 1.0f);
	WildcardPinTypeColor = FLinearColor(0.220000f, 0.195800f, 0.195800f, 1.0f);
	VectorPinTypeColor = FLinearColor(1.0f, 0.591255f, 0.016512f, 1.0f);
	RotatorPinTypeColor = FLinearColor(0.353393f, 0.454175f, 1.0f, 1.0f);
	TransformPinTypeColor = FLinearColor(1.0f, 0.172585f, 0.0f, 1.0f);
	IndexPinTypeColor = FLinearColor(0.013575f, 0.770000f, 0.429609f, 1.0f);

	// Blueprint editor graph node title colors
	EventNodeTitleColor = FLinearColor(1.f, 0.0f, 0.0f, 1.0f);
	FunctionCallNodeTitleColor = FLinearColor(0.190525f, 0.583898f, 1.0f, 1.0f);
	PureFunctionCallNodeTitleColor = FLinearColor(0.4f, 0.850000f, 0.350000f, 1.0f);
	ParentFunctionCallNodeTitleColor = FLinearColor(1.0f, 0.170000f, 0.0f, 1.0f);
	FunctionTerminatorNodeTitleColor = FLinearColor(0.6f, 0.0f, 1.0f, 1.0f);
	ExecBranchNodeTitleColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	ExecSequenceNodeTitleColor= FLinearColor(0.8f, 0.4f, 0.4f, 1.0f);

	// Blueprint debugging visuals
	TraceAttackColor = FLinearColor(1.0f, 0.05f, 0.0f, 1.0f);
	TraceAttackWireThickness = 12.0f;
	TraceSustainColor = FLinearColor(1.0f, 0.7f, 0.4f, 1.0f);
	TraceSustainWireThickness = 8.0f;
	TraceReleaseColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	TraceReleaseWireThickness = 2.0f;

	// Blueprint debugging exec curve constants
	TracePositionBonusPeriod=0.5f;
	TracePositionExponent=5.0f;
	TraceAttackHoldPeriod=0.3f;
	TraceDecayPeriod=0.4f;
	TraceDecayExponent=1.8f;
	TraceSustainHoldPeriod=0.4f;
	TraceReleasePeriod=1.5f;
	TraceReleaseExponent=1.4f;

	//Default to high quality
	MaterialQualityLevel = 1;

	BlueprintFavorites = ConstructObject<UBlueprintPaletteFavorites>(UBlueprintPaletteFavorites::StaticClass(), this);

	bAutoApplyLightingEnable = true;
}

void UEditorUserSettings::PostInitProperties()
{
	Super::PostInitProperties();

	//Ensure the material quality cvar is set to the settings loaded.
	static IConsoleVariable* MaterialQualityLevelVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MaterialQualityLevel"));
	MaterialQualityLevelVar->Set(MaterialQualityLevel);
}

void UEditorUserSettings::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bUseCurvesForDistributions")))
	{
		extern CORE_API uint32 GDistributionType;
		//GDistributionType == 0 for curves
		GDistributionType = (bUseCurvesForDistributions) ? 0 : 1;
	}
	else if ( Name == FName(TEXT("bAllowTranslateRotateZWidget")) )
	{
		if ( bAllowTranslateRotateZWidget )
		{
			GEditorModeTools().SetWidgetMode( FWidget::WM_TranslateRotateZ );
		}
		else if ( GEditorModeTools().GetWidgetMode() == FWidget::WM_TranslateRotateZ )
		{
			GEditorModeTools().SetWidgetMode( FWidget::WM_Translate );
		}
	}
	else if ( Name == FName(TEXT("bNavigationAutoUpdate")) )
	{
		FWorldContext &EditorContext = GEditor->GetEditorWorldContext();
		UNavigationSystem::SetNavigationAutoUpdateEnabled(bNavigationAutoUpdate, EditorContext.World()->GetNavigationSystem() );
	}

	GEditor->SaveEditorUserSettings();

	UserSettingChangedEvent.Broadcast(Name);
}


#undef LOCTEXT_NAMESPACE
