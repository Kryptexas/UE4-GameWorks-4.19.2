// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "ConnectionDrawingPolicy.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetDebugUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "KismetNodes/KismetNodeInfoContext.h"
#include "SoundDefinitions.h"
#include "AnimationStateNodes/SGraphNodeAnimTransition.h"

//////////////////////////////////////////////////////////////////////////
// FGeometryHelper

FVector2D FGeometryHelper::VerticalMiddleLeftOf(const FGeometry& SomeGeometry)
{
	const FVector2D GeometryDrawSize = SomeGeometry.GetDrawSize();
	return FVector2D(
		SomeGeometry.AbsolutePosition.X,
		SomeGeometry.AbsolutePosition.Y + GeometryDrawSize.Y/2 );
}

FVector2D FGeometryHelper::VerticalMiddleRightOf(const FGeometry& SomeGeometry)
{
	const FVector2D GeometryDrawSize = SomeGeometry.GetDrawSize();
	return FVector2D(
		SomeGeometry.AbsolutePosition.X + GeometryDrawSize.X,
		SomeGeometry.AbsolutePosition.Y + GeometryDrawSize.Y/2 );
}

FVector2D FGeometryHelper::CenterOf(const FGeometry& SomeGeometry)
{
	const FVector2D GeometryDrawSize = SomeGeometry.GetDrawSize();
	return SomeGeometry.AbsolutePosition + (GeometryDrawSize * 0.5f);
}

void FGeometryHelper::ConvertToPoints(const FGeometry& Geom, TArray<FVector2D>& Points)
{
	const FVector2D Size = Geom.GetDrawSize();
	const FVector2D Location = Geom.AbsolutePosition;

	int32 Index = Points.AddUninitialized(4);
	Points[Index++] = Location;
	Points[Index++] = Location + FVector2D(0.0f, Size.Y);
	Points[Index++] = Location + FVector2D(Size.X, Size.Y);
	Points[Index++] = Location + FVector2D(Size.X, 0.0f);
}

/** Find the point on line segment from LineStart to LineEnd which is closest to Point */
FVector2D FGeometryHelper::FindClosestPointOnLine(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& TestPoint)
{
	const FVector2D LineVector = LineEnd - LineStart;

	const float A = -FVector2D::DotProduct(LineStart - TestPoint, LineVector);
	const float B = LineVector.SizeSquared();
	const float T = FMath::Clamp<float>(A / B, 0.0f, 1.0f);

	// Generate closest point
	return LineStart + (T * LineVector);
}

FVector2D FGeometryHelper::FindClosestPointOnGeom(const FGeometry& Geom, const FVector2D& TestPoint)
{
	TArray<FVector2D> Points;
	FGeometryHelper::ConvertToPoints(Geom, Points);

	float BestDistanceSquared = MAX_FLT;
	FVector2D BestPoint;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector2D Candidate = FindClosestPointOnLine(Points[i], Points[(i + 1) % Points.Num()], TestPoint);
		const float CandidateDistanceSquared = (Candidate-TestPoint).SizeSquared();
		if (CandidateDistanceSquared < BestDistanceSquared)
		{
			BestPoint = Candidate;
			BestDistanceSquared = CandidateDistanceSquared;
		}
	}

	return BestPoint;
}

//////////////////////////////////////////////////////////////////////////
// FKismetNodeInfoContext

// Context used to aid debugging displays for nodes
FKismetNodeInfoContext::FKismetNodeInfoContext(UEdGraph* SourceGraph)
	: ActiveObjectBeingDebugged(NULL)
{
	// Only show pending latent actions in PIE/SIE mode
	SourceBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(SourceGraph);

	if (SourceBlueprint != NULL)
	{
		ActiveObjectBeingDebugged = SourceBlueprint->GetObjectBeingDebugged();

		// Run thru debugged objects to see if any are objects with pending latent actions
		if (ActiveObjectBeingDebugged != NULL)
		{
			UBlueprintGeneratedClass* Class = CastChecked<UBlueprintGeneratedClass>((UObject*)(ActiveObjectBeingDebugged->GetClass()));
			FBlueprintDebugData const& ClassDebugData = Class->GetDebugData();

			TSet<UObject*> LatentContextObjects;

			TArray<UK2Node_CallFunction*> FunctionNodes;
			SourceGraph->GetNodesOfClass<UK2Node_CallFunction>(FunctionNodes);
			// collect all the world context objects for all of the graph's latent nodes
			for (UK2Node_CallFunction const* FunctionNode : FunctionNodes)
			{
				UFunction* Function = FunctionNode->GetTargetFunction();
				if ((Function == NULL) || !Function->HasMetaData(FBlueprintMetadata::MD_Latent))
				{
					continue;
				}

				UObject* NodeWorldContext = ActiveObjectBeingDebugged;
				// if the node has a specific "world context" pin, attempt to get the value set for that first
				if (Function->HasMetaData(FBlueprintMetadata::MD_WorldContext))
				{
					FString const WorldContextPinName = Function->GetMetaData(FBlueprintMetadata::MD_WorldContext);
					if (UEdGraphPin* ContextPin = FunctionNode->FindPin(WorldContextPinName))
					{
						if (UObjectPropertyBase* ContextProperty = Cast<UObjectPropertyBase>(ClassDebugData.FindClassPropertyForPin(ContextPin)))
						{
							UObject* PropertyValue = ContextProperty->GetObjectPropertyValue_InContainer(ActiveObjectBeingDebugged);
							if (PropertyValue != NULL)
							{
								NodeWorldContext = PropertyValue;
							}
						}
					}
				}
				
				LatentContextObjects.Add(NodeWorldContext);
			}

			for (UObject* ContextObject : LatentContextObjects)
			{
				if (UWorld* World = GEngine->GetWorldFromContextObject(ContextObject, /*bChecked =*/false))
				{
					FLatentActionManager& Manager = World->GetLatentActionManager();

					TSet<int32> UUIDSet;
					Manager.GetActiveUUIDs(ActiveObjectBeingDebugged, /*out*/ UUIDSet);

					for (TSet<int32>::TConstIterator IterUUID(UUIDSet); IterUUID; ++IterUUID)
					{
						const int32 UUID = *IterUUID;

						if (UEdGraphNode* ParentNode = ClassDebugData.FindNodeFromUUID(UUID))
						{
 							TArray<FObjectUUIDPair>& Pairs = NodesWithActiveLatentActions.FindOrAdd(ParentNode);
 							new (Pairs) FObjectUUIDPair(ContextObject, UUID);
						}
					}
				}
			}
		}

		// Covert the watched pin array into a set
		for (auto WatchedPinIt = SourceBlueprint->PinWatches.CreateConstIterator(); WatchedPinIt; ++WatchedPinIt)
		{
			UEdGraphPin* WatchedPin = *WatchedPinIt;

			UEdGraphNode* OwningNode = Cast<UEdGraphNode>(WatchedPin->GetOuter());
			if (!ensure(OwningNode != NULL)) // shouldn't happen, but just in case a dead pin was added to the PinWatches array
			{
				continue;
			}
			check(OwningNode == WatchedPin->GetOwningNode());

			WatchedPinSet.Add(WatchedPin);
			WatchedNodeSet.Add(OwningNode);
		}
	}
}

