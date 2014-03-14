// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FConnectionDrawingPolicy

class GRAPHEDITOR_API FGeometryHelper
{
public:
	static FVector2D VerticalMiddleLeftOf(const FGeometry& SomeGeometry);
	static FVector2D VerticalMiddleRightOf(const FGeometry& SomeGeometry);
	static FVector2D CenterOf(const FGeometry& SomeGeometry);
	static void ConvertToPoints(const FGeometry& Geom, TArray<FVector2D>& Points);

	/** Find the point on line segment from LineStart to LineEnd which is closest to Point */
	static FVector2D FindClosestPointOnLine(const FVector2D& LineStart, const FVector2D& LineEnd, const FVector2D& TestPoint);

	static FVector2D FindClosestPointOnGeom(const FGeometry& Geom, const FVector2D& TestPoint);
};

/////////////////////////////////////////////////////
// FConnectionDrawingPolicy

// This class draws the connections for an UEdGraph composed of pins and nodes
class GRAPHEDITOR_API FConnectionDrawingPolicy
{
protected:
	int32 WireLayerID;
	int32 ArrowLayerID;

	const FSlateBrush* ArrowImage;
	const FSlateBrush* MidpointImage;
	const FSlateBrush* BubbleImage;
public:
	FVector2D ArrowRadius;
	FVector2D MidpointRadius;
protected:
	float ZoomFactor; 
	const FSlateRect& ClippingRect;
	FSlateWindowElementList& DrawElementsList;
	TMap< UEdGraphPin*, TSharedRef<SGraphPin> > PinToPinWidgetMap;
	TSet< UEdGraphPin* > HoveredPins;
	double LastHoverTimeEvent;
public:
	FConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements);

	// Update the drawing policy with the set of hovered pins (which can be empty)
	void SetHoveredPins(const TSet< TWeakObjectPtr<UEdGraphPin> >& InHoveredPins, const TArray< TSharedPtr<SGraphPin> >& OverridePins, double HoverTime);

	// Update the drawing policy with the marked pin (which may not be valid)
	void SetMarkedPin(TWeakPtr<SGraphPin> InMarkedPin);

	static float MakeSplineReparamTable(const FVector2D& P0, const FVector2D& P0Tangent, const FVector2D& P1, const FVector2D& P1Tangent, FInterpCurve<float>& OutReparamTable);

	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional);
	
	virtual void DrawSplineWithArrow(FGeometry& StartGeom, FGeometry& EndGeom, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional);

	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& InColor, float Thickness, bool bDrawBubbles);
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin);

	// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool &bBidirectional);

	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes);

	virtual void DetermineLinkGeometry(
		TMap<TSharedRef<SWidget>,
		FArrangedWidget>& PinGeometries,
		FArrangedChildren& ArrangedNodes, 
		TSharedRef<SWidget>& OutputPinWidget,
		UEdGraphPin* OutputPin,
		UEdGraphPin* InputPin,
		/*out*/ FArrangedWidget*& StartWidgetGeometry,
		/*out*/ FArrangedWidget*& EndWidgetGeometry
		);

	virtual void SetIncompatiblePinDrawState(const TSharedPtr<SGraphPin>& StartPin, const TSet< TSharedRef<SWidget> >& VisiblePins);
	virtual void ResetIncompatiblePinDrawState(const TSet< TSharedRef<SWidget> >& VisiblePins);

	virtual void ApplyHoverDeemphasis(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor);
};

/////////////////////////////////////////////////////
// FKismetConnectionDrawingPolicy

// This class draws the connections for an UEdGraph using a K2-based schema
class FKismetConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	// Times for one execution pair within the current graph
	struct FTimePair
	{
		double PredExecTime;
		double ThisExecTime;

		FTimePair()
			: PredExecTime(0.0)
			, ThisExecTime(0.0)
		{
		}
	};

	// Map of pairings
	typedef TMap<UEdGraphNode*, FTimePair> FExecPairingMap;

	// Map of nodes that preceeded before a given node in the execution sequence (one entry for each pairing)
	TMap<UEdGraphNode*, FExecPairingMap> PredecessorNodes;

	UEdGraph* GraphObj;
	double CurrentTime;
	double LatestTimeDiscovered;

	FLinearColor AttackColor;
	FLinearColor SustainColor;
	FLinearColor ReleaseColor;

	float AttackWireThickness;
	float SustainWireThickness;
	float ReleaseWireThickness;

	float TracePositionBonusPeriod;
	float TracePositionExponent;
	float AttackHoldPeriod;
	float DecayPeriod;
	float DecayExponent;
	float SustainHoldPeriod;
	float ReleasePeriod;
	float ReleaseExponent;

