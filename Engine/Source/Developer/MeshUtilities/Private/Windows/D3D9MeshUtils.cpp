// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "../MeshUtilitiesPrivate.h"
#include "RawMesh.h"

#if PLATFORM_WINDOWS && !UE3_LEAN_AND_MEAN

#include "AllowWindowsPlatformTypes.h"
#include "d3d9.h"
#include "D3DX9Mesh.h"
#include "HideWindowsPlatformTypes.h"
#include "D3D9MeshUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogD3D9MeshUtils, Log, All);

#define LOCTEXT_NAMESPACE "D3D9MeshUtils"

#define VERIFYD3D9RESULT(x) x

LRESULT CALLBACK WindowProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		} break;
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
}

bool FD3D9MeshUtilities::IsValid() const
{
	return DummyWindowHandle && Device && Direct3D;
}

FD3D9MeshUtilities::FD3D9MeshUtilities()
	: DummyWindowHandle(0), Direct3D(0), Device(0)
{
	Direct3D = Direct3DCreate9(D3D_SDK_VERSION);

	if(!Direct3D)
	{
		return;
	}

	WNDCLASSEXW wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEXW));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = L"DummyWindowClass";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(0);
//	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;

	RegisterClassExW(&wc);

	DummyWindowHandle = CreateWindowExW(NULL, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = DummyWindowHandle;
	d3dpp.BackBufferWidth = 1;
	d3dpp.BackBufferHeight = 1;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	Direct3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, DummyWindowHandle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device);
}

FD3D9MeshUtilities::~FD3D9MeshUtilities()
{
	if(DummyWindowHandle)
	{
		DestroyWindow(DummyWindowHandle);
	}

	if(Device)
	{
		Device->Release();
	}
	if(Direct3D)
	{
		Direct3D->Release();
	}
}

#define THRESH_UVS_ARE_SAME (1.0f / 1024.0f)

// Temporary vertex for utility class
struct FUtilVertex
{
	FVector   Position;
	FVector2D UVs[8];
	FColor	  Color;
	uint32    SmoothingMask;
	FVector TangentX;
	FVector TangentY;
	FVector TangentZ;
};

/**
 * ConstructD3DVertexElement - acts as a constructor for D3DVERTEXELEMENT9
 */
static D3DVERTEXELEMENT9 ConstructD3DVertexElement(uint16 Stream, uint16 Offset, uint8 Type, uint8 Method, uint8 Usage, uint8 UsageIndex)
{
	D3DVERTEXELEMENT9 newElement;
	newElement.Stream = Stream;
	newElement.Offset = Offset;
	newElement.Type = Type;
	newElement.Method = Method;
	newElement.Usage = Usage;
	newElement.UsageIndex = UsageIndex;
	return newElement;
}

/** Creates a vertex element list for the D3D9MeshUtils vertex format. */
static void GetD3D9MeshVertexDeclarations(TArray<D3DVERTEXELEMENT9>& OutVertexElements)
{
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,Position), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[0]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[1]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[2]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[3]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[4]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[5]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[6]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 6));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,UVs[7]), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 7));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,Color), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,SmoothingMask), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,TangentX), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,TangentY), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0, STRUCT_OFFSET(FUtilVertex,TangentZ), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0));
	OutVertexElements.Push(ConstructD3DVertexElement(0xFF,0,D3DDECLTYPE_UNUSED,0,0,0));
}