/////////////////////////////////////////////////////
// FConnectionDrawingPolicy

FConnectionDrawingPolicy::FConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
	: WireLayerID(InBackLayerID)
	, ArrowLayerID(InFrontLayerID)
	, ZoomFactor(InZoomFactor)
	, ClippingRect(InClippingRect)
	, DrawElementsList(InDrawElements)
{
	ArrowImage = FEditorStyle::GetBrush( TEXT("Graph.Arrow") );
	ArrowRadius = ArrowImage->ImageSize * ZoomFactor * 0.5f;
	MidpointImage = NULL;
	MidpointRadius = FVector2D::ZeroVector;

	BubbleImage = FEditorStyle::GetBrush( TEXT("Graph.ExecutionBubble") );
}

void FConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional)
{
	// Draw the spline
	DrawConnection(
		WireLayerID,
		StartPoint,
		EndPoint,
		WireColor,
		WireThickness,
		bDrawBubbles);

	// Draw the arrow
	if (ArrowImage != nullptr)
	{
		FVector2D ArrowPoint = EndPoint - ArrowRadius;

		FSlateDrawElement::MakeBox(
			DrawElementsList,
			ArrowLayerID,
			FPaintGeometry(ArrowPoint, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
			ArrowImage,
			ClippingRect,
			ESlateDrawEffect::None,
			WireColor
			);
	}
}

void FConnectionDrawingPolicy::DrawSplineWithArrow(FGeometry& StartGeom, FGeometry& EndGeom, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional)
{
	const FVector2D StartPoint = FGeometryHelper::VerticalMiddleRightOf( StartGeom );
	const FVector2D EndPoint = FGeometryHelper::VerticalMiddleLeftOf( EndGeom ) - FVector2D(ArrowRadius.X, 0);

	DrawSplineWithArrow(StartPoint, EndPoint, WireColor, WireThickness, bDrawBubbles, Bidirectional);
}

// Update the drawing policy with the set of hovered pins (which can be empty)
void FConnectionDrawingPolicy::SetHoveredPins(const TSet< TWeakObjectPtr<UEdGraphPin> >& InHoveredPins, const TArray< TSharedPtr<SGraphPin> >& OverridePins, double HoverTime)
{
	HoveredPins.Empty();

	LastHoverTimeEvent = (OverridePins.Num() > 0) ? 0.0 : HoverTime;

	for (auto PinIt = OverridePins.CreateConstIterator(); PinIt; ++PinIt)
	{
		if (SGraphPin* GraphPin = PinIt->Get())
		{
			HoveredPins.Add(GraphPin->GetPinObj());
		}
	}

	// Convert the widget pointer for hovered pins to be EdGraphPin pointers for their connected nets (both ends of any connection)
	for (auto PinIt = InHoveredPins.CreateConstIterator(); PinIt; ++PinIt)
	{
		if (UEdGraphPin* Pin = PinIt->Get())
		{
			if (Pin->LinkedTo.Num() > 0)
			{
				HoveredPins.Add(Pin);

				for (auto LinkIt = Pin->LinkedTo.CreateConstIterator(); LinkIt; ++LinkIt)
				{
					HoveredPins.Add(*LinkIt);
				}
			}
		}
	}
}

void FConnectionDrawingPolicy::SetMarkedPin(TWeakPtr<SGraphPin> InMarkedPin)
{
	if (InMarkedPin.IsValid())
	{
		LastHoverTimeEvent = 0.0;

		UEdGraphPin* MarkedPin = InMarkedPin.Pin()->GetPinObj();
		HoveredPins.Add(MarkedPin);

		for (auto LinkIt = MarkedPin->LinkedTo.CreateConstIterator(); LinkIt; ++LinkIt)
		{
			HoveredPins.Add(*LinkIt);
		}
	}
}

/** Util to make a 'distance->alpha' table and also return spline length */
float FConnectionDrawingPolicy::MakeSplineReparamTable(const FVector2D& P0, const FVector2D& P0Tangent, const FVector2D& P1, const FVector2D& P1Tangent, FInterpCurve<float>& OutReparamTable)
{
	OutReparamTable.Reset();

	const int32 NumSteps = 10; // TODO: Make this adaptive...

	// Find range of input
	float Param = 0.f;
	const float MaxInput = 1.f;
	const float Interval = (MaxInput - Param)/((float)(NumSteps-1)); 

	// Add first entry, using first point on curve, total distance will be 0
	FVector2D OldSplinePos = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, Param);
	float TotalDist = 0.f;
	OutReparamTable.AddPoint(TotalDist, Param);
	Param += Interval;

	// Then work over rest of points	
	for(int32 i=1; i<NumSteps; i++)
	{
		// Iterate along spline at regular param intervals
		const FVector2D NewSplinePos = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, Param);
		TotalDist += (NewSplinePos - OldSplinePos).Size();
		OldSplinePos = NewSplinePos;

		OutReparamTable.AddPoint(TotalDist, Param);

		Param += Interval;
	}

	return TotalDist;
}

