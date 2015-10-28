// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "MovieSceneCapture.h"
#include "MovieSceneCaptureModule.h"
#include "JsonObjectConverter.h"

#define LOCTEXT_NAMESPACE "MovieSceneCapture"

class FMovieSceneCaptureModule : public IMovieSceneCaptureModule, public FTickableGameObject, public FGCObject
{
public:

	void TickCapture(UMovieSceneCapture* Capture)
	{
		ExistingCaptures.Add(Capture);
	}

	void StopTickingCapture(UMovieSceneCapture* Capture)
	{
		ExistingCaptures.Remove(Capture);
	}

private:

	TArray<UMovieSceneCapture*> ExistingCaptures;

	/** Tickable interface */
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FCachePutAsyncWorker, STATGROUP_ThreadPoolAsyncTasks); }
	virtual void Tick( float DeltaTime ) override
	{
		for (auto* Obj : ExistingCaptures)
		{
			Obj->CaptureFrame(DeltaTime);
		}
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		for (auto* Obj : ExistingCaptures)
		{
			Collector.AddReferencedObject(Obj);
		}
	}

	/** End Tickable interface */
	virtual void StartupModule() override
	{
		FCoreDelegates::OnPreExit.AddRaw(this, &FMovieSceneCaptureModule::PreExit);
	}

	void PreExit()
	{
		TArray<UMovieSceneCapture*> ExistingCapturesCopy;
		Swap(ExistingCaptures, ExistingCapturesCopy);

		for (auto* Obj : ExistingCapturesCopy)
		{
			Obj->Close();
		}
	}

	virtual void PreUnloadCallback() override
	{
		FCoreDelegates::OnPreExit.RemoveAll(this);
		PreExit();
	}

	virtual IMovieSceneCaptureInterface* InitializeFromCommandLine() override
	{
		if (GIsEditor)
		{
			return nullptr;
		}

		FString ManifestPath;
		if (!FParse::Value(FCommandLine::Get(), TEXT("-MovieSceneCaptureManifest="), ManifestPath) || ManifestPath.IsEmpty())
		{
			return nullptr;
		}

		UMovieSceneCapture* Capture = nullptr;
		FString Json;
		if (FFileHelper::LoadFileToString(Json, *ManifestPath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(Json);
			if (FJsonSerializer::Deserialize(JsonReader, RootObject) && RootObject.IsValid())
			{
				auto TypeField = RootObject->TryGetField(TEXT("Type"));
				if (!TypeField.IsValid())
				{
					return nullptr;
				}

				UClass* Class = FindObject<UClass>(nullptr, *TypeField->AsString());
				if (!Class)
				{
					return nullptr;
				}

				Capture = NewObject<UMovieSceneCapture>(GetTransientPackage(), Class);
				if (!Capture)
				{
					return nullptr;
				}

				auto DataField = RootObject->TryGetField(TEXT("Data"));
				if (!DataField.IsValid())
				{
					return nullptr;
				}

				if (!FJsonObjectConverter::JsonAttributesToUStruct(DataField->AsObject()->Values, Class, Capture, 0, 0))
				{
					return nullptr;
				}

				TickCapture(Capture);
				return Capture;
			}
		}

		return nullptr;
	}

	virtual IMovieSceneCaptureInterface* CreateMovieSceneCapture(TWeakPtr<FSceneViewport> InSceneViewport) override
	{
		UMovieSceneCapture* Capture = NewObject<UMovieSceneCapture>(GetTransientPackage());
		TickCapture(Capture);
		Capture->Initialize(InSceneViewport);
		Capture->StartCapture();
		return Capture;
	}

	virtual IMovieSceneCaptureInterface* RetrieveMovieSceneInterface(FMovieSceneCaptureHandle Handle)
	{
		auto** Existing = ExistingCaptures.FindByPredicate([&](const UMovieSceneCapture* In){ return In->GetHandle() == Handle; });
		return Existing ? *Existing : nullptr;
	}

	IMovieSceneCaptureInterface* GetFirstActiveMovieSceneCapture()
	{
		return ExistingCaptures.Num() ? ExistingCaptures[0] : nullptr;
	}

	virtual void OnMovieSceneCaptureFinished(IMovieSceneCaptureInterface* Capture)
	{
		StopTickingCapture(static_cast<UMovieSceneCapture*>(Capture));
	}

	virtual void DestroyMovieSceneCapture(FMovieSceneCaptureHandle Handle)
	{
		// Calling Close() can remove itself from the array
		auto Captures = ExistingCaptures;
		for (auto* Existing : Captures)
		{
			if (Existing->GetHandle() == Handle)
			{
				Existing->Close();
			}
		}
	}
};

IMPLEMENT_MODULE( FMovieSceneCaptureModule, MovieSceneCapture )

#undef LOCTEXT_NAMESPACE