/**
	* Creates a D3DXMESH from a FStaticMeshRenderData
	* @param Triangles The triangles to create the mesh from.
	* @param bRemoveDegenerateTriangles True if degenerate triangles should be removed
	* @param OutD3DMesh Mesh to create
	* @return Boolean representing success or failure
*/
bool ConvertRawMeshToD3DXMesh(
	IDirect3DDevice9* Device,
	FRawMesh& RawMesh,
	const bool bRemoveDegenerateTriangles,	
	TRefCountPtr<ID3DXMesh>& OutD3DMesh
	)
{
	TArray<D3DVERTEXELEMENT9> VertexElements;
	GetD3D9MeshVertexDeclarations(VertexElements);

	TArray<FUtilVertex> Vertices;
	TArray<uint16> Indices;
	TArray<uint32> Attributes;
	int32 NumWedges = RawMesh.WedgeIndices.Num();
	int32 NumTriangles = NumWedges / 3;
	int32 NumUVs = 0;
	for (; NumUVs < 8; ++NumUVs)
	{
		if (RawMesh.WedgeTexCoords[NumUVs].Num() != RawMesh.WedgeIndices.Num())
		{
			break;
		}
	}

	bool bHaveColors = RawMesh.WedgeColors.Num() == NumWedges;
	bool bHaveNormals = RawMesh.WedgeTangentZ.Num() == NumWedges;
	bool bHaveTangents = bHaveNormals && RawMesh.WedgeTangentX.Num() == NumWedges
		&& RawMesh.WedgeTangentY.Num() == NumWedges;

	for(int32 TriangleIndex = 0;TriangleIndex < NumTriangles;TriangleIndex++)
	{
		bool bTriangleIsDegenerate = false;

		if( bRemoveDegenerateTriangles )
		{
			// Detect if the triangle is degenerate.
			for(int32 EdgeIndex = 0;EdgeIndex < 3;EdgeIndex++)
			{
				const int32 Wedge0 = TriangleIndex * 3 + EdgeIndex;
				const int32 Wedge1 = TriangleIndex * 3 + ((EdgeIndex + 1) % 3);
				if((RawMesh.GetWedgePosition(Wedge0) - RawMesh.GetWedgePosition(Wedge1)).IsNearlyZero(THRESH_POINTS_ARE_SAME * 4.0f))
				{
					bTriangleIsDegenerate = true;
					break;
				}
			}
		}

		if(!bTriangleIsDegenerate)
		{
			Attributes.Add(RawMesh.FaceMaterialIndices[TriangleIndex]);
			for(int32 J=0;J<3;J++)
			{
				FUtilVertex* Vertex = new(Vertices) FUtilVertex;
				FMemory::Memzero(Vertex,sizeof(FUtilVertex));
				int32 WedgeIndex = TriangleIndex * 3 + J;
				Vertex->Position = RawMesh.GetWedgePosition(WedgeIndex);
				if (bHaveColors)
				{
					Vertex->Color = RawMesh.WedgeColors[WedgeIndex];
				}
				else
				{
					Vertex->Color = FColor::White;
				}
				//store the smoothing mask per vertex since there is only one per-face attribute that is already being used (materialIndex)
				Vertex->SmoothingMask = RawMesh.FaceSmoothingMasks[TriangleIndex];
				if (bHaveTangents)
				{
					Vertex->TangentX = RawMesh.WedgeTangentX[WedgeIndex];
					Vertex->TangentY = RawMesh.WedgeTangentY[WedgeIndex];
				}
				if (bHaveNormals)
				{
					Vertex->TangentZ = RawMesh.WedgeTangentZ[WedgeIndex];
				}
				for(int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
				{
					Vertex->UVs[UVIndex] = RawMesh.WedgeTexCoords[UVIndex][WedgeIndex];
				}
	 
				Indices.Add(Vertices.Num() - 1);
			}
		}
	}

	// This code uses the raw triangles. Needs welding, etc.
	const int32 NumFaces = Indices.Num() / 3;
	const int32 NumVertices = NumFaces*3;

	check(Attributes.Num() == NumFaces);
	check(NumFaces * 3 == Indices.Num());

	// Create mesh for source data
	if (FAILED(D3DXCreateMesh(NumFaces,NumVertices,D3DXMESH_SYSTEMMEM,(D3DVERTEXELEMENT9 *)VertexElements.GetData(),Device,OutD3DMesh.GetInitReference()) ) )
	{
		UE_LOG(LogD3D9MeshUtils, Warning, TEXT("D3DXCreateMesh() Failed!"));
		return false;
	}

	// Fill D3DMesh mesh
	FUtilVertex* D3DVertices;
	uint16*		 D3DIndices;
	::DWORD *		 D3DAttributes;
	OutD3DMesh->LockVertexBuffer(0,(LPVOID*)&D3DVertices);
	OutD3DMesh->LockIndexBuffer(0,(LPVOID*)&D3DIndices);
	OutD3DMesh->LockAttributeBuffer(0, &D3DAttributes);

	FMemory::Memcpy(D3DVertices,Vertices.GetTypedData(),Vertices.Num() * sizeof(FUtilVertex));
	FMemory::Memcpy(D3DIndices,Indices.GetTypedData(),Indices.Num() * sizeof(uint16));
	FMemory::Memcpy(D3DAttributes,Attributes.GetTypedData(),Attributes.Num() * sizeof(uint32));

	OutD3DMesh->UnlockIndexBuffer();
	OutD3DMesh->UnlockVertexBuffer();
	OutD3DMesh->UnlockAttributeBuffer();

	return true;
}

/**
 * Creates a FStaticMeshRenderData from a D3DXMesh
 * @param DestMesh Destination mesh to extract to
 * @param NumUVs Number of UVs
 * @param Elements Elements array
 * @return Boolean representing success or failure
 */
bool ConvertD3DXMeshToRawMesh(
	TRefCountPtr<ID3DXMesh>& D3DMesh, 									  
	FRawMesh& DestMesh, 				  
	int32 NumUVs
	)
{
	// Extract simplified data to LOD
	FUtilVertex* D3DVertices;
	uint16*		 D3DIndices;
	::DWORD *		 D3DAttributes;
	D3DMesh->LockVertexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DVertices);
	D3DMesh->LockIndexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DIndices);
	D3DMesh->LockAttributeBuffer(D3DLOCK_READONLY,&D3DAttributes);

	int32 NumFaces = D3DMesh->GetNumFaces();
	int32 NumWedges = NumFaces * 3;

	DestMesh.FaceMaterialIndices.Init(NumFaces);
	DestMesh.FaceSmoothingMasks.Init(NumFaces);
	DestMesh.VertexPositions.Init(NumWedges);
	DestMesh.WedgeIndices.Init(NumWedges);
	DestMesh.WedgeColors.Init(NumWedges);
	DestMesh.WedgeTangentX.Init(NumWedges);
	DestMesh.WedgeTangentY.Init(NumWedges);
	DestMesh.WedgeTangentZ.Init(NumWedges);
	for (int32 UVIndex = 0; UVIndex < NumUVs; ++UVIndex)
	{
		DestMesh.WedgeTexCoords[UVIndex].Init(NumWedges);
	}

	for(int32 I=0;I<NumFaces;I++)
	{
		// Copy smoothing mask and index from any vertex into this triangle
		DestMesh.FaceSmoothingMasks[I] = D3DVertices[D3DIndices[I*3+0]].SmoothingMask;
		DestMesh.FaceMaterialIndices[I] = D3DAttributes[I];

		for(int UVs=0;UVs<NumUVs;UVs++)
		{
			DestMesh.WedgeTexCoords[UVs][I*3+0] = D3DVertices[D3DIndices[I*3+0]].UVs[UVs];
			DestMesh.WedgeTexCoords[UVs][I*3+1] = D3DVertices[D3DIndices[I*3+1]].UVs[UVs];
			DestMesh.WedgeTexCoords[UVs][I*3+2] = D3DVertices[D3DIndices[I*3+2]].UVs[UVs];
		}

		for(int32 K=0;K<3;K++)
		{
			DestMesh.WedgeIndices[I*3+K] = I*3+K;
			DestMesh.VertexPositions[I*3+K] = D3DVertices[D3DIndices[I*3+K]].Position;
			DestMesh.WedgeColors[I*3+K]   = D3DVertices[D3DIndices[I*3+K]].Color;
			DestMesh.WedgeTangentX[I*3+K] = D3DVertices[D3DIndices[I*3+K]].TangentX;
			DestMesh.WedgeTangentY[I*3+K] = D3DVertices[D3DIndices[I*3+K]].TangentY;
			DestMesh.WedgeTangentZ[I*3+K] = D3DVertices[D3DIndices[I*3+K]].TangentZ;
		}
	}

	D3DMesh->UnlockIndexBuffer();
	D3DMesh->UnlockVertexBuffer();
	D3DMesh->UnlockAttributeBuffer();
	return true;
}