void FConnectionDrawingPolicy::DrawConnection( int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& InColor, float Thickness, bool bDrawBubbles )
{
	const FVector2D& P0 = Start;
	const FVector2D& P1 = End;

	const int32 Tension  = FMath::Abs<int32>(Start.X - End.X);
	const FVector2D P0Tangent = Tension * FVector2D(1.0f, 0);
	const FVector2D P1Tangent = P0Tangent;

	// Draw the spline itself
	FSlateDrawElement::MakeDrawSpaceSpline(
		DrawElementsList,
		LayerId,
		P0, P0Tangent,
		P1, P1Tangent,
		ClippingRect,
		Thickness,
		ESlateDrawEffect::None,
		InColor
	);

	if (bDrawBubbles || (MidpointImage != NULL))
	{
		// This table maps distance along curve to alpha
		FInterpCurve<float> SplineReparamTable;
		float SplineLength = MakeSplineReparamTable(P0, P0Tangent, P1, P1Tangent, SplineReparamTable);

		// Draw bubbles on the spline
		if (bDrawBubbles)
		{
			const float BubbleSpacing = 64.f * ZoomFactor;
			const float BubbleSpeed = 192.f * ZoomFactor;
			const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.1f * Thickness;

			float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::Ceil(SplineLength/BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = ((float)i * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
					FVector2D BubblePos = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, Alpha);
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(
						DrawElementsList,
						LayerId,
						FPaintGeometry( BubblePos, BubbleSize, ZoomFactor  ),
						BubbleImage,
						ClippingRect,
						ESlateDrawEffect::None,
						InColor
						);
				}
			}
		}

		// Draw the midpoint image
		if (MidpointImage != NULL)
		{
			// Determine the spline position for the midpoint
			const float MidpointAlpha = SplineReparamTable.Eval(SplineLength * 0.5f, 0.f);
			const FVector2D Midpoint = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha);

			// Approximate the slope at the midpoint (to orient the midpoint image to the spline)
			const FVector2D MidpointPlusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha + KINDA_SMALL_NUMBER);
			const FVector2D MidpointMinusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha - KINDA_SMALL_NUMBER);
			const FVector2D SlopeUnnormalized = MidpointPlusE - MidpointMinusE;

			// Draw the arrow
			const FVector2D MidpointDrawPos = Midpoint - MidpointRadius;
			const float AngleInRadians = SlopeUnnormalized.IsNearlyZero() ? 0.0f : FMath::Atan2(SlopeUnnormalized.Y, SlopeUnnormalized.X);

			FSlateDrawElement::MakeRotatedBox(
				DrawElementsList,
				LayerId,
				FPaintGeometry(MidpointDrawPos, MidpointImage->ImageSize * ZoomFactor, ZoomFactor),
				MidpointImage,
				ClippingRect,
				ESlateDrawEffect::None,
				AngleInRadians,
				TOptional<FVector2D>(),
				FSlateDrawElement::RelativeToElement,
				InColor
				);
		}
	}

}

void FConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	float Thickness = 1.0f;
	FLinearColor WireColor = FLinearColor::White;
	bool bDrawBubbles = false;
	bool bBiDirectional = false;
	DetermineWiringStyle(Pin, NULL, /*inout*/ Thickness, /*inout*/ WireColor, /*inout*/ bDrawBubbles, /*inout*/ bBiDirectional);
	DrawSplineWithArrow(StartPoint, EndPoint, WireColor, Thickness, bDrawBubbles, bBiDirectional);
}

void FConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool &bBidirectional)
{
}

void FConnectionDrawingPolicy::DetermineLinkGeometry(
	TMap<TSharedRef<SWidget>,
	FArrangedWidget>& PinGeometries,
	FArrangedChildren& ArrangedNodes, 
	TSharedRef<SWidget>& OutputPinWidget,
	UEdGraphPin* OutputPin,
	UEdGraphPin* InputPin,
	/*out*/ FArrangedWidget*& StartWidgetGeometry,
	/*out*/ FArrangedWidget*& EndWidgetGeometry
	)
{
	StartWidgetGeometry = PinGeometries.Find(OutputPinWidget);
	
	if (TSharedRef<SGraphPin>* pTargetWidget = PinToPinWidgetMap.Find(InputPin))
	{
		TSharedRef<SGraphPin> InputWidget = *pTargetWidget;
		EndWidgetGeometry = PinGeometries.Find(InputWidget);
	}
}

void FConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes)
{
	PinToPinWidgetMap.Empty();
	for (TMap<TSharedRef<SWidget>, FArrangedWidget>::TIterator ConnectorIt(PinGeometries); ConnectorIt; ++ConnectorIt)
	{
		TSharedRef<SWidget> SomePinWidget = ConnectorIt.Key();
		SGraphPin& PinWidget = static_cast<SGraphPin&>(SomePinWidget.Get());

		PinToPinWidgetMap.Add(PinWidget.GetPinObj(), StaticCastSharedRef<SGraphPin>(SomePinWidget));
	}

	for (TMap<TSharedRef<SWidget>, FArrangedWidget>::TIterator ConnectorIt(PinGeometries); ConnectorIt; ++ConnectorIt)
	{
		TSharedRef<SWidget> SomePinWidget = ConnectorIt.Key();
		SGraphPin& PinWidget = static_cast<SGraphPin&>(SomePinWidget.Get());
		UEdGraphPin* ThePin = PinWidget.GetPinObj();

		if (ThePin->Direction == EGPD_Output)
		{
			for (int32 LinkIndex=0; LinkIndex < ThePin->LinkedTo.Num(); ++LinkIndex)
			{
				FArrangedWidget* LinkStartWidgetGeometry = NULL;
				FArrangedWidget* LinkEndWidgetGeometry = NULL;

				UEdGraphPin* TargetPin = ThePin->LinkedTo[LinkIndex];

				DetermineLinkGeometry(PinGeometries, ArrangedNodes, SomePinWidget, ThePin, TargetPin, /*out*/ LinkStartWidgetGeometry, /*out*/ LinkEndWidgetGeometry);

				if ((LinkEndWidgetGeometry != NULL) && (LinkStartWidgetGeometry != NULL))
				{
					float Thickness = 1.0f;
					FLinearColor WireColor = FLinearColor::White;
					bool bDrawBubbles = false;
					bool bBidirectional = false;

					DetermineWiringStyle(ThePin, TargetPin, /*inout*/ Thickness, /*inout*/ WireColor, /*inout*/ bDrawBubbles, /*inout*/ bBidirectional);

					DrawSplineWithArrow(LinkStartWidgetGeometry->Geometry, LinkEndWidgetGeometry->Geometry, WireColor, Thickness, bDrawBubbles, bBidirectional);
				}
			}
		}
	}
}

