// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"
#include "ObjectEditorUtils.h"
#include "VisualLog.h"

AFunctionalTest::AFunctionalTest( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, TimesUpResult(EFunctionalTestResult::Failed)
	, TimeLimit(DefaultTimeLimit)
	, TimesUpMessage( NSLOCTEXT("FunctionalTest", "DefaultTimesUpMessage", "Time's up!") )
	, bIsEnabled(true)
	, bIsRunning(false)
	, TotalTime(0.f)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SpriteComponent = PCIP.CreateDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->bHiddenInGame = false;
		SpriteComponent->AlwaysLoadOnClient = false;
		SpriteComponent->AlwaysLoadOnServer = false;
#if WITH_EDITORONLY_DATA

		if (!IsRunningCommandlet())
		{
			struct FConstructorStatics
			{
				ConstructorHelpers::FObjectFinderOptional<UTexture2D> Texture;
				FName ID_FTests;
				FText NAME_FTests;

				FConstructorStatics()
					: Texture(TEXT("/Engine/EditorResources/S_FTest"))
					, ID_FTests(TEXT("FTests"))
					, NAME_FTests(NSLOCTEXT( "SpriteCategory", "FTests", "FTests" ))
				{
				}
			};
			static FConstructorStatics ConstructorStatics;

			SpriteComponent->Sprite = ConstructorStatics.Texture.Get();
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_FTests;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_FTests;
		}

#endif
		RootComponent = SpriteComponent;
	}
}

void AFunctionalTest::Tick(float DeltaSeconds)
{
	// already requested not to tick. 
	if (bIsRunning == false)
	{
		return;
	}

	TotalTime += DeltaSeconds;
	if (TimeLimit > 0.f && TotalTime > TimeLimit)
	{
		FinishTest(TimesUpResult, TimesUpMessage.ToString());
	}
	else
	{
		Super::Tick(DeltaSeconds);
	}
}

bool AFunctionalTest::StartTest()
{
	FailureMessage = TEXT("");

	TotalTime = 0.f;
	if (TimeLimit > 0)
	{
		SetActorTickEnabled(true);
	}

	bIsRunning = true;
	
	OnTestStart.Broadcast();

	return true;
}

void AFunctionalTest::FinishTest(TEnumAsByte<EFunctionalTestResult::Type> TestResult, const FString& Message)
{
	const static UEnum* FTestResultTypeEnum = FindObject<UEnum>( NULL, TEXT("FunctionalTesting.FunctionalTest.EFunctionalTestResult") );
	
	bIsRunning = false;
	SetActorTickEnabled(false);

	OnTestFinished.Broadcast();

	AActor** ActorToDestroy = AutoDestroyActors.GetTypedData();

	for (int32 ActorIndex = 0; ActorIndex < AutoDestroyActors.Num(); ++ActorIndex, ++ActorToDestroy)
	{
		if (*ActorToDestroy != NULL)
		{
			// will be removed next frame
			(*ActorToDestroy)->SetLifeSpan( 0.01f );
		}
	}

	const FText ResultText = FTestResultTypeEnum->GetEnumText( TestResult.GetValue() );
	const FString OutMessage = FString::Printf(TEXT("%s> Result: %s> %s")
		, *GetActorLabel()
		, *ResultText.ToString()
		, Message.IsEmpty() == false ? *Message : TEXT("Test finished") );
	const FString AdditionalDetails = GetAdditionalTestFinishedMessage(TestResult) + FString::Printf(TEXT(", time %.2fs"), TotalTime);

	AutoDestroyActors.Reset();

	EMessageSeverity::Type MessageLogSeverity = EMessageSeverity::Info;
	
	switch (TestResult.GetValue())
	{
		case EFunctionalTestResult::Invalid:
		case EFunctionalTestResult::Error:
			UE_VLOG(this, LogFunctionalTest, Error, TEXT("%s"), *OutMessage);
			MessageLogSeverity = EMessageSeverity::Error;
			break;
		case EFunctionalTestResult::Running:
			UE_VLOG(this, LogFunctionalTest, Warning, TEXT("%s"), *OutMessage);
			MessageLogSeverity = EMessageSeverity::Warning;
			break;
		case EFunctionalTestResult::Failed:
			UE_VLOG(this, LogFunctionalTest, Error, TEXT("%s"), *OutMessage);
			MessageLogSeverity = EMessageSeverity::Error;
			break;
		default:
			UE_VLOG(this, LogFunctionalTest, Log, TEXT("%s"), *OutMessage);
			break;
	}

	FMessageLog("FunctionalTestingLog").Message(MessageLogSeverity, FText::FromString(GetActorLabel()))
		->AddToken(FTextToken::Create(ResultText))
		->AddToken(FTextToken::Create(FText::FromString(Message)))
		->AddToken(FTextToken::Create(FText::FromString(AdditionalDetails)))
		->AddToken(FTextToken::Create(FText::FromString(FailureMessage)));

	TestFinishedObserver.ExecuteIfBound(this);
}

void AFunctionalTest::CleanUp()
{
	FailureMessage = TEXT("");
}

//@todo add "warning" level here
void AFunctionalTest::LogMessage(const FString& Message)
{
	UE_VLOG(this, LogFunctionalTest, Warning
		, TEXT("%s> %s")
		, *GetActorLabel(), *Message);
}

void AFunctionalTest::SetTimeLimit(float InTimeLimit, TEnumAsByte<EFunctionalTestResult::Type> InResult)
{
	if (InTimeLimit < 0.f)
	{
		UE_VLOG(this, LogFunctionalTest, Warning
			, TEXT("%s> Trying to set TimeLimit to less than 0. Falling back to 0 (infinite).")
			, *GetActorLabel());

		InTimeLimit = 0.f;
	}
	TimeLimit = InTimeLimit;

	if (InResult == EFunctionalTestResult::Invalid)
	{
		UE_VLOG(this, LogFunctionalTest, Warning
			, TEXT("%s> Trying to set test Result to \'Invalid\'. Falling back to \'Failed\'")
			, *GetActorLabel());

		InResult = EFunctionalTestResult::Failed;
	}
	TimesUpResult = InResult;
}

void AFunctionalTest::RegisterAutoDestroyActor(AActor* ActorToAutoDestroy)
{
	AutoDestroyActors.AddUnique(ActorToAutoDestroy);
}

#if WITH_EDITOR

void AFunctionalTest::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_FunctionalTesting = FName(TEXT("FunctionalTesting"));
	static const FName NAME_TimeLimit = FName(TEXT("TimeLimit"));
	static const FName NAME_TimesUpResult = FName(TEXT("TimesUpResult"));

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != NULL)
	{
		if (FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property) == NAME_FunctionalTesting)
		{
			// first validate new data since there are some dependencies
			if (PropertyChangedEvent.Property->GetFName() == NAME_TimeLimit)
			{
				if (TimeLimit < 0.f)
				{
					TimeLimit = 0.f;
				}
			}
			else if (PropertyChangedEvent.Property->GetFName() == NAME_TimesUpResult)
			{
				if (TimesUpResult == EFunctionalTestResult::Invalid)
				{
					TimesUpResult = EFunctionalTestResult::Failed;
				}
			}
		}
	}
}

#endif // WITH_EDITOR