/**
 * Checks whether two points are within a given threshold distance of each other.
 */
template<typename PointType>
bool ArePointsWithinThresholdDistance(const PointType& A,const PointType& B,float InThresholdDistance)
{
	return (A - B).IsNearlyZero(InThresholdDistance);
}

/**
 * An adjacency filter that disallows adjacency between exterior and interior triangles, and between interior triangles that
 * aren't in the same fragment.
 */
class FFragmentedAdjacencyFilter
{
public:

	bool AreEdgesAdjacent(const FUtilVertex& V0,const FUtilVertex& V1,const FUtilVertex& OtherV0,const FUtilVertex& OtherV1) const
	{
		const bool bEdgesMatch =
			ArePointsWithinThresholdDistance(V0.Position,OtherV1.Position,THRESH_POINTS_ARE_SAME * 4.0f) &&
			ArePointsWithinThresholdDistance(V1.Position,OtherV0.Position,THRESH_POINTS_ARE_SAME * 4.0f);
		return bEdgesMatch;
	}
};

/** An adjacency filter that derives adjacency from a UV mapping. */
class FUVChartAdjacencyFilter
{
public:

	/** Initialization constructor. */
	FUVChartAdjacencyFilter(int32 InUVIndex)
	:	UVIndex(InUVIndex)
	{}

	bool AreEdgesAdjacent(const FUtilVertex& V0,const FUtilVertex& V1,const FUtilVertex& OtherV0,const FUtilVertex& OtherV1) const
	{
		const bool bEdgesMatch =
			ArePointsWithinThresholdDistance(V0.UVs[UVIndex],OtherV1.UVs[UVIndex],THRESH_UVS_ARE_SAME) &&
			ArePointsWithinThresholdDistance(V1.UVs[UVIndex],OtherV0.UVs[UVIndex],THRESH_UVS_ARE_SAME);
		return bEdgesMatch;
	}

private:

	int32 UVIndex;
};

// Helper class for generating uv mapping winding info. Use to prevent creating charts from with inconsistent mapping triangle windings.
class FLayoutUVWindingInfo
{
public:
	FLayoutUVWindingInfo(TRefCountPtr<ID3DXMesh> & Mesh, int32 TexCoordIndex)
	{
		D3DMesh = Mesh;
		FUtilVertex* D3DVertices;
		uint16*		 D3DIndices;
		::DWORD *		 D3DAttributes;
		D3DMesh->LockVertexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DVertices);
		D3DMesh->LockIndexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DIndices);
		D3DMesh->LockAttributeBuffer(D3DLOCK_READONLY,&D3DAttributes);

		for(uint32 I=0;I<D3DMesh->GetNumFaces();I++)
		{


			FUtilVertex & vert0 = D3DVertices[D3DIndices[I*3+0]];
			FUtilVertex & vert1 = D3DVertices[D3DIndices[I*3+1]];
			FUtilVertex & vert2 = D3DVertices[D3DIndices[I*3+2]];

			const FVector2D uve1= vert0.UVs[TexCoordIndex] - vert1.UVs[TexCoordIndex];
			const FVector2D uve2= vert0.UVs[TexCoordIndex] - vert2.UVs[TexCoordIndex];

			FVector vec1(uve1 , 0);
			FVector vec2(uve2, 0);	
			vec1.Normalize();
			vec2.Normalize();

			FVector Sidedness = vec1 ^ vec2;
			BOOL Side = false;

			if(Sidedness.Z > 0 )
			{
				Side = true;
			}
			Windings.Push(Side);
		}
		D3DMesh->UnlockVertexBuffer();
		D3DMesh->UnlockIndexBuffer();
		D3DMesh->UnlockAttributeBuffer();

	}

	// check if 2 triangles have same winding
	bool HaveSameWinding(uint32 tr1, uint32 tri2)
	{
		if (Windings[tr1] == Windings[tri2])
		{
			return true;
		}
		else
		{
			return false;
		}
	};