void FConnectionDrawingPolicy::SetIncompatiblePinDrawState(const TSharedPtr<SGraphPin>& StartPin, const TSet< TSharedRef<SWidget> >& VisiblePins)
{
}

void FConnectionDrawingPolicy::ResetIncompatiblePinDrawState(const TSet< TSharedRef<SWidget> >& VisiblePins)
{
}

void FConnectionDrawingPolicy::ApplyHoverDeemphasis(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor)
{
	const float FadeInBias = 0.75f; // Time in seconds before the fading starts to occur
	const float FadeInPeriod = 0.6f; // Time in seconds after the bias before the fade is fully complete
	const float TimeFraction = FMath::SmoothStep(0.0f, FadeInPeriod, (float)(FSlateApplication::Get().GetCurrentTime() - LastHoverTimeEvent - FadeInBias));

	const float DarkFraction = 0.8f;
	const float LightFraction = 0.25f;
	const FLinearColor DarkenedColor(0.0f, 0.0f, 0.0f, 0.5f);
	const FLinearColor LightenedColor(1.0f, 1.0f, 1.0f, 1.0f);

	const bool bContainsBoth = HoveredPins.Contains(InputPin) && HoveredPins.Contains(OutputPin);
	const bool bContainsOutput = HoveredPins.Contains(OutputPin);
	const bool bEmphasize = bContainsBoth || (bContainsOutput && (InputPin == NULL));
	if (bEmphasize)
	{
		Thickness = FMath::Lerp(Thickness, Thickness * ((Thickness < 3.0f) ? 5.0f : 3.0f), TimeFraction);
		WireColor = FMath::Lerp<FLinearColor>(WireColor, LightenedColor, LightFraction * TimeFraction);
	}
	else
	{
		WireColor = FMath::Lerp<FLinearColor>(WireColor, DarkenedColor, DarkFraction * TimeFraction);
	}
}

/////////////////////////////////////////////////////
// FKismetConnectionDrawingPolicy

FKismetConnectionDrawingPolicy::FKismetConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	const UEditorUserSettings& Options = GEditor->AccessEditorUserSettings();

	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;

	// But we do want to draw midpoint arrowheads
	if (GetDefault<UEditorExperimentalSettings>()->bDrawMidpointArrowsInBlueprints)
	{
		MidpointImage = FEditorStyle::GetBrush( TEXT("Graph.Arrow") );
		MidpointRadius = MidpointImage->ImageSize * ZoomFactor * 0.5f;
	}

	// Cache off the editor options
	AttackColor = Options.TraceAttackColor;
	SustainColor = Options.TraceSustainColor;
	ReleaseColor = Options.TraceReleaseColor;

	AttackWireThickness = Options.TraceAttackWireThickness;
	SustainWireThickness = Options.TraceSustainWireThickness;
	ReleaseWireThickness = Options.TraceReleaseWireThickness;

	TracePositionBonusPeriod = Options.TracePositionBonusPeriod;
	TracePositionExponent = Options.TracePositionExponent;
	AttackHoldPeriod = Options.TraceAttackHoldPeriod;
	DecayPeriod = Options.TraceDecayPeriod;
	DecayExponent = Options.TraceDecayExponent;
	SustainHoldPeriod = Options.TraceSustainHoldPeriod;
	ReleasePeriod = Options.TraceReleasePeriod;
	ReleaseExponent = Options.TraceReleaseExponent;

	CurrentTime = 0.0;
	LatestTimeDiscovered = 0.0;
}

void FKismetConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build the execution roadmap (also populates execution times)
	BuildExecutionRoadmap();

	// Draw everything
	FConnectionDrawingPolicy::Draw(PinGeometries, ArrangedNodes);
}

bool FKismetConnectionDrawingPolicy::CanBuildRoadmap() const
{
	UBlueprint* TargetBP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(GraphObj);
	UObject* ActiveObject = TargetBP->GetObjectBeingDebugged();

	return ActiveObject != NULL;
}