protected:
	// Should this wire be treated as an execution pin, querying the execution trace for connected nodes to draw it differently?
	virtual bool TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const;
public:
	FKismetConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	virtual void BuildExecutionRoadmap();

	virtual bool CanBuildRoadmap() const;

	void CalculateEnvelopeAlphas(double ExecutionTime, /*out*/ float& AttackAlpha, /*out*/ float& SustainAlpha) const;

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool& bBidirectional) OVERRIDE;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) OVERRIDE;
	// End of FConnectionDrawingPolicy interface

	virtual void DetermineStyleOfExecWire(float& Thickness, FLinearColor& WireColor, bool& bDrawBubbles, const FTimePair& Times);

	virtual void SetIncompatiblePinDrawState(const TSharedPtr<SGraphPin>& StartPin, const TSet< TSharedRef<SWidget> >& VisiblePins);
	virtual void ResetIncompatiblePinDrawState(const TSet< TSharedRef<SWidget> >& VisiblePins);
};

/////////////////////////////////////////////////////
// FStateMachineConnectionDrawingPolicy

// This class draws the connections for an UEdGraph with a state machine schema
class FStateMachineConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	UEdGraph* GraphObj;

	TMap<UEdGraphNode*, int32> NodeWidgetMap;
public:
	//
	FStateMachineConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/ bool& bDrawBubbles, /*inout*/ bool& bBidirectional) OVERRIDE;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) OVERRIDE;
	virtual void DetermineLinkGeometry(
		TMap<TSharedRef<SWidget>,
		FArrangedWidget>& PinGeometries,
		FArrangedChildren& ArrangedNodes, 
		TSharedRef<SWidget>& OutputPinWidget,
		UEdGraphPin* OutputPin,
		UEdGraphPin* InputPin,
		/*out*/ FArrangedWidget*& StartWidgetGeometry,
		/*out*/ FArrangedWidget*& EndWidgetGeometry
		) OVERRIDE;
	virtual void DrawSplineWithArrow(FGeometry& StartGeom, FGeometry& EndGeom, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional) OVERRIDE;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles, bool Bidirectional) OVERRIDE;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) OVERRIDE;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& InColor, float Thickness, bool bDrawBubbles) OVERRIDE;
	// End of FConnectionDrawingPolicy interface

protected:
	void Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FLinearColor& WireColor, float WireThickness, bool bDrawBubbles);
};

/////////////////////////////////////////////////////
// FAnimGraphConnectionDrawingPolicy

// This class draws the connections for an UEdGraph with an animation schema
class FAnimGraphConnectionDrawingPolicy : public FKismetConnectionDrawingPolicy
{
public:
	// Constructor
	FAnimGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FKismetConnectionDrawingPolicy interface
	virtual bool TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const OVERRIDE;
	virtual void BuildExecutionRoadmap() OVERRIDE;
	virtual void DetermineStyleOfExecWire(float& Thickness, FLinearColor& WireColor, bool& bDrawBubbles, const FTimePair& Times) OVERRIDE;
	// End of FKismetConnectionDrawingPolicy interface
};

/////////////////////////////////////////////////////
// FSoundCueGraphConnectionDrawingPolicy

// This class draws the connections for an UEdGraph using a SoundCue schema
class FSoundCueGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	// Times for one execution pair within the current graph
	struct FTimePair
	{
		double PredExecTime;
		double ThisExecTime;

		FTimePair()
			: PredExecTime(0.0)
			, ThisExecTime(0.0)
		{
		}
	};

	// Map of pairings
	typedef TMap<UEdGraphNode*, FTimePair> FExecPairingMap;

	// Map of nodes that preceeded before a given node in the execution sequence (one entry for each pairing)
	TMap<UEdGraphNode*, FExecPairingMap> PredecessorNodes;

	UEdGraph* GraphObj;

	FLinearColor ActiveColor;
	FLinearColor InactiveColor;

	float ActiveWireThickness;
	float InactiveWireThickness;

public:
	FSoundCueGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	void BuildAudioFlowRoadmap();

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool& bBidirectional) OVERRIDE;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) OVERRIDE;
	// End of FConnectionDrawingPolicy interface
};

/////////////////////////////////////////////////////
// FMaterialGraphConnectionDrawingPolicy

// This class draws the connections for an UEdGraph using a MaterialGraph schema
class FMaterialGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	UMaterialGraph* MaterialGraph;
	const UMaterialGraphSchema* MaterialGraphSchema;

public:
	FMaterialGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor, /*inout*/bool& bDrawBubbles, /*inout*/ bool& bBidirectional) OVERRIDE;
	// End of FConnectionDrawingPolicy interface
};