private:
	TRefCountPtr<ID3DXMesh> D3DMesh;
	TArray<BOOL> Windings;
};

/**
 * Generates adjacency for a D3DXMesh
 * @param SourceMesh - The mesh to generate adjacency for.
 * @param OutAdjacency - An array that the adjacency info is written to in the expected D3DX format.
 * @param Filter - A filter that determines which edge pairs are adjacent.
 */
template<typename FilterType>
void GenerateAdjacency(ID3DXMesh* SourceMesh,TArray<uint32>& OutAdjacency,const FilterType& Filter, FLayoutUVWindingInfo * WindingInfo = NULL) 
{
	const uint32 NumTriangles = SourceMesh->GetNumFaces();

	// Initialize the adjacency array.
	OutAdjacency.Empty(NumTriangles * 3);
	for(uint32 AdjacencyIndex = 0;AdjacencyIndex < NumTriangles * 3;AdjacencyIndex++)
	{
		OutAdjacency.Add(INDEX_NONE);
	}

	// Lock the D3DX mesh's vertex and index data.
	FUtilVertex* D3DVertices;
	uint16*		 D3DIndices;
	SourceMesh->LockVertexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DVertices);
	SourceMesh->LockIndexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DIndices);

	for(uint32 TriangleIndex = 0;TriangleIndex < NumTriangles;TriangleIndex++)
	{
		const uint16* TriangleVertexIndices = D3DIndices + TriangleIndex * 3;

		// Find other triangles in the mesh that have a matching edge.
		for(uint32 OtherTriangleIndex = TriangleIndex + 1;OtherTriangleIndex < NumTriangles;OtherTriangleIndex++)
		{
			const uint16* OtherTriangleVertexIndices = D3DIndices + OtherTriangleIndex * 3;

			for(int32 EdgeIndex = 0;EdgeIndex < 3;EdgeIndex++)
			{
				if(OutAdjacency[TriangleIndex * 3 + EdgeIndex] == INDEX_NONE)
				{
					for(int32 OtherEdgeIndex = 0;OtherEdgeIndex < 3;OtherEdgeIndex++)
					{
						if(OutAdjacency[OtherTriangleIndex * 3 + OtherEdgeIndex] == INDEX_NONE)
						{
							const FUtilVertex& V0 = D3DVertices[TriangleVertexIndices[EdgeIndex]];
							const FUtilVertex& V1 = D3DVertices[TriangleVertexIndices[(EdgeIndex + 1) % 3]];
							const FUtilVertex& OtherV0 = D3DVertices[OtherTriangleVertexIndices[OtherEdgeIndex]];
							const FUtilVertex& OtherV1 = D3DVertices[OtherTriangleVertexIndices[(OtherEdgeIndex + 1) % 3]];
							// added check when separating  mirrored, overlapped chunks for "Layout using 0 chunks" mode
							if(Filter.AreEdgesAdjacent(V0,V1,OtherV0,OtherV1) && (WindingInfo ? WindingInfo->HaveSameWinding(TriangleIndex, OtherTriangleIndex) : true)) 
							{
								OutAdjacency[TriangleIndex * 3 + EdgeIndex] = OtherTriangleIndex;
								OutAdjacency[OtherTriangleIndex * 3 + OtherEdgeIndex] = TriangleIndex;
								break;
							}
						}
					}
				}
			}
		}
	}

	// Unlock the D3DX mesh's vertex and index data.
	SourceMesh->UnlockVertexBuffer();
	SourceMesh->UnlockIndexBuffer();
}