void FKismetConnectionDrawingPolicy::BuildExecutionRoadmap()
{
	LatestTimeDiscovered = 0.0;

	// Only do highlighting in PIE or SIE
	if (!CanBuildRoadmap())
	{
		return;
	}

	UBlueprint* TargetBP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(GraphObj);
	UObject* ActiveObject = TargetBP->GetObjectBeingDebugged();
	check(ActiveObject); // Due to CanBuildRoadmap

	// Redirect the target Blueprint when debugging with a macro graph visible
	if (TargetBP->BlueprintType == BPTYPE_MacroLibrary)
	{
		TargetBP = Cast<UBlueprint>(ActiveObject->GetClass()->ClassGeneratedBy);
	}

	TArray<UEdGraphNode*> SequentialNodesInGraph;
	TArray<double> SequentialNodeTimes;

	{
		const TSimpleRingBuffer<FKismetTraceSample>& TraceStack = FKismetDebugUtilities::GetTraceStack();

		UBlueprintGeneratedClass* TargetClass = Cast<UBlueprintGeneratedClass>(TargetBP->GeneratedClass);
		FBlueprintDebugData& DebugData = TargetClass->GetDebugData();

		for (int32 i = 0; i < TraceStack.Num(); ++i)
		{
			const FKismetTraceSample& Sample = TraceStack(i);

			if (UObject* TestObject = Sample.Context.Get())
			{
				if (TestObject == ActiveObject)
				{
					if (UEdGraphNode* Node = DebugData.FindSourceNodeFromCodeLocation(Sample.Function.Get(), Sample.Offset, /*bAllowImpreciseHit=*/ false))
					{
						if (GraphObj == Node->GetGraph())
						{
							SequentialNodesInGraph.Add(Node);
							SequentialNodeTimes.Add(Sample.ObservationTime);
						}
						else
						{
							// If the top-level source node is a macro instance node
							UK2Node_MacroInstance* MacroInstanceNode = Cast<UK2Node_MacroInstance>(Node);
							if (MacroInstanceNode)
							{
								// Attempt to locate the macro source node through the code mapping
								UEdGraphNode* MacroSourceNode = DebugData.FindMacroSourceNodeFromCodeLocation(Sample.Function.Get(), Sample.Offset);
								if (MacroSourceNode)
								{
									// If the macro source node is located in the current graph context
									if (GraphObj == MacroSourceNode->GetGraph())
									{
										// Add it to the sequential node list
										SequentialNodesInGraph.Add(MacroSourceNode);
										SequentialNodeTimes.Add(Sample.ObservationTime);
									}
									else
									{
										// The macro source node isn't in the current graph context, but we might have a macro instance node that is
										// in the current graph context, so obtain the set of macro instance nodes that are mapped to the code here.
										TArray<UEdGraphNode*> MacroInstanceNodes;
										DebugData.FindMacroInstanceNodesFromCodeLocation(Sample.Function.Get(), Sample.Offset, MacroInstanceNodes);

										// For each macro instance node in the set
										for (auto MacroInstanceNodeIt = MacroInstanceNodes.CreateConstIterator(); MacroInstanceNodeIt; ++MacroInstanceNodeIt)
										{
											// If the macro instance node is located in the current graph context
											MacroInstanceNode = Cast<UK2Node_MacroInstance>(*MacroInstanceNodeIt);
											if (MacroInstanceNode && GraphObj == MacroInstanceNode->GetGraph())
											{
												// Add it to the sequential node list
												SequentialNodesInGraph.Add(MacroInstanceNode);
												SequentialNodeTimes.Add(Sample.ObservationTime);

												// Exit the loop; we're done
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Run thru and apply bonus time
	const float InvNumNodes = 1.0f / (float)SequentialNodeTimes.Num();
	for (int32 i = 0; i < SequentialNodesInGraph.Num(); ++i)
	{
		double& ObservationTime = SequentialNodeTimes[i];

		const float PositionRatio = (SequentialNodeTimes.Num() - i) * InvNumNodes;
		const float PositionBonus = FMath::Pow(PositionRatio, TracePositionExponent) * TracePositionBonusPeriod;
		ObservationTime += PositionBonus;

		LatestTimeDiscovered = FMath::Max<double>(LatestTimeDiscovered, ObservationTime);
	}

	// Record the unique node->node pairings, keeping only the most recent times for each pairing
	for (int32 i = SequentialNodesInGraph.Num() - 1; i >= 1; --i)
	{
		UEdGraphNode* CurNode = SequentialNodesInGraph[i];
		double CurNodeTime = SequentialNodeTimes[i];
		UEdGraphNode* NextNode = SequentialNodesInGraph[i-1];
		double NextNodeTime = SequentialNodeTimes[i-1];

		FExecPairingMap& Predecessors = PredecessorNodes.FindOrAdd(NextNode);

		// Update the timings if this is a more recent pairing
		FTimePair& Timings = Predecessors.FindOrAdd(CurNode);
		if (Timings.ThisExecTime < NextNodeTime)
		{
			Timings.PredExecTime = CurNodeTime;
			Timings.ThisExecTime = NextNodeTime;
		}
	}

	// Fade only when free-running (since we're using GCurrentTime, instead of FPlatformTime::Seconds)
	const double MaxTimeAhead = FMath::Min(GCurrentTime + 2*TracePositionBonusPeriod, LatestTimeDiscovered); //@TODO: Rough clamping; should be exposed as a parameter
	CurrentTime = FMath::Max(GCurrentTime, MaxTimeAhead);
}

void FKismetConnectionDrawingPolicy::CalculateEnvelopeAlphas(double ExecutionTime, /*out*/ float& AttackAlpha, /*out*/ float& SustainAlpha) const
{
	const float DeltaTime = (float)(CurrentTime - ExecutionTime);

	{
		const float UnclampedDecayRatio = 1.0f - ((DeltaTime - AttackHoldPeriod) / DecayPeriod);
		const float ClampedDecayRatio = FMath::Clamp<float>(UnclampedDecayRatio, 0.0f, 1.0f);
		AttackAlpha = FMath::Pow(ClampedDecayRatio, DecayExponent);
	}

	{
		const float SustainEndTime = AttackHoldPeriod + DecayPeriod + SustainHoldPeriod;

		const float UnclampedReleaseRatio = 1.0f - ((DeltaTime - SustainEndTime) / ReleasePeriod);
		const float ClampedReleaseRatio = FMath::Clamp<float>(UnclampedReleaseRatio, 0.0f, 1.0f);
		SustainAlpha = FMath::Pow(ClampedReleaseRatio, ReleaseExponent);
	}
}

bool FKismetConnectionDrawingPolicy::TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	return (InputPin != NULL) && (Schema->IsExecPin(*OutputPin));
}

void FKismetConnectionDrawingPolicy::DetermineStyleOfExecWire(float& Thickness, FLinearColor& WireColor, bool& bDrawBubbles, const FTimePair& Times)
{
	// It's a followed link, make it strong and yellowish but fading over time
	const double ExecTime = Times.ThisExecTime;

	float AttackAlpha;
	float SustainAlpha;
	CalculateEnvelopeAlphas(ExecTime, /*out*/ AttackAlpha, /*out*/ SustainAlpha);

	const float DecayedAttackThickness = FMath::Lerp<float>(SustainWireThickness, AttackWireThickness, AttackAlpha);
	Thickness = FMath::Lerp<float>(ReleaseWireThickness, DecayedAttackThickness, SustainAlpha);

	const FLinearColor DecayedAttackColor = FMath::Lerp<FLinearColor>(SustainColor, AttackColor, AttackAlpha);
	WireColor = WireColor * FMath::Lerp<FLinearColor>(ReleaseColor, DecayedAttackColor, SustainAlpha);

	if (SustainAlpha > KINDA_SMALL_NUMBER)
	{
		bDrawBubbles = true;
	}
}

// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
void FKismetConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/bool& bBidirectional)
{
	// Get the schema and grab the default color from it
	check(OutputPin);
	check(GraphObj);
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	WireColor = Schema->GetPinTypeColor(OutputPin->PinType);

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;

	// If this is a K2 graph, try to be a little more specific
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Schema);
	if (K2Schema != NULL)
	{
		if (TreatWireAsExecutionPin(InputPin, OutputPin))
		{
			if (CanBuildRoadmap())
			{
				bool bExecuted = false;

				// Run thru the predecessors, and on
				if (FExecPairingMap* PredecessorMap = PredecessorNodes.Find(InputPin->GetOwningNode()))
				{
					if (FTimePair* Times = PredecessorMap->Find(OutputPin->GetOwningNode()))
					{
						bExecuted = true;

						DetermineStyleOfExecWire(/*inout*/ Thickness, /*inout*/ WireColor, /*inout*/ bDrawBubbles, *Times);
					}
				}

				if (!bExecuted)
				{
					// It's not followed, fade it and keep it thin
					WireColor = ReleaseColor;
					Thickness = ReleaseWireThickness;
				}
			}
			else
			{
				// Make exec wires slightly thicker even outside of debug
				Thickness = 3.0f;
			}
		}
		else
		{
			// Array types should draw thicker
			if( (InputPin && InputPin->PinType.bIsArray) || (OutputPin && OutputPin->PinType.bIsArray) )
			{
				Thickness = 3.0f;
			}
		}
	}

	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Thickness, /*inout*/ WireColor);
	}
}

void FKismetConnectionDrawingPolicy::SetIncompatiblePinDrawState(const TSharedPtr<SGraphPin>& StartPin, const TSet< TSharedRef<SWidget> >& VisiblePins)
{
	ResetIncompatiblePinDrawState(VisiblePins);

	for(auto VisiblePinIterator = VisiblePins.CreateConstIterator(); VisiblePinIterator; ++VisiblePinIterator)
	{
		TSharedPtr<SGraphPin> CheckPin = StaticCastSharedRef<SGraphPin>(*VisiblePinIterator);
		if(CheckPin != StartPin)
		{
			const FPinConnectionResponse Response = StartPin->GetPinObj()->GetSchema()->CanCreateConnection(StartPin->GetPinObj(), CheckPin->GetPinObj());
			if(Response.Response == CONNECT_RESPONSE_DISALLOW)
			{
				CheckPin->SetPinColorModifier(FLinearColor(0.25f, 0.25f, 0.25f, 0.5f));
			}
		}
	}
}

void FKismetConnectionDrawingPolicy::ResetIncompatiblePinDrawState(const TSet< TSharedRef<SWidget> >& VisiblePins)
{
	for(auto VisiblePinIterator = VisiblePins.CreateConstIterator(); VisiblePinIterator; ++VisiblePinIterator)
	{
		TSharedPtr<SGraphPin> VisiblePin = StaticCastSharedRef<SGraphPin>(*VisiblePinIterator);
		VisiblePin->SetPinColorModifier(FLinearColor::White);
	}
}

/////////////////////////////////////////////////////
// FStateMachineConnectionDrawingPolicy

FStateMachineConnectionDrawingPolicy::FStateMachineConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
}

void FStateMachineConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/ bool& bDrawBubbles, /*inout*/ bool& bBidirectional)
{
	Thickness = 1.5f;

	if (InputPin)
	{
		if (UAnimStateTransitionNode* TransNode = Cast<UAnimStateTransitionNode>(InputPin->GetOwningNode()))
		{
			WireColor = SGraphNodeAnimTransition::StaticGetTransitionColor(TransNode, HoveredPins.Contains(InputPin));

			bBidirectional = TransNode->Bidirectional;
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Thickness, /*inout*/ WireColor);
	}
}

void FStateMachineConnectionDrawingPolicy::DetermineLinkGeometry(
	TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries,
	FArrangedChildren& ArrangedNodes, 
	TSharedRef<SWidget>& OutputPinWidget,
	UEdGraphPin* OutputPin,
	UEdGraphPin* InputPin,
	/*out*/ FArrangedWidget*& StartWidgetGeometry,
	/*out*/ FArrangedWidget*& EndWidgetGeometry
	)
{
	if (UAnimStateEntryNode* EntryNode = Cast<UAnimStateEntryNode>(OutputPin->GetOwningNode()))
	{
		//FConnectionDrawingPolicy::DetermineLinkGeometry(PinGeometries, ArrangedNodes, OutputPinWidget, OutputPin, InputPin, StartWidgetGeometry, EndWidgetGeometry);

		StartWidgetGeometry = PinGeometries.Find(OutputPinWidget);

		UAnimStateNodeBase* State = CastChecked<UAnimStateNodeBase>(InputPin->GetOwningNode());
		int32 StateIndex = NodeWidgetMap.FindChecked(State);
		EndWidgetGeometry = &(ArrangedNodes(StateIndex));
	}
	else if (UAnimStateTransitionNode* TransNode = Cast<UAnimStateTransitionNode>(InputPin->GetOwningNode()))
	{
		UAnimStateNodeBase* PrevState = TransNode->GetPreviousState();
		UAnimStateNodeBase* NextState = TransNode->GetNextState();
		if ((PrevState != NULL) && (NextState != NULL))
		{
			int32* PrevNodeIndex = NodeWidgetMap.Find(PrevState);
			int32* NextNodeIndex = NodeWidgetMap.Find(NextState);
			if ((PrevNodeIndex != NULL) && (NextNodeIndex != NULL))
			{
				StartWidgetGeometry = &(ArrangedNodes(*PrevNodeIndex));
				EndWidgetGeometry = &(ArrangedNodes(*NextNodeIndex));
			}
		}
	}
	else
	{
		StartWidgetGeometry = PinGeometries.Find(OutputPinWidget);

		if (TSharedRef<SGraphPin>* pTargetWidget = PinToPinWidgetMap.Find(InputPin))
		{
			TSharedRef<SGraphPin> InputWidget = *pTargetWidget;
			EndWidgetGeometry = PinGeometries.Find(InputWidget);
		}
	}
}

void FStateMachineConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes(NodeIndex);
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(PinGeometries, ArrangedNodes);
}

void FStateMachineConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	float Thickness = 1.0f;
	FLinearColor WireColor = FLinearColor::White;
	bool bDrawBubbles = false;
	bool bBiDirectional = false;
	DetermineWiringStyle(Pin, NULL, /*inout*/ Thickness, /*inout*/ WireColor, /*inout*/ bDrawBubbles, /*inout*/ bBiDirectional);


	const FVector2D SeedPoint = EndPoint;
	const FVector2D AdjustedStartPoint = FGeometryHelper::FindClosestPointOnGeom(PinGeometry, SeedPoint);

	DrawSplineWithArrow(AdjustedStartPoint, EndPoint, WireColor, Thickness, bDrawBubbles, bBiDirectional);
}


void FStateMachineConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional)
{
	Internal_DrawLineWithArrow(StartAnchorPoint, EndAnchorPoint, WireColor, WireThickness, bDrawBubbles);
	if (Bidirectional)
	{
		Internal_DrawLineWithArrow(EndAnchorPoint, StartAnchorPoint, WireColor, WireThickness, bDrawBubbles);
	}
}

void FStateMachineConnectionDrawingPolicy::Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles)
{
	//@TODO: Should this be scaled by zoom factor?
	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.SafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).SafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = StartAnchorPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = EndAnchorPoint + DirectionBias - LengthBias;

	// Draw a line/spline
	DrawConnection(WireLayerID, StartPoint, EndPoint, WireColor, WireThickness, bDrawBubbles);

	// Draw the arrow
	const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
	const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

	FSlateDrawElement::MakeRotatedBox(
		DrawElementsList,
		ArrowLayerID,
		FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
		ArrowImage,
		ClippingRect,
		ESlateDrawEffect::None,
		AngleInRadians,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		WireColor
		);
}

