#pragma once

#include <string>
#include <vector>

#ifdef UNREALUSDWRAPPER_EXPORTS
#define UNREALUSDWRAPPER_API __declspec(dllexport)
#else
#define UNREALUSDWRAPPER_API //__declspec(dllimport)
#endif

void InitWrapper();


enum EUsdInterpolationMethod
{
	/** Each element in a buffer maps directly to a specific vertex */
	Vertex,
	/** Each element in a buffer maps to a specific face/vertex pair */
	FaceVarying,
	/** Each vertex on a face is the same value */
	Uniform,
};

enum EUsdGeomOrientation
{
	/** Right handed coordinate system */
	RightHanded,
	/** Left handed coordinate system */
	LeftHanded,
};

enum EUsdSubdivisionScheme
{
	None,
	CatmullClark,
	Loop,
	Bilinear,

};

enum EUsdUpAxis
{
	XAxis,
	YAxis,
	ZAxis,
};


namespace UnrealIdentifiers
{
	/**
	 * Identifies the LOD variant set on a primitive which means this primitive has child prims that LOD meshes
	 * named LOD0, LOD1, LOD2, etc
	 */
	static const std::string LOD = "LOD";

	static const std::string AssetPath = "unrealAssetPath";

}

struct FUsdVector2Data
{
	FUsdVector2Data(float InX = 0, float InY = 0)
		: X(InX)
		, Y(InY)
	{}

	float X;
	float Y;
};


struct FUsdVectorData
{
	FUsdVectorData(float InX = 0, float InY = 0, float InZ = 0)
		: X(InX)
		, Y(InY)
		, Z(InZ)
	{}

	float X;
	float Y;
	float Z;
};

struct FUsdUVData
{
	FUsdUVData()
	{}

	/** Defines how UVs are mapped to faces */
	EUsdInterpolationMethod UVInterpMethod;

	/** Raw UVs */
	std::vector<FUsdVector2Data> Coords;
};

struct FUsdQuatData
{
	FUsdQuatData(float InX=0, float InY = 0, float InZ = 0, float InW = 0)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{}

	float X;
	float Y;
	float Z;
	float W;
};

struct FUsdMatrixData
{
	static const int NumRows = 4;
	static const int NumColumns = 4;

	double Data[NumRows*NumColumns];

	double* operator[](int Row)
	{
		return Data + (Row*NumColumns);
	}

	const double* operator[](int Row) const
	{
		return Data + (Row*NumColumns);
	}
};

struct FUsdGeomData
{
	FUsdGeomData()
		: NumUVs(0)
	{}

	/** How many vertices are in each face.  The size of this array tells you how many faces there are */
	std::vector<int> FaceVertexCounts;
	/** Index buffer which matches faces to Points */
	std::vector<int> FaceIndices;
	/** Maps a face to a material index.  MaterialIndex = FaceMaterialIndices[FaceNum] */
	std::vector<int> FaceMaterialIndices;
	/** For subdivision surfaces these are the indices to vertices that have creases */
	std::vector<int> CreaseIndices;
	/** For subdivision surfaces.  Each element gives the number of (must be adjacent) vertices in each crease, whose indices are linearly laid out in the 'CreaseIndices' array. */
	std::vector<int> CreaseLengths;
	/** The per-crease or per-edge sharpness for all creases*/
	std::vector<float> CreaseSharpnesses;
	/** Indices to points that have sharpness */
	std::vector<int> CornerCreaseIndices;
	/** The per-corner sharpness for all corner creases*/
	std::vector<float> CornerSharpnesses;
	/** List of all vertices in the mesh. This just holds the untransformed position of the vertex */
	std::vector<FUsdVectorData> Points;
	/** List of all normals in the mesh.  */
	std::vector<FUsdVectorData> Normals;
	/** List of all vertex colors in the mesh */
	std::vector<FUsdVectorData> VertexColors;
	/** List of all materials in the mesh.  The size of this array represents the number of materials */
	std::vector<std::string> MaterialNames;

	/** Raw UVs.  The size of this array represents how many UV sets there are */
	FUsdUVData UVs[8];

	FUsdUVData SeparateUMap;
	FUsdUVData SeparateVMap;

	/** Orientation of the points */
	EUsdGeomOrientation Orientation;

	EUsdSubdivisionScheme SubdivisionScheme;

	EUsdInterpolationMethod VertexColorInterpMethod;

	int NumUVs;
};

class IUsdPrim
{
public:
	virtual ~IUsdPrim() {}
	virtual const char* GetPrimName() const = 0;
	virtual const char* GetPrimPath() const = 0;
	virtual bool IsGroup() const = 0;
	virtual bool HasTransform() const = 0;
	virtual FUsdMatrixData GetLocalToWorldTransform() const = 0;
	virtual FUsdMatrixData GetLocalToWorldTransform(double Time) const = 0;
	virtual FUsdMatrixData GetLocalToParentTransform() const = 0;
	virtual FUsdMatrixData GetLocalToParentTransform(double Time) const = 0;
	virtual int GetNumChildren() const = 0;
	virtual IUsdPrim* GetChild(int ChildIndex) = 0;
	virtual const char* GetUnrealAssetPath() const = 0;
	virtual bool HasGeometryData() const = 0;
	/** Returns geometry data at the default USD time */
	virtual const FUsdGeomData* GetGeometryData() = 0;

	/** Returns usd geometry data at a given time.  Note that it will reuse internal structures so  */
	virtual const FUsdGeomData* GetGeometryData(double Time) = 0;
	virtual int GetNumLODs() const = 0;
	virtual IUsdPrim* GetLODChild(int LODIndex) = 0;

	// @todo generic attribute, metadata and variant support
};

class IUsdStage
{
public:
	virtual ~IUsdStage() {}
	virtual EUsdUpAxis GetUpAxis() const = 0;
	virtual IUsdPrim* GetRootPrim() = 0;
	virtual bool HasAuthoredTimeCodeRange() const = 0;
	virtual double GetStartTimeCode() const = 0;
	virtual double GetEndTimeCode() const = 0;
	virtual double GetFramesPerSecond() const = 0;
	virtual double GetTimeCodesPerSecond() const = 0;

};

class UNREALUSDWRAPPER_API UnrealUSDWrapper
{
public:
	static void Initialize(const char* PathToModule);
	static IUsdStage* ImportUSDFile(const char* Path, const char* Filename);
	static void CleanUp();
	static double GetDefaultTimeCode();
private:
	static class FUsdStage* CurrentStage;
	static bool bInitialized;
};