/** Merges a set of D3DXMeshes. */
static void MergeD3DXMeshes(
	IDirect3DDevice9* Device,
	TRefCountPtr<ID3DXMesh>& OutMesh,TArray<int32>& OutBaseFaceIndex,const TArray<ID3DXMesh*>& Meshes)
{
	TArray<D3DVERTEXELEMENT9> VertexElements;
	GetD3D9MeshVertexDeclarations(VertexElements);
		
	// Count the number of faces and vertices in the input meshes.
	int32 NumFaces = 0;
	int32 NumVertices = 0;
	for(int32 MeshIndex = 0;MeshIndex < Meshes.Num();MeshIndex++)
	{
		NumFaces += Meshes[MeshIndex]->GetNumFaces();
		NumVertices += Meshes[MeshIndex]->GetNumVertices();
	}

	// Create mesh for source data
	VERIFYD3D9RESULT(D3DXCreateMesh(
		NumFaces,
		NumVertices,
		D3DXMESH_SYSTEMMEM,
		(D3DVERTEXELEMENT9*)VertexElements.GetData(),
		Device,
		OutMesh.GetInitReference()
		) );

	// Fill D3DXMesh
	FUtilVertex* ResultVertices;
	uint16*		 ResultIndices;
	::DWORD *		 ResultAttributes;
	OutMesh->LockVertexBuffer(0,(LPVOID*)&ResultVertices);
	OutMesh->LockIndexBuffer(0,(LPVOID*)&ResultIndices);
	OutMesh->LockAttributeBuffer(0, &ResultAttributes);

	int32 BaseVertexIndex = 0;
	int32 BaseFaceIndex = 0;
	for(int32 MeshIndex = 0;MeshIndex < Meshes.Num();MeshIndex++)
	{
		ID3DXMesh* Mesh = Meshes[MeshIndex];
				
		FUtilVertex* Vertices;
		uint16*		 Indices;
		::DWORD *		 Attributes;
		Mesh->LockVertexBuffer(0,(LPVOID*)&Vertices);
		Mesh->LockIndexBuffer(0,(LPVOID*)&Indices);
		Mesh->LockAttributeBuffer(0, &Attributes);

		for(uint32 FaceIndex = 0;FaceIndex < Mesh->GetNumFaces();FaceIndex++)
		{
			for(uint32 VertexIndex = 0;VertexIndex < 3;VertexIndex++)
			{
				*ResultIndices++ = BaseVertexIndex + *Indices++;
			}
		}
		OutBaseFaceIndex.Add(BaseFaceIndex);
		BaseFaceIndex += Mesh->GetNumFaces();

		FMemory::Memcpy(ResultVertices,Vertices,Mesh->GetNumVertices() * sizeof(FUtilVertex));
		ResultVertices += Mesh->GetNumVertices();
		BaseVertexIndex += Mesh->GetNumVertices();

		FMemory::Memcpy(ResultAttributes,Attributes,Mesh->GetNumFaces() * sizeof(uint32));
		ResultAttributes += Mesh->GetNumFaces();

		Mesh->UnlockIndexBuffer();
		Mesh->UnlockVertexBuffer();
		Mesh->UnlockAttributeBuffer();
	}

	OutMesh->UnlockIndexBuffer();
	OutMesh->UnlockVertexBuffer();
	OutMesh->UnlockAttributeBuffer();
}

/**
 * Assigns a group index to each triangle such that it's the same group as its adjacent triangles.
 * The group indices are between zero and the minimum number of indices needed.
 * @return The number of groups used.
 */
static uint32 AssignMinimalAdjacencyGroups(const TArray<uint32>& Adjacency,TArray<int32>& OutTriangleGroups)
{
	const uint32 NumTriangles = Adjacency.Num() / 3;
	check(Adjacency.Num() == NumTriangles * 3);

	// Initialize the triangle group array.
	OutTriangleGroups.Empty(NumTriangles);
	for(uint32 TriangleIndex = 0;TriangleIndex < NumTriangles;TriangleIndex++)
	{
		OutTriangleGroups.Add(INDEX_NONE);
	}

	uint32 NumGroups = 0;
	while(true)
	{
		const uint32 CurrentGroupIndex = NumGroups;
		TArray<uint32> PendingGroupTriangles;

		// Find the next ungrouped triangle to start the group with.
		for(uint32 TriangleIndex = 0;TriangleIndex < NumTriangles;TriangleIndex++)
		{
			if(OutTriangleGroups[TriangleIndex] == INDEX_NONE)
			{
				PendingGroupTriangles.Push(TriangleIndex);
				break;
			}
		}

		if(!PendingGroupTriangles.Num())
		{
			break;
		}
		else
		{
			// Recursively expand the group to include all triangles adjacent to the group's triangles.
			while(PendingGroupTriangles.Num())
			{
				const uint32 TriangleIndex = PendingGroupTriangles.Pop();

				OutTriangleGroups[TriangleIndex] = CurrentGroupIndex;

				for(int32 EdgeIndex = 0;EdgeIndex < 3;EdgeIndex++)
				{
					const int32 AdjacentTriangleIndex = Adjacency[TriangleIndex * 3 + EdgeIndex];
					if(AdjacentTriangleIndex != INDEX_NONE)
					{
						const int32 AdjacentTriangleGroupIndex = OutTriangleGroups[AdjacentTriangleIndex];
						check(AdjacentTriangleGroupIndex == INDEX_NONE || AdjacentTriangleGroupIndex == CurrentGroupIndex);
						if(AdjacentTriangleGroupIndex == INDEX_NONE)
						{
							PendingGroupTriangles.Push(AdjacentTriangleIndex);
						}
					}
				}
			};

			NumGroups++;
		}
	};

	return NumGroups;
}

/**
 * Called during unique UV set generation to allow us to update status in the GUI
 *
 * @param	InPercentDone	Scalar (0-1) of percent currently complete
 * @param	InUserData		User data pointer
 */
static HRESULT __stdcall GenerateUVsStatusCallback( float InPercentDone, LPVOID InUserData )
{
	GWarn->UpdateProgress( InPercentDone * 100, 100 );

	// NOTE: Returning anything other than S_OK will abort the operation
	return S_OK;
}