void FStateMachineConnectionDrawingPolicy::DrawSplineWithArrow(FGeometry& StartGeom, FGeometry& EndGeom, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;
	
	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, WireColor, WireThickness, bDrawBubbles, Bidirectional);
}

void FStateMachineConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& InColor, float Thickness, bool bDrawBubbles)
{
	const FVector2D& P0 = Start;
	const FVector2D& P1 = End;

	const FVector2D Delta = End-Start;
	const FVector2D NormDelta = Delta.SafeNormal();
	
	const FVector2D P0Tangent = NormDelta;
	const FVector2D P1Tangent = NormDelta;

	// Draw the spline itself
	FSlateDrawElement::MakeDrawSpaceSpline(
		DrawElementsList,
		LayerId,
		P0, P0Tangent,
		P1, P1Tangent,
		ClippingRect,
		Thickness,
		ESlateDrawEffect::None,
		InColor
		);

	//@TODO: Handle bDrawBubbles
}

/////////////////////////////////////////////////////
// FAnimGraphConnectionDrawingPolicy

FAnimGraphConnectionDrawingPolicy::FAnimGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
{
}

bool FAnimGraphConnectionDrawingPolicy::TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();

	return (InputPin != NULL) && (Schema->IsPosePin(OutputPin->PinType));
}

