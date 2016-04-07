// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "UnrealEd.h"
#include "STreeView.h"
#include "SListView.h"
#include "STableViewBase.h"
#include "SlateBasics.h"

#include "PixelInspectorResult.h"
#include "PixelInspectorView.h"

class IDetailsView;

namespace PixelInspector
{
#define WAIT_FRAMENUMBER_BEFOREREADING 5
	/**
	* Implements the PixelInspector window.
	*/
	class SPixelInspector : public SCompoundWidget, public FNotifyHook
	{

	public:
		/** Default constructor. */
		SPixelInspector();

		/** Virtual destructor. */
		virtual ~SPixelInspector();

		/** Release all the ressource */
		void ReleaseRessource();

		SLATE_BEGIN_ARGS(SPixelInspector){}
		SLATE_END_ARGS()

		/**
		* Constructs this widget.
		*/
		void Construct(const FArguments& InArgs);

		//~ Begin SCompoundWidget Interface
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		//~ End SCompoundWidget Interface

		void OnApplicationPreInputKeyDownListener(const FKeyEvent& InKeyEvent);

		/** Button handlers */
		ECheckBoxState IsPixelInspectorEnable() const;
		void HandleTogglePixelInspectorEnable(ECheckBoxState CheckType);
		/** End button handlers */

		/*
		 * Create a request and the associate buffers
		 *
		 * ScreenPosition: This is the viewport coordinate in 2D of the pixel to analyze
		 * viewportUniqueId: The id of the view (FSceneView::State::GetViewKey) we want to capture the pixel, ScreenPosition has to come from this viewport
		 * SceneInterface: The interface to set the data for the next render frame.
		*/
		 void CreatePixelInspectorRequest(FIntPoint ScreenPosition, int32 viewportUniqueId, FSceneInterface *SceneInterface);

		 /**
		 * Look if there is some request ready to be read and retrieve the value.
		 * If there is a request that are ready it will read the gpu buffer to get the value
		 * and store the result.
		 * The request will be configure to be available again and the buffers will be release.
		 */
		 void ReadBackRequestData();

	protected:

		/** 
		 * Create the necessary rendertarget buffers for a request and set the render scene data.
		 * First created buffer (1x1) is for the normal (GBufferA) which can be of the following format: PF_FloatRGBA PF_B8G8R8A8 or PF_A2B10G10R10, depending on the precision settings
		 * Second created buffer (1x4) is for the other data (GBuffer B, C, D and E) which can be of the following format: PF_FloatRGBA or PF_B8G8R8A8, depending on the precision settings
		 *
		 * GBufferFormat: 0(Low RGB8), 1 (default), 5(float)
		 *
		 * Return a unique Index to allow the request to know how to find them in the FPixelInspectorData at the post process time when sending the read buffer graphic commands.
		 */
		int32 CreateRequestBuffer(FSceneInterface *SceneInterface, const int32 GBufferFormat);

		/**
		 * Release all Ubuffer with the BufferIndex so the garbage collector will destroy them.
		 */
		void ReleaseBuffers(int32 BufferIndex);

		void OnLevelActorDeleted(AActor* Actor);
		void OnRedrawViewport(bool bInvalidateHitProxies);
	private:

		void ReleaseAllRequests();
		FDelegateHandle OnLevelActorDeletedDelegateHandle;
		FDelegateHandle OnEditorCloseHandle;
		FDelegateHandle OnRedrawViewportHandle;
		FDelegateHandle OnApplicationPreInputKeyDownListenerHandle;

		bool bIsPixelInspectorEnable;

		int32 TickSinceLastCreateRequest;
		FPixelInspectorRequest Requests[2];

		//////////////////////////////////////////////////////////////////////////
		//Buffer management we can do only one pixel inspection per frame
		//We have two buffer of each type to not halt the render thread when we do the read back from the GPU
		//FinalColor Buffer
		UTextureRenderTarget2D* Buffer_FinalColor_RGB8[2];
		//Depth Buffer
		UTextureRenderTarget2D* Buffer_Depth_Float[2];
		//HDR Buffer
		UTextureRenderTarget2D* Buffer_HDR_Float[2];
		//GBufferA RenderTarget
		UTextureRenderTarget2D* Buffer_A_Float[2];
		UTextureRenderTarget2D* Buffer_A_RGB8[2];
		UTextureRenderTarget2D* Buffer_A_RGB10[2];
		//GBuffer BCDE RenderTarget
		UTextureRenderTarget2D* Buffer_BCDE_Float[2];
		UTextureRenderTarget2D* Buffer_BCDE_RGB8[2];
		//Which index we are at for the current Request
		int32 LastBufferIndex;

		//////////////////////////////////////////////////////////////////////////
		// ReadBack Data
		TArray<PixelInspectorResult> AccumulationResult;

		//////////////////////////////////////////////////////////////////////////
		// Display UObject to use the Detail Property Widget
		UPixelInspectorView *DisplayResult;
		TSharedPtr<IDetailsView> DisplayDetailsView;
	};
}
