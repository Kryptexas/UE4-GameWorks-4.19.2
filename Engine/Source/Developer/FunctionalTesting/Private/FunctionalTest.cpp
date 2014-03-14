// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"
#include "ObjectEditorUtils.h"

AFunctionalTest::AFunctionalTest( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, TimesUpResult(EFunctionalTestResult::Failed)
	, TimeLimit(DefaultTimeLimit)
	, TimesUpMessage( NSLOCTEXT("FunctionalTest", "DefaultTimesUpMessage", "Time's up!") )
	, bIsRunning(false)
	, TimeLeft(0.f)
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> Texture;

		FConstructorStatics()
			: Texture(TEXT("/Engine/EditorResources/S_FTest"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
#if WITH_EDITORONLY_DATA
	UTexture2D* SpriteTexture = ConstructorStatics.Texture.Get();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		if (SpriteTexture != NULL)
		{
			SpriteComponent = NewNamedObject<UBillboardComponent>(this, TEXT("Sprite"));

			if (SpriteComponent != NULL)
			{
				SpriteComponent->Sprite = SpriteTexture;
				SpriteComponent->bHiddenInGame = true;
				SpriteComponent->bVisible = true;
				SpriteComponent->AlwaysLoadOnClient = false;
				SpriteComponent->AlwaysLoadOnServer = false;

				//SpriteComponent->AttachParent = RootComponent;
				SpriteComponent->SetAbsolute(false, false, true);

				RootComponent = SpriteComponent;
			}
		}
	
		RenderComp = NewNamedObject<UFuncTestRenderingComponent>(this, TEXT("FTestRenderComp"));		
		RenderComp->PostPhysicsComponentTick.bCanEverTick = false;
		RenderComp->AttachParent = RootComponent;
	}
#endif // WITH_EDITORONLY_DATA
}

void AFunctionalTest::Tick(float DeltaSeconds)
{
	// already requested not to tick. 
	if (bIsRunning == false)
	{
		return;
	}

	TimeLeft -= DeltaSeconds;
	if (TimeLimit > 0.f && TimeLeft <= 0.f)
	{
		FinishTest(TimesUpResult, TimesUpMessage.ToString());
	}
	else
	{
		Super::Tick(DeltaSeconds);
	}
}

void AFunctionalTest::StartTest()
{
	if (TimeLimit > 0)
	{
		TimeLeft = TimeLimit;
		SetActorTickEnabled(true);
	}

	bIsRunning = true;
	
	OnTestStart.Broadcast();
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
		->AddToken( FTextToken::Create( ResultText ) )
		->AddToken( FTextToken::Create( Message.IsEmpty() == false ? FText::FromString(Message) : NSLOCTEXT("FunctionalTest", "FinishedTest", "Test finished") ) );

	TestFinishedObserver.ExecuteIfBound(this);
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

		InResult == EFunctionalTestResult::Failed;
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