void FAnimGraphConnectionDrawingPolicy::BuildExecutionRoadmap()
{
	UAnimBlueprint* TargetBP = CastChecked<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForGraphChecked(GraphObj));
	UAnimBlueprintGeneratedClass* AnimBlueprintClass = (UAnimBlueprintGeneratedClass*)(*(TargetBP->GeneratedClass));

	if (TargetBP->GetObjectBeingDebugged() == NULL)
	{
		return;
	}

	TMap<UProperty*, UObject*> PropertySourceMap;
	AnimBlueprintClass->GetDebugData().GenerateReversePropertyMap(/*out*/ PropertySourceMap);

	FAnimBlueprintDebugData& DebugInfo = AnimBlueprintClass->GetAnimBlueprintDebugData();
	for (auto VisitIt = DebugInfo.UpdatedNodesThisFrame.CreateIterator(); VisitIt; ++VisitIt)
	{
		const FAnimBlueprintDebugData::FNodeVisit& VisitRecord = *VisitIt;

		if ((VisitRecord.SourceID >= 0) && (VisitRecord.SourceID < AnimBlueprintClass->AnimNodeProperties.Num()) && (VisitRecord.TargetID >= 0) && (VisitRecord.TargetID < AnimBlueprintClass->AnimNodeProperties.Num()))
		{
			if (UAnimGraphNode_Base* SourceNode = Cast<UAnimGraphNode_Base>(PropertySourceMap.FindRef(AnimBlueprintClass->AnimNodeProperties[VisitRecord.SourceID])))
			{
				if (UAnimGraphNode_Base* TargetNode = Cast<UAnimGraphNode_Base>(PropertySourceMap.FindRef(AnimBlueprintClass->AnimNodeProperties[VisitRecord.TargetID])))
				{
					//@TODO: Extend the rendering code to allow using the recorded blend weight instead of faked exec times
					FExecPairingMap& Predecessors = PredecessorNodes.FindOrAdd((UEdGraphNode*)SourceNode);
					FTimePair& Timings = Predecessors.FindOrAdd((UEdGraphNode*)TargetNode);
					Timings.PredExecTime = 0.0;
					Timings.ThisExecTime = VisitRecord.Weight;
				}
			}
		}
	}
}

void FAnimGraphConnectionDrawingPolicy::DetermineStyleOfExecWire(float& Thickness, FLinearColor& WireColor, bool& bDrawBubbles, const FTimePair& Times)
{
	// It's a followed link, make it strong and yellowish but fading over time
	const double BlendWeight = Times.ThisExecTime;

	const float HeavyBlendThickness = AttackWireThickness;
	const float LightBlendThickness = SustainWireThickness;

	Thickness = FMath::Lerp<float>(LightBlendThickness, HeavyBlendThickness, BlendWeight);
	WireColor = WireColor * (BlendWeight * 0.5f + 0.5f);//FMath::Lerp<FLinearColor>(SustainColor, AttackColor, BlendWeight);

	bDrawBubbles = true;
}

