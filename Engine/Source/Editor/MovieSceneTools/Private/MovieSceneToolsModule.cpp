// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ModuleManager.h"
#include "ISequencerModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "ScopedTransaction.h"
#include "MovieScene.h"
#include "MovieSceneTrackEditor.h"
#include "BoolPropertyTrackEditor.h"
#include "BytePropertyTrackEditor.h"
#include "ColorPropertyTrackEditor.h"
#include "FloatPropertyTrackEditor.h"
#include "VectorPropertyTrackEditor.h"
#include "VisibilityPropertyTrackEditor.h"
#include "TransformTrackEditor.h"
#include "ShotTrackEditor.h"
#include "SubMovieSceneTrackEditor.h"
#include "AudioTrackEditor.h"
#include "SkeletalAnimationTrackEditor.h"
#include "ParticleTrackEditor.h"
#include "AttachTrackEditor.h"
#include "PathTrackEditor.h"


/**
 * Implements the MovieSceneTools module.
 */
class FMovieSceneToolsModule
	: public IMovieSceneTools
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		// register with the sequencer module that we provide auto-key handlers
		ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );

		BoolPropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FBoolPropertyTrackEditor::CreateTrackEditor ) );
		BytePropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FBytePropertyTrackEditor::CreateTrackEditor ) );
		ColorPropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FColorPropertyTrackEditor::CreateTrackEditor ) );
		FloatPropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FFloatPropertyTrackEditor::CreateTrackEditor ) );
		VectorPropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FVectorPropertyTrackEditor::CreateTrackEditor ) );
		VisibilityPropertyTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FVisibilityPropertyTrackEditor::CreateTrackEditor ) );

		AnimationTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FSkeletalAnimationTrackEditor::CreateTrackEditor ) );
		AttachTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &F3DAttachTrackEditor::CreateTrackEditor ) );
		AudioTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FAudioTrackEditor::CreateTrackEditor ) );
		EditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FShotTrackEditor::CreateTrackEditor ) );
		ParticleTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FParticleTrackEditor::CreateTrackEditor ) );
		PathTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &F3DPathTrackEditor::CreateTrackEditor ) );
		SubMovieSceneTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &FSubMovieSceneTrackEditor::CreateTrackEditor ) );
		TransformTrackEditorCreateTrackEditorDelegateHandle = SequencerModule.RegisterTrackEditor_Handle( FOnCreateTrackEditor::CreateStatic( &F3DTransformTrackEditor::CreateTrackEditor ) );
	}

	virtual void ShutdownModule() override
	{
		if (!FModuleManager::Get().IsModuleLoaded("Sequencer"))
		{
			return;
		}

		// unregister auto key handlers
		ISequencerModule& SequencerModule = FModuleManager::Get().GetModuleChecked<ISequencerModule>( "Sequencer" );

		SequencerModule.UnRegisterTrackEditor_Handle( BoolPropertyTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( BytePropertyTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( ColorPropertyTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( FloatPropertyTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( VectorPropertyTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( VisibilityPropertyTrackEditorCreateTrackEditorDelegateHandle );

		SequencerModule.UnRegisterTrackEditor_Handle( AnimationTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( AttachTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( AudioTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( EditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( ParticleTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( PathTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( SubMovieSceneTrackEditorCreateTrackEditorDelegateHandle );
		SequencerModule.UnRegisterTrackEditor_Handle( TransformTrackEditorCreateTrackEditorDelegateHandle );
	}

private:

	/** Registered delegate handles */
	FDelegateHandle BoolPropertyTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle BytePropertyTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle ColorPropertyTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle FloatPropertyTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle VectorPropertyTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle VisibilityPropertyTrackEditorCreateTrackEditorDelegateHandle;

	FDelegateHandle AnimationTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle AttachTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle AudioTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle EditorCreateTrackEditorDelegateHandle;
	FDelegateHandle ParticleTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle PathTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle SubMovieSceneTrackEditorCreateTrackEditorDelegateHandle;
	FDelegateHandle TransformTrackEditorCreateTrackEditorDelegateHandle;
};


IMPLEMENT_MODULE( FMovieSceneToolsModule, MovieSceneTools );