bool FD3D9MeshUtilities::LayoutUVs(
	struct FRawMesh& RawMesh,
	uint32 TextureResolution,
	uint32 TexCoordIndex,
	FText& OutError
	)
{
	OutError = FText();
	if(!IsValid() || !RawMesh.IsValid())
	{
		OutError = LOCTEXT("LayoutUVs_FailedInvalid", "LayoutUVs failed, mesh was invalid.");
		return false;
	}

	int32 NumTexCoords = 0;
	for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
	{
		if (RawMesh.WedgeTexCoords[i].Num() != RawMesh.WedgeIndices.Num())
		{
			break;
		}
		NumTexCoords++;
	}

	if (TexCoordIndex > (uint32)NumTexCoords)
	{
		OutError = LOCTEXT("LayoutUVs_FailedUVs", "LayoutUVs failed, incorrect number of texcoords.");
		return false;
	}

	// Sort the mesh's triangles by whether they need to be charted, or just to be packed into the atlas.
	FRawMesh MeshToAtlas = RawMesh;
	if (TexCoordIndex > 0)
	{
		MeshToAtlas.WedgeTexCoords[TexCoordIndex] = MeshToAtlas.WedgeTexCoords[0];
	}

	TRefCountPtr<ID3DXMesh> ChartMesh;
	TArray<uint32> AtlasAndChartAdjacency;
	TArray<int32> AtlasAndChartTriangleCharts;
	TRefCountPtr<ID3DXMesh> MergedMesh;
	TArray<uint32> MergedAdjacency;
	TArray<int32> MergedTriangleCharts;
	TRefCountPtr<ID3DXMesh> AtlasOnlyMesh;
	TArray<uint32> AtlasOnlyAdjacency;
	TArray<int32> AtlasOnlyTriangleCharts;

	{
		// Create a D3DXMesh for the triangles that only need to be atlassed.
		const bool bRemoveDegenerateTriangles = true;
		if (!ConvertRawMeshToD3DXMesh(Device,MeshToAtlas,bRemoveDegenerateTriangles,AtlasOnlyMesh))
		{
			OutError = LOCTEXT("LayoutUVs_FailedConvert", "LayoutUVs failed, couldn't convert to a D3DXMesh.");
			return false;
		}
		// generate mapping orientations info 
		FLayoutUVWindingInfo WindingInfo(AtlasOnlyMesh, TexCoordIndex);
		// Generate adjacency for the pre-charted triangles based on their input charts.
		GenerateAdjacency(AtlasOnlyMesh,AtlasOnlyAdjacency,FUVChartAdjacencyFilter(TexCoordIndex), &WindingInfo);


		////clean the mesh
		TRefCountPtr<ID3DXMesh> TempMesh;
		TArray<uint32> CleanedAdjacency;
		CleanedAdjacency.AddUninitialized(AtlasOnlyMesh->GetNumFaces() * 3);
		if( FAILED(D3DXCleanMesh( D3DXCLEAN_SIMPLIFICATION, 
			AtlasOnlyMesh, 
			(::DWORD *)AtlasOnlyAdjacency.GetTypedData(), 
			TempMesh.GetInitReference(), 
			(::DWORD *)CleanedAdjacency.GetTypedData(), 
			NULL ) ) )
		{
			OutError = LOCTEXT("LayoutUVs_FailedClean", "LayoutUVs failed, couldn't clean mesh.");
			return false;
		}

		// Group the pre-charted triangles into indexed charts based on their adjacency in the chart.
		AssignMinimalAdjacencyGroups(CleanedAdjacency,AtlasOnlyTriangleCharts);

		MergedMesh = TempMesh;
		MergedAdjacency = CleanedAdjacency;
		MergedTriangleCharts = AtlasOnlyTriangleCharts;
	}

	if(MergedMesh)
	{
		// Create a buffer to hold the triangle chart data.
		TRefCountPtr<ID3DXBuffer> MergedTriangleChartsBuffer;
		VERIFYD3D9RESULT(D3DXCreateBuffer(
			MergedTriangleCharts.Num() * sizeof(int32),
			MergedTriangleChartsBuffer.GetInitReference()
			));
		uint32* MergedTriangleChartsBufferPointer = (uint32*)MergedTriangleChartsBuffer->GetBufferPointer();
		for(int32 TriangleIndex = 0;TriangleIndex < MergedTriangleCharts.Num();TriangleIndex++)
		{
			*MergedTriangleChartsBufferPointer++ = MergedTriangleCharts[TriangleIndex];
		}
		const float GutterSize = 2.0f;
		// Pack the charts into a unified atlas.
		HRESULT Result = D3DXUVAtlasPack(
			MergedMesh,
			TextureResolution,
			TextureResolution,
			GutterSize,
			TexCoordIndex,
			(::DWORD *)MergedAdjacency.GetTypedData(),
			NULL,
			0,
			NULL,
			0,
			MergedTriangleChartsBuffer
			);
		if (FAILED(Result))
		{
			UE_LOG(LogD3D9MeshUtils, Warning, 
				TEXT("D3DXUVAtlasPack() returned %u."),
				Result
				);
			OutError = LOCTEXT("LayoutUVs_FailedPack", "LayoutUVs failed, D3DXUVAtlasPack failed.");
			return false;
		}

		int32 NewNumTexCoords = FMath::Max<int32>(NumTexCoords, TexCoordIndex + 1);
		FRawMesh FinalMesh;
		if (!ConvertD3DXMeshToRawMesh(MergedMesh, FinalMesh, NewNumTexCoords))
		{
			OutError = LOCTEXT("LayoutUVs_FailedSimple", "LayoutUVs failed, couldn't convert the simplified D3DXMesh back to a UStaticMesh.");
			return false;
		}
		RawMesh = FinalMesh;
	}
	return true;
}