/////////////////////////////////////////////////////
// FSoundCueGraphConnectionDrawingPolicy

FSoundCueGraphConnectionDrawingPolicy::FSoundCueGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	// Cache off the editor options
	const UEditorUserSettings& Options = GEditor->AccessEditorUserSettings();
	ActiveColor = Options.TraceAttackColor;
	InactiveColor = Options.TraceReleaseColor;

	ActiveWireThickness = Options.TraceAttackWireThickness;
	InactiveWireThickness = Options.TraceReleaseWireThickness;

	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;
}

void FSoundCueGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build the execution roadmap (also populates execution times)
	BuildAudioFlowRoadmap();

	// Draw everything
	FConnectionDrawingPolicy::Draw(PinGeometries, ArrangedNodes);
}

void FSoundCueGraphConnectionDrawingPolicy::BuildAudioFlowRoadmap()
{
	FAudioDevice* AudioDevice = GEngine->GetAudioDevice();

	if (AudioDevice)
	{
		USoundCueGraph* SoundCueGraph = CastChecked<USoundCueGraph>(GraphObj);
		USoundCue* SoundCue = SoundCueGraph->GetSoundCue();

		UAudioComponent* PreviewAudioComponent = GEditor->GetPreviewAudioComponent();

		if (PreviewAudioComponent && PreviewAudioComponent->IsPlaying() && PreviewAudioComponent->Sound == SoundCue)
		{
			TArray<FWaveInstance*> WaveInstances;
			const int32 FirstActiveIndex = AudioDevice->GetSortedActiveWaveInstances(WaveInstances, ESortedActiveWaveGetType::QueryOnly);

			// Run through the active instances and cull out anything that isn't related to this graph
			if (FirstActiveIndex > 0)
			{
				WaveInstances.RemoveAt(0, FirstActiveIndex + 1);
			}

			for (int32 WaveIndex = WaveInstances.Num() - 1; WaveIndex >= 0 ; --WaveIndex)
			{
				UAudioComponent* WaveInstanceAudioComponent = WaveInstances[WaveIndex]->ActiveSound->AudioComponent.Get();
				if (WaveInstanceAudioComponent != PreviewAudioComponent)
				{
					WaveInstances.RemoveAtSwap(WaveIndex);
				}
			}

			for (int32 WaveIndex = 0; WaveIndex < WaveInstances.Num(); ++WaveIndex)
			{
				TArray<USoundNode*> PathToWaveInstance;
				if (SoundCue->FindPathToNode(WaveInstances[WaveIndex]->WaveInstanceHash, PathToWaveInstance))
				{
					TArray<USoundCueGraphNode_Root*> RootNode;
					TArray<UEdGraphNode*> GraphNodes;
					SoundCueGraph->GetNodesOfClass<USoundCueGraphNode_Root>(RootNode);
					check(RootNode.Num() == 1);
					GraphNodes.Add(RootNode[0]);

					TArray<double> NodeTimes;
					NodeTimes.Add(GCurrentTime); // Time for the root node

					for (int32 i = 0; i < PathToWaveInstance.Num(); ++i)
					{
						const double ObservationTime = GCurrentTime + 1.f;

						NodeTimes.Add(ObservationTime);
						GraphNodes.Add(PathToWaveInstance[i]->GraphNode);
					}

					// Record the unique node->node pairings, keeping only the most recent times for each pairing
					for (int32 i = GraphNodes.Num() - 1; i >= 1; --i)
					{
						UEdGraphNode* CurNode = GraphNodes[i];
						double CurNodeTime = NodeTimes[i];
						UEdGraphNode* NextNode = GraphNodes[i-1];
						double NextNodeTime = NodeTimes[i-1];

						FExecPairingMap& Predecessors = PredecessorNodes.FindOrAdd(NextNode);

						// Update the timings if this is a more recent pairing
						FTimePair& Timings = Predecessors.FindOrAdd(CurNode);
						if (Timings.ThisExecTime < NextNodeTime)
						{
							Timings.PredExecTime = CurNodeTime;
							Timings.ThisExecTime = NextNodeTime;
						}
					}
				}
			}
		}
	}
}

// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
void FSoundCueGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/bool& bBidirectional)
{
	// Get the schema and grab the default color from it
	check(OutputPin);
	check(GraphObj);
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	WireColor = Schema->GetPinTypeColor(OutputPin->PinType);

	if (InputPin == NULL)
	{
		return;
	}
	
	bool bExecuted = false;

	// Run thru the predecessors, and on
	if (FExecPairingMap* PredecessorMap = PredecessorNodes.Find(InputPin->GetOwningNode()))
	{
		if (FTimePair* Times = PredecessorMap->Find(OutputPin->GetOwningNode()))
		{
			bExecuted = true;

			Thickness = ActiveWireThickness;
			WireColor = ActiveColor;

			bDrawBubbles = true;
		}
	}

	if (!bExecuted)
	{
		// It's not followed, fade it and keep it thin
		WireColor = InactiveColor;
		Thickness = InactiveWireThickness;
	}
}

/////////////////////////////////////////////////////
// FMaterialGraphConnectionDrawingPolicy

FMaterialGraphConnectionDrawingPolicy::FMaterialGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, MaterialGraph(CastChecked<UMaterialGraph>(InGraphObj))
	, MaterialGraphSchema(CastChecked<UMaterialGraphSchema>(InGraphObj->GetSchema()))
{
	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;
}

void FMaterialGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool& bBidirectional)
{
	WireColor = MaterialGraphSchema->ActivePinColor;

	// Have to consider both pins as the input will be an 'output' when previewing a connection
	if (OutputPin)
	{
		if (!MaterialGraph->IsInputActive(OutputPin))
		{
			WireColor = MaterialGraphSchema->InactivePinColor;
		}
	}
	if (InputPin)
	{
		if (!MaterialGraph->IsInputActive(InputPin))
		{
			WireColor = MaterialGraphSchema->InactivePinColor;
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Thickness, /*inout*/ WireColor);
	}
}