bool FD3D9MeshUtilities::GenerateUVs(
	struct FRawMesh& RawMesh,
	uint32 TexCoordIndex,
	float MinChartSpacingPercent,
	float BorderSpacingPercent,
	bool bUseMaxStretch,
	const TArray< int32 >* InFalseEdgeIndices,
	uint32& MaxCharts,
	float& MaxDesiredStretch,
	FText& OutError
	)
{
	OutError = FText();
	if(!IsValid())
	{
		OutError = LOCTEXT("GenerateUVs_FailedInvalid", "GenerateUVs failed, mesh was invalid.");
		return false;
	}

	int32 NumTexCoords = 0;
	for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
	{
		if (RawMesh.WedgeTexCoords[i].Num() != RawMesh.WedgeIndices.Num())
		{
			break;
		}
		NumTexCoords++;
	}

	if (TexCoordIndex > (uint32)NumTexCoords)
	{
		OutError = LOCTEXT("GenerateUVs_FailedUVs", "GenerateUVs failed, incorrect number of texcoords.");
		return false;
	}

	TRefCountPtr<ID3DXMesh> ChartMesh;
	TArray<uint32> AtlasAndChartAdjacency;
	TArray<int32> AtlasAndChartTriangleCharts;
	{
		const bool bUseFalseEdges = InFalseEdgeIndices != NULL;

		// When using false edges we don't remove degenerates as we want our incoming selected edge list to map
		// correctly to the D3DXMesh.
		const bool bRemoveDegenerateTriangles = !bUseFalseEdges;

		// Create a D3DXMesh for the triangles being charted.
		TRefCountPtr<ID3DXMesh> SourceMesh;
		if (!ConvertRawMeshToD3DXMesh(Device, RawMesh,bRemoveDegenerateTriangles,SourceMesh))
		{
			OutError = LOCTEXT("GenerateUVs_FailedConvert", "GenerateUVs failed, couldn't convert to a D3DXMesh.");
			return false;
		}

		//generate adjacency info for the mesh, which is needed later
		TArray<uint32> Adjacency;
		GenerateAdjacency(SourceMesh,Adjacency,FFragmentedAdjacencyFilter());

		// We don't clean the mesh as this can collapse vertices or delete degenerate triangles, and
		// we want our incoming selected edge list to map correctly to the D3DXMesh.
		if( !bUseFalseEdges )
		{
			//clean the mesh
			TRefCountPtr<ID3DXMesh> TempMesh;
			TArray<uint32> CleanedAdjacency;
			CleanedAdjacency.AddUninitialized(SourceMesh->GetNumFaces() * 3);
			if( FAILED(D3DXCleanMesh( D3DXCLEAN_SIMPLIFICATION, SourceMesh, (::DWORD *)Adjacency.GetTypedData(), TempMesh.GetInitReference(), 
				(::DWORD *)CleanedAdjacency.GetTypedData(), NULL ) ) )
			{
				OutError = LOCTEXT("GenerateUVs_FailedClean", "GenerateUVs failed, couldn't clean mesh.");
				return false;
			}
			SourceMesh = TempMesh;
			Adjacency = CleanedAdjacency;
		}


		// Setup the D3DX "false edge" array.  This is three DWORDS per face that define properties of the
		// face's edges.  Values of -1 indicates that the edge may be used as a UV seam in a the chart.  Any
		// other value indicates that the edge should never be a UV seam.  This essentially allows us to
		// provide a precise list of edges to be used as UV seams in the new charts.
		uint32* FalseEdgeArray = NULL;
		TArray<uint32> FalseEdges;
		if( bUseFalseEdges )
		{
			// -1 means "always use this edge as a chart UV seam" to D3DX
 			FalseEdges.AddUninitialized( SourceMesh->GetNumFaces() * 3 );
			for( int32 CurFalseEdgeIndex = 0; CurFalseEdgeIndex < (int32)SourceMesh->GetNumFaces() * 3; ++CurFalseEdgeIndex )
			{
				FalseEdges[ CurFalseEdgeIndex ] = -1;
			}

			// For each tagged edge
			for( int32 CurTaggedEdgeIndex = 0; CurTaggedEdgeIndex < InFalseEdgeIndices->Num(); ++CurTaggedEdgeIndex )
			{
				const int32 EdgeIndex = ( *InFalseEdgeIndices )[ CurTaggedEdgeIndex ];
					
				// Mark this as a false edge by setting it to a value other than negative one
				FalseEdges[ EdgeIndex ] = Adjacency[ CurTaggedEdgeIndex ];
			}

			FalseEdgeArray = (uint32*)FalseEdges.GetTypedData();
		}

			
		// Partition the mesh's triangles into charts.
		TRefCountPtr<ID3DXBuffer> PartitionResultAdjacencyBuffer;
		TRefCountPtr<ID3DXBuffer> FacePartitionBuffer;
		HRESULT Result = D3DXUVAtlasPartition(
			SourceMesh,
			bUseMaxStretch ? 0 : MaxCharts,				// Max charts (0 = use max stretch instead)
			MaxDesiredStretch,
			TexCoordIndex,
			(::DWORD *)Adjacency.GetTypedData(),
			(::DWORD *)FalseEdgeArray,	// False edges
			NULL,		// IMT data
			&GenerateUVsStatusCallback,
			0.01f,		// Callback frequency
			NULL,			// Callback user data
			D3DXUVATLAS_GEODESIC_QUALITY,
			ChartMesh.GetInitReference(),
			FacePartitionBuffer.GetInitReference(),
			NULL,
			PartitionResultAdjacencyBuffer.GetInitReference(),
			&MaxDesiredStretch,
			&MaxCharts
			);
		if (FAILED(Result))
		{
			UE_LOG(LogD3D9MeshUtils, Warning, 
				TEXT("D3DXUVAtlasPartition() returned %u with MaxDesiredStretch=%.2f, TexCoordIndex=%u."),
				Result,
				MaxDesiredStretch,
				TexCoordIndex
				);
			OutError = LOCTEXT("GenerateUVs_FailedPartition", "GenerateUVs failed, D3DXUVAtlasPartition failed.");
			return false;
		}

		// Extract the chart adjacency data from the D3DX buffer into an array.
		for(uint32 TriangleIndex = 0;TriangleIndex < ChartMesh->GetNumFaces();TriangleIndex++)
		{
			for(int32 EdgeIndex = 0;EdgeIndex < 3;EdgeIndex++)
			{
				AtlasAndChartAdjacency.Add(*((uint32*)PartitionResultAdjacencyBuffer->GetBufferPointer()+TriangleIndex*3+EdgeIndex));
			}
		}

		// Extract the triangle chart data from the D3DX buffer into an array.
		uint32* FacePartitionBufferPointer = (uint32*)FacePartitionBuffer->GetBufferPointer();
		for(uint32 TriangleIndex = 0;TriangleIndex < ChartMesh->GetNumFaces();TriangleIndex++)
		{
			AtlasAndChartTriangleCharts.Add(*FacePartitionBufferPointer++);
		}

		// Scale the partitioned UVs down.
		FUtilVertex* LockedVertices;
		ChartMesh->LockVertexBuffer(0,(LPVOID*)&LockedVertices);
		for(uint32 VertexIndex = 0;VertexIndex < ChartMesh->GetNumVertices();VertexIndex++)
		{
			LockedVertices[VertexIndex].UVs[TexCoordIndex] /= 2048.0f;
		}
		ChartMesh->UnlockVertexBuffer();
	}

	if(ChartMesh)
	{
		// Create a buffer to hold the triangle chart data.
		TRefCountPtr<ID3DXBuffer> MergedTriangleChartsBuffer;
		VERIFYD3D9RESULT(D3DXCreateBuffer(
			AtlasAndChartTriangleCharts.Num() * sizeof(int32),
			MergedTriangleChartsBuffer.GetInitReference()
			));
		uint32* MergedTriangleChartsBufferPointer = (uint32*)MergedTriangleChartsBuffer->GetBufferPointer();
		for(int32 TriangleIndex = 0;TriangleIndex < AtlasAndChartTriangleCharts.Num();TriangleIndex++)
		{
			*MergedTriangleChartsBufferPointer++ = AtlasAndChartTriangleCharts[TriangleIndex];
		}

		const uint32 FakeTexSize = 1024;
		const float GutterSize = ( float )FakeTexSize * MinChartSpacingPercent * 0.01f;

		// Pack the charts into a unified atlas.
		HRESULT Result = D3DXUVAtlasPack(
			ChartMesh,
			FakeTexSize,
			FakeTexSize,
			GutterSize,
			TexCoordIndex,
			(::DWORD *)AtlasAndChartAdjacency.GetTypedData(),
			&GenerateUVsStatusCallback,
			0.01f,		// Callback frequency
			NULL,
			0,
			MergedTriangleChartsBuffer
			);
		if (FAILED(Result))
		{
			UE_LOG(LogD3D9MeshUtils, Warning, 
				TEXT("D3DXUVAtlasPack() returned %u."),
				Result
				);
			OutError = LOCTEXT("GenerateUVs_FailedPack", "GenerateUVs failed, D3DXUVAtlasPack failed.");
			return false;
		}

		int32 NewNumTexCoords = FMath::Max<int32>(NumTexCoords, TexCoordIndex + 1);
		FRawMesh FinalMesh;

		if (!ConvertD3DXMeshToRawMesh(ChartMesh, FinalMesh, NewNumTexCoords))
		{
			OutError = LOCTEXT("GenerateUVs_FailedSimple", "GenerateUVs failed, couldn't convert the simplified D3DXMesh back to a UStaticMesh.");
			return false;
		}

		// Scale/offset the UVs appropriately to ensure there is empty space around the border
		{
			const float BorderSize = BorderSpacingPercent * 0.01f;
			const float ScaleAmount = 1.0f - BorderSize * 2.0f;

			for( int32 CurUVIndex = 0; CurUVIndex < MAX_MESH_TEXTURE_COORDS; ++CurUVIndex )
			{
				int32 NumWedges = FinalMesh.WedgeTexCoords[CurUVIndex].Num();
				for( int32 WedgeIndex = 0; WedgeIndex < NumWedges; ++WedgeIndex )
				{
					FVector2D& UV = FinalMesh.WedgeTexCoords[CurUVIndex][WedgeIndex];
					UV.X = BorderSize + UV.X * ScaleAmount;
					UV.Y = BorderSize + UV.Y * ScaleAmount;
				}
			}
		}
		RawMesh = FinalMesh;
	}
	return true; 
}

#undef LOCTEXT_NAMESPACE

#endif

