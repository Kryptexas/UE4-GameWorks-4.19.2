// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013-2016 NVIDIA Corporation. All rights reserved.

#ifndef FLEX_H
#define FLEX_H

//! \cond HIDDEN_SYMBOLS
#if _WIN32
#define FLEX_API __declspec(dllexport)
#else
#define FLEX_API
#endif

// least 2 significant digits define minor version, eg: 10 -> version 0.10
#define FLEX_VERSION 110

//! \endcond

/** \file flex.h 
 * The main include file for the core Flex solver.
 */

extern "C" {

/**
 * Opaque type representing a library that can create FlexSolvers, FlexTriangleMeshes, and FlexBuffers
 */
typedef struct FlexLibrary FlexLibrary;

/**
 * Opaque type representing a collection of particles and constraints
 */
typedef struct FlexSolver FlexSolver;

/**
 * Opaque type representing a data buffer, type and contents depends on usage, see flexAllocBuffer()
 */
typedef struct FlexBuffer FlexBuffer;

/**
 * Controls behavior of flexMap()
 */
enum FlexMapFlags
{
   	eFlexMapWait = 0,		//!< Calling thread will be blocked until buffer is ready for access, default
   	eFlexMapDoNotWait = 1,	//!< Calling thread will check if buffer is ready for access, if not ready then the method will return NULL immediately
	eFlexMapDiscard = 2		//!< Buffer contents will be discarded, this allows for efficent buffer reuse
};

/**
 * Controls memory space of a FlexBuffer, see flexAllocBuffer()
 */
enum FlexBufferType
{
   	eFlexBufferHost		= 0,	//!< Host mappable buffer, pinned memory on CUDA, staging buffer on DX
   	eFlexBufferDevice	= 1,	//!< Device memory buffer, mapping this on CUDA will return a device memory pointer, and will return a buffer pointer on DX
};

/**
 * Controls the relaxation method used by the solver to ensure convergence
 */
enum FlexRelaxationMode
{
	eFlexRelaxationGlobal = 0,	//!< The relaxation factor is a fixed multiplier on each constraint's position delta
	eFlexRelaxationLocal  = 1	//!< The relaxation factor is a fixed multiplier on each constraint's delta divided by the particle's constraint count, convergence will be slower but more reliable
};


/**
 * Simulation parameters for a solver
 */
struct FlexParams
{
	int mNumIterations;					//!< Number of solver iterations to perform per-substep

	float mGravity[3];					//!< Constant acceleration applied to all particles
	float mRadius;						//!< The maximum interaction radius for particles
	float mSolidRestDistance;			//!< The distance non-fluid particles attempt to maintain from each other, must be in the range (0, radius]
	float mFluidRestDistance;			//!< The distance fluid particles are spaced at the rest density, must be in the range (0, radius], for fluids this should generally be 50-70% of mRadius, for rigids this can simply be the same as the particle radius

	// common params
	float mDynamicFriction;				//!< Coefficient of friction used when colliding against shapes
	float mStaticFriction;				//!< Coefficient of static friction used when colliding against shapes
	float mParticleFriction;			//!< Coefficient of friction used when colliding particles
	float mRestitution;					//!< Coefficient of restitution used when colliding against shapes, particle collisions are always inelastic
	float mAdhesion;					//!< Controls how strongly particles stick to surfaces they hit, default 0.0, range [0.0, +inf]
	float mSleepThreshold;				//!< Particles with a velocity magnitude < this threshold will be considered fixed
	
	float mMaxSpeed;					//!< The magnitude of particle velocity will be clamped to this value at the end of each step
	float mMaxAcceleration;				//!< The magnitude of particle acceleration will be clamped to this value at the end of each step (limits max velocity change per-second), useful to avoid popping due to large interpenetrations
	
	float mShockPropagation;			//!< Artificially decrease the mass of particles based on height from a fixed reference point, this makes stacks and piles converge faster
	float mDissipation;					//!< Damps particle velocity based on how many particle contacts it has
	float mDamping;						//!< Viscous drag force, applies a force proportional, and opposite to the particle velocity

	// cloth params
	float mWind[3];						//!< Constant acceleration applied to particles that belong to dynamic triangles, drag needs to be > 0 for wind to affect triangles
	float mDrag;						//!< Drag force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the negative velocity direction
	float mLift;						//!< Lift force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the direction perpendicular to velocity and (if possible), parallel to the plane normal

	// fluid params
	bool mFluid;						//!< If true then particles with phase 0 are considered fluid particles and interact using the position based fluids method
	float mCohesion;					//!< Control how strongly particles hold each other together, default: 0.025, range [0.0, +inf]
	float mSurfaceTension;				//!< Controls how strongly particles attempt to minimize surface area, default: 0.0, range: [0.0, +inf]    
	float mViscosity;					//!< Smoothes particle velocities using XSPH viscosity
	float mVorticityConfinement;		//!< Increases vorticity by applying rotational forces to particles
	float mAnisotropyScale;				//!< Control how much anisotropy is present in resulting ellipsoids for rendering, if zero then anisotropy will not be calculated, see flexGetAnisotropy()
	float mAnisotropyMin;				//!< Clamp the anisotropy scale to this fraction of the radius
	float mAnisotropyMax;				//!< Clamp the anisotropy scale to this fraction of the radius
	float mSmoothing;					//!< Control the strength of Laplacian smoothing in particles for rendering, if zero then smoothed positions will not be calculated, see flexGetSmoothParticles()
	float mSolidPressure;				//!< Add pressure from solid surfaces to particles
	float mFreeSurfaceDrag;				//!< Drag force applied to boundary fluid particles
	float mBuoyancy;					//!< Gravity is scaled by this value for fluid particles

	// diffuse params
	float mDiffuseThreshold;			//!< Particles with kinetic energy + divergence above this threshold will spawn new diffuse particles
	float mDiffuseBuoyancy;				//!< Scales force opposing gravity that diffuse particles receive
	float mDiffuseDrag;					//!< Scales force diffuse particles receive in direction of neighbor fluid particles
	int mDiffuseBallistic;				//!< The number of neighbors below which a diffuse particle is considered ballistic
	float mDiffuseSortAxis[3];			//!< Diffuse particles will be sorted by depth along this axis if non-zero
	float mDiffuseLifetime;				//!< Time in seconds that a diffuse particle will live for after being spawned, particles will be spawned with a random lifetime in the range [0, mDiffuseLifetime]

	// rigid params
	float mPlasticThreshold;			//!< Particles belonging to rigid shapes that move with a position delta magnitude > threshold will be permanently deformed in the rest pose
	float mPlasticCreep;				//!< Controls the rate at which particles in the rest pose are deformed for particles passing the deformation threshold 

	// collision params
	float mCollisionDistance;			//!< Distance particles maintain against shapes, note that for robust collision against triangle meshes this distance should be greater than zero
	float mParticleCollisionMargin;		//!< Increases the radius used during neighbor finding, this is useful if particles are expected to move significantly during a single step to ensure contacts aren't missed on subsequent iterations
	float mShapeCollisionMargin;		//!< Increases the radius used during contact finding against kinematic shapes

	float mPlanes[8][4];				//!< Collision planes in the form ax + by + cz + d = 0
	int mNumPlanes;						//!< Num collision planes

	FlexRelaxationMode mRelaxationMode;	//!< How the relaxation is applied inside the solver
	float mRelaxationFactor;			//!< Control the convergence rate of the parallel solver, default: 1, values greater than 1 may lead to instability
};

/**
 * Flags that control the a particle's behavior and grouping, use flexMakePhase() to construct a valid 32bit phase identifier
 */
enum FlexPhase
{
	eFlexPhaseGroupMask			= 0x00ffffff,	//!< Low 24 bits represent the particle group for controlling collisions	

	eFlexPhaseSelfCollide		= 1 << 24,		//!< If set this particle will interact with particles of the same group
	eFlexPhaseSelfCollideFilter = 1 << 25,		//!< If set this particle will ignore collisions with particles closer than the radius in the rest pose, this flag should not be specified unless valid rest positions have been specified using flexSetRestParticles()
	eFlexPhaseFluid				= 1 << 26,		//!< If set this particle will generate fluid density constraints for its overlapping neighbors
};

/**
 * Generate a bit set for the particle phase, the group should be an integer < 2^24, and the flags should be a combination of FlexPhase enum values
 */
FLEX_API inline int flexMakePhase(int group, int flags) { return (group & eFlexPhaseGroupMask) | flags; }


/**
 * Time spent in each section of the solver update, times in GPU seconds, see flexUpdateSolver()
 */
struct FlexTimers
{
	float mPredict;				//!< Time spent in prediction
	float mCreateCellIndices;	//!< Time spent creating grid indices
	float mSortCellIndices;		//!< Time spent sorting grid indices
	float mCreateGrid;			//!< Time spent creating grid
	float mReorder;				//!< Time spent reordering particles
	float mCollideParticles;	//!< Time spent finding particle neighbors
	float mCollideShapes;		//!< Time spent colliding convex shapes
	float mCollideTriangles;	//!< Time spent colliding triangle shapes
	float mCollideFields;		//!< Time spent colliding signed distance field shapes
	float mCalculateDensity;	//!< Time spent calculating fluid density
	float mSolveDensities;		//!< Time spent solving density constraints
	float mSolveVelocities;		//!< Time spent solving velocity constraints
	float mSolveShapes;			//!< Time spent solving rigid body constraints
	float mSolveSprings;		//!< Time spent solving distance constraints
	float mSolveContacts;		//!< Time spent solving contact constraints
	float mSolveInflatables;	//!< Time spent solving pressure constraints
	float mApplyDeltas;	        //!< Time spent adding position deltas to particles
	float mCalculateAnisotropy;	//!< Time spent calculating particle anisotropy for fluid
	float mUpdateDiffuse;		//!< Time spent updating diffuse particles
	float mUpdateTriangles;		//!< Time spent updating dynamic triangles
	float mUpdateNormals;		//!< Time spent updating vertex normals
	float mFinalize;			//!< Time spent finalizing state
	float mUpdateBounds;		//!< Time spent updating particle bounds
	float mTotal;				//!< Sum of all timers above
};

/**
 * Flex error return codes
 */
enum FlexErrorSeverity
{
	eFlexLogError	=  0,	//!< Error messages
	eFlexLogInfo	=  1,	//!< Information messages
	eFlexLogWarning	=  2,	//!< Warning messages
	eFlexLogDebug	=  4,	//!< Used only in debug version of dll
	eFlexLogAll		= -1,	//!< All log types
};

 
/** Defines the set of stages at which callbacks may be registered 
 */
enum FlexSolverCallbackStage
{
	eFlexStageIterationStart,	//!< Called at the beginning of each constraint iteration
	eFlexStageIterationEnd,		//!< Called at the end of each constraint iteration
	eFlexStageSubstepBegin,		//!< Called at the beginning of each substep after the prediction step has been completed
	eFlexStageSubstepEnd,		//!< Called at the end of each substep after the velocity has been updated by the constraints
	eFlexStageUpdateEnd,		//!< Called at the end of solver update after the final substep has completed
	eFlexStageCount,			//!< Number of stages
};

/** Defines the different DirectX compute modes that Flex can use
*/
enum FlexComputeType
{
	eFlexCUDA,		//!< Use CUDA compute for Flex, the application must link against the CUDA libraries
	eFlexD3D11,		//!< Use DirectX 11 compute for Flex, the application must link against the D3D libraries
	eFlexD3D12,		//!< Use DirectX 12 compute for Flex, the application must link against the D3D libraries
};

/** Structure containing pointers to the internal solver data that is passed to each registered solver callback
 *
 *  @remarks Pointers to internal data are only valid for the lifetime of the callback and should not be stored.
 *  However, it is safe to launch kernels and memory transfers using the device pointers.
 *
 *  @remarks Because Flex re-orders particle data internally for performance, the particle data in the callback is not
 *  in the same order as it was provided to the API. The callback provides arrays which map original particle indices
 *  to sorted positions and vice-versa.
 *
 *  @remarks Particle positions may be modified during any callback, but velocity modifications should only occur during 
 *  the eFlexStageUpdateEnd stage, otherwise any velocity changes will be discarded.
 */
struct FlexSolverCallbackParams
{
	FlexSolver* mSolver;				//!< Pointer to the solver that the callback is registered to
	void* mUserData;					//!< Pointer to the user data provided to flexRegisterSolverCallback()

	float* mParticles;					//!< Device pointer to the active particle basic data in the form x,y,z,1/m
	float* mVelocities;					//!< Device pointer to the active particle velocity data in the form x,y,z,w (last component is not used)
	int* mPhases;						//!< Device pointer to the active particle phase data

	int mNumActive;						//!< The number of active particles returned, the callback data only return pointers to active particle data, this is the same as flexGetActiveCount()
	
	float mDt;							//!< The per-update time-step, this is the value passed to flexUpdateSolver()

	const int* mOriginalToSortedMap;	//!< Device pointer that maps the sorted callback data to the original position given by SetParticles()
	const int* mSortedToOriginalMap;	//!< Device pointer that maps the original particle index to the index in the callback data structure
};

/** Descriptor used to initialize Flex
*/
struct FlexInitDesc
{
	int mDeviceIndex;				//!< The GPU device index that should be used, if there is already a CUDA context on the calling thread then this parameter will be ignored and the active CUDA context used. Otherwise a new context will be created using the suggested device ordinal.
	bool mEnableExtension;			//!< Enable or disable NVIDIA extensions in DirectX, can lead to improved performance.
	void* mRenderDevice;			//!< Direct3D device to use for simulation, if none is specified a new device and context will be created.
	void* mRenderContext;			//!< Direct3D context to use for simulation, if none is specified a new context will be created, in DirectX 12 this should be a pointer to the ID3D12CommandQueue where compute operations will take place. 
	
	FlexComputeType mComputeType;	//!< Set to eFlexD3D11 if DirectX 11 should be used, eFlexD3D12 for DirectX 12, this must match the libraries used to link the application
};

/** Solver callback definition, see flexRegisterSolverCallback()
 */
struct FlexSolverCallback
{
	/** User data passed to the callback*/
	void* mUserData;
	
	/** Function pointer to a callback method */
	void (*mFunction)(FlexSolverCallbackParams params);
};

/**
 * Function pointer type for error reporting callbacks
 */
typedef void (*FlexErrorCallback)(FlexErrorSeverity type, const char* msg, const char* file, int line);

/**
* Initialize library, should be called before any other API function.
*
*
* @param[in] version The version number the app is expecting, should almost always be FLEX_VERSION
* @param[in] errorFunc The callback used for reporting errors.
* @param[in] desc The FlexInitDesc struct defining the device ordinal, D3D device/context and the type of D3D compute being used
* @return A pointer to a library instance that can be used to allocate shared object such as triangle meshes, buffers, etc
*/
FLEX_API FlexLibrary* flexInit(int version = FLEX_VERSION, FlexErrorCallback errorFunc = 0, FlexInitDesc * desc = 0);

/**
 * Shutdown library, users should manually destroy any previously created   
 * solvers to ensure memory is freed before calling this method. If a new CUDA context was created during flexInit() then it will be destroyed.
 *
 * @param[in] lib The library intance to use
 */
FLEX_API void flexShutdown(FlexLibrary* lib);

/**
 * Get library version number
 */
FLEX_API int flexGetVersion();

/**
 * Create a new particle solver
 *
 * @param[in] lib The library instance to use
 * @param[in] maxParticles Maximum number of simulation particles possible for this solver
 * @param[in] maxDiffuseParticles Maximum number of diffuse (non-simulation) particles possible for this solver
 * @param[in] maxNeighborsPerParticle Maximum number of neighbors per particle possible for this solver
 */
FLEX_API FlexSolver* flexCreateSolver(FlexLibrary* lib, int maxParticles, int maxDiffuseParticles, int maxNeighborsPerParticle = 96);
/**
 * Delete a particle solver
 *
 * @param[in] solver A valid solver pointer created from flexCreateSolver()
 */
FLEX_API void flexDestroySolver(FlexSolver* solver);

/**
 * Return the library associated with a solver
 *
 * @param[in] solver A valid solver created with flexCreateSolver()
 * @return A library pointer
 */
FLEX_API FlexLibrary* flexGetSolverLibrary(FlexSolver* solver);

/** Registers a callback for a solver stage, the callback will be invoked from the same thread that calls flexUpdateSolver().
 *
 * @param[in] solver A valid solver
 * @param[in] function A pointer to a function that will be called during the solver update
 * @param[in] stage The stage of the update at which the callback function will be called
 *
 * @return The previously registered callback for this slot, this allows multiple users to chain callbacks together
 */
FLEX_API FlexSolverCallback flexRegisterSolverCallback(FlexSolver* solver, FlexSolverCallback function, FlexSolverCallbackStage stage);

/**
 * Integrate particle solver forward in time. Below is an example of how to step Flex in the context of a simple game loop:
 *
 \code{.c}
	
		FlexLibrary* library = flexInit();
	FlexSolver* solver = flexCreateSolver(library);

	FlexBuffer* particleBuffer = flexAllocBuffer(library, n, sizeof(Vec4), eFlexBufferHost);
	FlexBuffer* velocityBuffer = flexAllocBuffer(library, n, sizeof(Vec4), eFlexBufferHost);
	FlexBuffer* phaseBuffer = flexAllocBuffer(library, n, sizeof(int), eFlexBufferHost);

	while(!done)
	{
		// map buffers for reading / writing
		float4* particles = (float4*)flexMap(particles, eFlexMapWait);
		float3* velocities  = (float3*)flexMap(velocities, eFlexMapWait);
		int* phases = (int*)flexMap(phases, eFlexMapWait);

		// spawn (user method)
		SpawnParticles(particles, velocities, phases);

		// render (user method)
		RenderParticles(particles, velocities, phases);

		// unmap buffers
		flexUnmap(particleBuffer);
		flexUnmap(velocityBuffer);
		flexUnmap(phaseBuffer);

		// write to device (async)
		flexSetParticles(particleBuffer, n);
		flexSetVelocities(velocityBuffer, n);
		flexSetPhases(phaseBuffer, n);

		// tick
		flexUpdateSolver(solver, dt, 1, NULL);

		// read back (async)
		flexGetParticles(particleBuffer, n);
		flexGetVelocities(velocityBuffer, n);
		flexGetPhases(phaseBuffer, n);
	}

	flexFreeBuffer(particleBuffer);
	flexFreeBuffer(velocityBuffer);
	flexFreeBuffer(phaseBuffer);

	flexDestroySolver(solver);
	flexShutdown(library);


 \endcode
 *
 * @param[in] solver A valid solver
 * @param[in] dt Time to integrate the solver forward in time by
 * @param[in] substeps The time dt will be divided into the number of sub-steps given by this parameter
 * @param[in] enableTimers Whether to enable per-kernel timers for profiling. Note that profiling can substantially slow down overall performance so this param should only be true in non-release builds
 */
FLEX_API void flexUpdateSolver(FlexSolver* solver, float dt, int substeps, bool enableTimers);

/**
 * Update solver paramters
 *
 * @param[in] solver A valid solver
 * @param[in] params Parameters structure in host memory, see FlexParams
 */
FLEX_API void flexSetParams(FlexSolver* solver, const FlexParams* params);

/**
 * Retrieve solver paramters, default values will be set at solver creation time
 *
 * @param[in] solver A valid solver
 * @param[out] params Parameters structure in host memory, see FlexParams
 */

FLEX_API void flexGetParams(FlexSolver* solver, FlexParams* params);

/**
 * Set the active particles indices in the solver
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Holds the indices of particles that have been made active
 * @param[in] n Number of particles to allocate
 */
FLEX_API void flexSetActive(FlexSolver* solver, FlexBuffer* indices, int n);

/**
 * Return the active particle indices
 * 
 * @param[in] solver A valid solver
 * @param[out] indices a buffer of indices at least activeCount in length
 */
FLEX_API void flexGetActive(FlexSolver* solver, FlexBuffer* indices);

/**
 * Return the number of active particles in the solver
 * 
 * @param[in] solver A valid solver
 * @return The number of active particles in the solver
 */
FLEX_API int flexGetActiveCount(FlexSolver* solver);

/**
 * Set the particles state of the solver, a particle consists of 4 floating point numbers, its x,y,z position followed by its inverse mass (1/m)
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
FLEX_API void flexSetParticles(FlexSolver* solver, FlexBuffer* p, int n);

/**
 * Get the particles state of the solver, a particle consists of 4 floating point numbers, its x,y,z position followed by its inverse mass (1/m)
 * 
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of 4*n floats that will be filled out with the particle data, can be either a host or device pointer
 * @param[in] n The number of particles to get, must be less than max particles passed to flexCreateSolver
 */
FLEX_API void flexGetParticles(FlexSolver* solver, FlexBuffer* p, int n);

/**
 * Set the particle positions in their rest state, if FlexPhase::eFlexPhaseSelfCollideFilter is set on the particle's
 * phase attribute then particles that overlap in the rest state will not generate collisions with each other
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
FLEX_API void flexSetRestParticles(FlexSolver* solver, FlexBuffer* p, int n);

/**
 * Get the particle positions in their rest state
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
FLEX_API void flexGetRestParticles(FlexSolver* solver, FlexBuffer* p, int n);


/**
 * Get the Laplacian smoothed particle positions for rendering, see FlexParams::mSmoothing
 * 
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of 4*n floats that will be filled out with the data, can be either a host or device pointer
 * @param[in] n The number of smooth particles to return
 */
FLEX_API void flexGetSmoothParticles(FlexSolver* solver, FlexBuffer*  p, int n);

/**
 * Set the particle velocities, each velocity is a 3-tuple of x,y,z floating point values
 * 
 * @param[in] solver A valid solver
 * @param[in] v Pointer to a buffer of 3*n floats
 * @param[in] n The number of velocities to set
 *
 */
FLEX_API void flexSetVelocities(FlexSolver* solver, FlexBuffer*  v, int n);
/**
 * Get the particle velocities, each velocity is a 3-tuple of x,y,z floating point values
 * 
 * @param[in] solver A valid solver
 * @param[out] v Pointer to a buffer of 3*n floats that will be filled out with the data, can be either a host or device pointer
 * @param[in] n The number of velocities to get
 */
FLEX_API void flexGetVelocities(FlexSolver* solver, FlexBuffer*  v, int n);

/**
 * Set the particles phase id array, each particle has an associated phase id which 
 * controls how it interacts with other particles. Particles with phase 0 interact with all
 * other phase types.
 *
 * Particles with a non-zero phase id only interact with particles whose phase differs 
 * from theirs. This is useful, for example, to stop particles belonging to a single
 * rigid shape from interacting with each other.
 * 
 * Phase 0 is used to indicate fluid particles when FlexParams::mFluid is set.
 * 
 * @param[in] solver A valid solver
 * @param[in] phases Pointer to a buffer of n integers containing the phases
 * @param[in] n The number of phases to set
 *
 */
FLEX_API void flexSetPhases(FlexSolver* solver, FlexBuffer* phases, int n);
/**
 * Get the particle phase ids
 * 
 * @param[in] solver A valid solver
 * @param[out] phases Pointer to a buffer of n integers that will be filled with the phase data, can be either a host or device pointer
 * @param[in] n The number of phases to get
 */
FLEX_API void flexGetPhases(FlexSolver* solver, FlexBuffer* phases, int n);

/**
 * Set per-particle normals to the solver, these will be overwritten after each simulation step, but can be used to initialize the normals to valid values
 * 
 * @param[in] solver A valid solver
 * @param[in] normals Pointer to a buffer of normals, should be 4*n in length
 * @param[in] n The number of normals to set
 *
 */
FLEX_API void flexSetNormals(FlexSolver* solver, FlexBuffer* normals, int n);

/**
 * Get per-particle normals from the solver, these are the world-space normals computed during surface tension, cloth, and rigid body calculations
 * 
 * @param[in] solver A valid solver
 * @param[out] normals Pointer to a buffer of normals, should be 4*n in length
 * @param[in] n The number of normals to get
 */
FLEX_API void flexGetNormals(FlexSolver* solver, FlexBuffer* normals, int n);


/**
 * Set distance constraints for the solver. Each distance constraint consists of two particle indices
 * stored consecutively, a rest-length, and a stiffness value. These are not springs in the traditional
 * sense, but behave somewhat like a traditional spring when lowering the stiffness coefficient.
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Pointer to the spring indices array, should be 2*numSprings length, 2 indices per-spring
 * @param[in] restLengths Pointer to a buffer of rest lengths, should be numSprings length
 * @param[in] stiffness Pointer to the spring stiffness coefficents, should be numSprings in length, a negative stiffness value represents a tether constraint
 * @param[in] numSprings The number of springs to set
 *
 */
FLEX_API void flexSetSprings(FlexSolver* solver, FlexBuffer* indices, FlexBuffer* restLengths, FlexBuffer* stiffness, int numSprings);
/**
 * Get the distance constraints from the solver
 * 
 * @param[in] solver A valid solver
 * @param[out] indices Pointer to the spring indices array, should be 2*numSprings length, 2 indices per-spring
 * @param[out] restLengths Pointer to a buffer of rest lengths, should be numSprings length
 * @param[out] stiffness Pointer to the spring stiffness coefficents, should be numSprings in length, a negative stiffness value represents a unilateral tether constraint (only resists stretching, not compression), valid range [-1, 1]
 * @param[in] numSprings The number of springs to get
 */
FLEX_API void flexGetSprings(FlexSolver* solver, FlexBuffer* indices, FlexBuffer* restLengths, FlexBuffer* stiffness, int numSprings);

/**
 * Set rigid body constraints for the solver. 
 * @note A particle should not belong to more than one rigid body at a time.
 * 
 * @param[in] solver A valid solver
 * @param[in] offsets Pointer to a buffer of start offsets for a rigid in the indices array, should be numRigids+1 in length, the first entry must be 0
 * @param[in] indices Pointer to a buffer of indices for the rigid bodies, the indices for the jth rigid body start at indices[offsets[j]] and run to indices[offsets[j+1]] exclusive
 * @param[in] restPositions Pointer to a buffer of local space positions relative to the rigid's center of mass (average position), this should be at least 3*numIndices in length in the format x,y,z
 * @param[in] restNormals Pointer to a buffer of local space normals, this should be at least 4*numIndices in length in the format x,y,z,w where w is the (negative) signed distance of the particle inside its shape
 * @param[in] stiffness Pointer to a buffer of rigid stiffness coefficents, should be numRigids in length, valid values in range [0, 1]
 * @param[in] rotations Pointer to a buffer of quaternions (4*numRigids in length)
 * @param[in] translations Pointer to a buffer of translations of the center of mass (3*numRigids in length)
 * @param[in] numRigids The number of rigid bodies to set
 * @param[in] numIndices The number of indices in the indices array
 *
 */
FLEX_API void flexSetRigids(FlexSolver* solver, FlexBuffer* offsets, FlexBuffer* indices, FlexBuffer* restPositions, FlexBuffer* restNormals, FlexBuffer* stiffness, FlexBuffer* rotations, FlexBuffer* translations, int numRigids, int numIndices);


/**
 * Get the rotation matrices for the rigid bodies in the solver
 * 
 * @param[in] solver A valid solver
 * @param[out] rotations Pointer to a buffer of quaternions, should be 4*numRigids floats in length
 * @param[out] translations Pointer to a buffer of vectors to hold the rigid translations, should be 3*numRigids floats in length
 */
FLEX_API void flexGetRigidTransforms(FlexSolver* solver, FlexBuffer* rotations, FlexBuffer* translations);

/**
 * An opaque type representing a static triangle mesh in the solver
 */
typedef unsigned int FlexTriangleMeshId;

/**
 * An opaque type representing a signed distance field collision shape in the solver.
 */
typedef unsigned int FlexDistanceFieldId;

/**
 * An opaque type representing a convex mesh collision shape in the solver.
 * Convex mesh shapes may consist of up to 64 planes of the form a*x + b*y + c*z + d = 0,
 * particles will be constrained to the outside of the shape.
 */
typedef unsigned int FlexConvexMeshId;

/**
 * Create triangle mesh geometry, note that meshes may be used by multiple solvers if desired
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a triangle mesh object
 */
FLEX_API FlexTriangleMeshId flexCreateTriangleMesh(FlexLibrary* lib);

/**
 * Destroy a triangle mesh created with flexCreateTriangleMesh()
 *
 * @param[in] lib The library instance to use
 * @param[in] mesh A triangle mesh created with flexCreateTriangleMesh()
 */
FLEX_API void flexDestroyTriangleMesh(FlexLibrary* lib, FlexTriangleMeshId mesh);

/**
 * Specifies the triangle mesh geometry (vertices and indices), this method will cause any internal
 * data structures (e.g.: bounding volume hierarchies) to be rebuilt.
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh A triangle mesh created with flexCreateTriangleMesh()
 * @param[in] vertices Pointer to a buffer of float3 vertex positions
 * @param[in] indices Pointer to a buffer of triangle indices, should be length numTriangles*3
 * @param[in] numVertices The number of vertices in the vertices array
 * @param[in] numTriangles The number of triangles in the mesh
 * @param[in] lower A pointer to a float3 vector holding the lower spatial bounds of the mesh
 * @param[in] upper A pointer to a float3 vector holding the upper spatial bounds of the mesh
 */
FLEX_API void flexUpdateTriangleMesh(FlexLibrary* lib, FlexTriangleMeshId mesh, FlexBuffer* vertices, FlexBuffer* indices, int numVertices, int numTriangles, const float* lower, const float* upper);

/**
 * Retrieve the local space bounds of the mesh, these are the same values specified to flexUpdateTriangleMesh()
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh Pointer to a triangle mesh object
 * @param[out] lower Pointer to a buffer of 3 floats that the lower mesh bounds will be written to
 * @param[out] upper Pointer to a buffer of 3 floats that the upper mesh bounds will be written to
 */
FLEX_API void flexGetTriangleMeshBounds(FlexLibrary* lib, const FlexTriangleMeshId mesh, float* lower, float* upper);

/**
 * Create a signed distance field collision shape, see FlexDistanceFieldId for details.
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a signed distance field object
 */
FLEX_API FlexDistanceFieldId flexCreateDistanceField(FlexLibrary* lib);

/**
 * Destroy a signed distance field
 * 
 * @param[in] lib The library instance to use
 * @param[in] sdf A signed distance field created with flexCreateDistanceField()
 */
FLEX_API void flexDestroyDistanceField(FlexLibrary* lib, FlexDistanceFieldId sdf);

/**
 * Update the signed distance field volume data, this method will upload
 * the field data to a 3D texture on the GPU
 * 
 * @param[in] lib The library instance to use
 * @param[in] sdf A signed distance field created with flexCreateDistanceField()
 * @param[in] dimx The x-dimension of the volume data in voxels
 * @param[in] dimy The y-dimension of the volume data in voxels
 * @param[in] dimz The z-dimension of the volume data in voxels
 * @param[in] field The volume data stored such that the voxel at the x,y,z coordinate is addressed as field[z*dimx*dimy + y*dimx + x]
 */
FLEX_API void flexUpdateDistanceField(FlexLibrary* lib, FlexDistanceFieldId sdf, int dimx, int dimy, int dimz, FlexBuffer* field);

/**
 * Create a convex mesh collision shapes, see FlexConvexMeshId for details.
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a signed distance field object
 */
FLEX_API FlexConvexMeshId flexCreateConvexMesh(FlexLibrary* lib);

/**
 * Destroy a convex mesh
 * 
 * @param[in] lib The library instance to use
 * @param[in] sdf A signed distance field created with flexCreateConvexMesh()
 */
FLEX_API void flexDestroyConvexMesh(FlexLibrary* lib, FlexConvexMeshId convex);

/**
 * Update the convex mesh geometry
 * 
 * @param[in] lib The library instance to use
 * @param[in] convex A valid convex mesh shape created from flexCreateConvexMesh()
 * @param[in] planes An array of planes, each plane consists of 4 floats in the form a*x + b*y + c*z + d = 0
 * @param[in] numPlanes The number of planes in the convex
 * @param[in] lower The local space lower bound of the convex shape
 * @param[in] upper The local space upper bound of the convex shape
  */
FLEX_API void flexUpdateConvexMesh(FlexLibrary* lib, FlexConvexMeshId convex, FlexBuffer* planes, int numPlanes, float* lower, float* upper);

/**
 * Retrieve the local space bounds of the mesh, these are the same values specified to flexUpdateConvexMesh()
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh Pointer to a convex mesh object
 * @param[out] lower Pointer to a buffer of 3 floats that the lower mesh bounds will be written to
 * @param[out] upper Pointer to a buffer of 3 floats that the upper mesh bounds will be written to
 */
FLEX_API void flexGetConvexMeshBounds(FlexLibrary* lib, FlexConvexMeshId meshIndex, float* lower, float* upper);

/**
 * A basic sphere shape with origin at the center of the sphere and radius
 */
struct FlexSphereGeometry
{
	float mRadius;
};

/**
 * A collision capsule extends along the x-axis with its local origin at the center of the capsule 
 */
struct FlexCapsuleGeometry
{
	float mRadius;
	float mHalfHeight;
};

/**
 * A simple box with interior [-halfHeight, +halfHeight] along each dimension 
 */
struct FlexBoxGeometry
{
	float mHalfExtents[3];
};

/**
 * A convex mesh instance with non-uniform scale
 */
struct FlexConvexMeshGeometry
{
	float mScale[3];
	FlexConvexMeshId mMesh;
};

/**
 * A scaled triangle mesh instance with non-uniform scale
 */
struct FlexTriangleMeshGeometry
{
	float mScale[3];			//!< The scale of the object from local space to world space
	FlexTriangleMeshId mMesh;	//!< A triangle mesh pointer created by flexCreateTriangleMesh()
};

/**
 * A scaled signed distance field instance, the local origin of the SDF is at corner of the field corresponding to the first voxel.
 * The field is mapped to the local space volume [0, 1] in each dimension.
 */
struct FlexSDFGeometry
{
	float mScale;				 //!< Uniform scale of SDF, this corresponds to the world space width of the shape
	FlexDistanceFieldId mField;	 //!< A signed distance field pointer created by flexCreateDistanceField()
};

/**
 * This union allows collision geometry to be sent to Flex as a flat array of 16-byte data structures,
 * the shape flags array specifies the type for each shape, see flexSetShapes().
 */
union FlexCollisionGeometry
{
	FlexSphereGeometry mSphere;
	FlexCapsuleGeometry mCapsule;
	FlexBoxGeometry mBox;
	FlexConvexMeshGeometry mConvex;
	FlexTriangleMeshGeometry mTriMesh;
	FlexSDFGeometry mSDF;
};

enum FlexCollisionShapeType
{
	eFlexShapeSphere		= 0,		//!< A sphere shape, see FlexSphereGeometry
	eFlexShapeCapsule		= 1,		//!< A capsule shape, see FlexCapsuleGeometry
	eFlexShapeBox			= 2,		//!< A box shape, see FlexBoxGeometry
	eFlexShapeConvexMesh	= 3,		//!< A convex mesh shape, see FlexConvexMeshGeometry
	eFlexShapeTriangleMesh  = 4,		//!< A triangle mesh shape, see FlexTriangleMeshGeometry
	eFlexShapeSDF			= 5,		//!< A signed distance field shape, see FlexSDFGeometry
};

enum FlexCollisionShapeFlags
{
	eFlexShapeFlagTypeMask	= 0x7,		//!< Lower 3 bits holds the type of the collision shape
	eFlexShapeFlagDynamic	= 8,		//!< Indicates the shape is dynamic and should have lower priority over static collision shapes
	eFlexShapeFlagTrigger	= 16,		//!< Indicates that the shape is a trigger volume, this means it will not perform any collision response, but will be reported in the contacts array (see flexGetContacts())

	eFlexShapeFlagReserved = 0xffffff00
};

/** 
 * Combines geometry type and static/dynamic flags
 */
FLEX_API inline int flexMakeShapeFlags(FlexCollisionShapeType type, bool dynamic) { return type | (dynamic?eFlexShapeFlagDynamic:0); }

/**
 * Set the collision shapes for the solver
 * 
 * @param[in] solver A valid solver
 * @param[in] geometry Pointer to a buffer of FlexCollisionGeometry entries, the type of each shape determines how many entries it has in the array
 * @param[in] shapePositions Pointer to a buffer of translations for each shape in world space, should be 4*numShapes in length
 * @param[in] shapeRotations Pointer to an a buffer of rotations for each shape stored as quaternion, should be 4*numShapes in length
 * @param[in] shapePrevPositions Pointer to a buffer of translations for each shape at the start of the time step, should be 4*numShapes in length
 * @param[in] shapePrevRotations Pointer to an a buffer of rotations for each shape stored as a quaternion at the start of the time step, should be 4*numShapees in length
 * @param[in] shapeFlags The type and behavior of the shape, FlexCollisionShapeFlags for more detail
 * @param[in] numShapes The number of shapes
 *
 */
FLEX_API void flexSetShapes(FlexSolver* solver, FlexBuffer* geometry, FlexBuffer* shapePositions, FlexBuffer* shapeRotations, FlexBuffer* shapePrevPositions, FlexBuffer* shapePrevRotations, FlexBuffer* shapeFlags, int numShapes);

/**
 * Set dynamic triangles mesh indices, typically used for cloth. Flex will calculate normals and 
 * apply wind and drag effects to connected particles. See FlexParams::mDrag, FlexParams::mWind.
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Pointer to a buffer of triangle indices into the particles array, should be 3*numTris in length
 * @param[in] normals Pointer to a buffer of triangle normals, should be 3*numTris in length, can be NULL
 * @param[in] numTris The number of dynamic triangles
 *s
 */
FLEX_API void flexSetDynamicTriangles(FlexSolver* solver, FlexBuffer* indices, FlexBuffer* normals, int numTris);
/**
 * Get the dynamic triangle indices and normals.
 * 
 * @param[in] solver A valid solver
 * @param[out] indices Pointer to a buffer of triangle indices into the particles array, should be 3*numTris in length, if NULL indices will not be returned
 * @param[out] normals Pointer to a buffer of triangle normals, should be 3*numTris in length, if NULL normals will be not be returned
 * @param[in] numTris The number of dynamic triangles
 */
FLEX_API void flexGetDynamicTriangles(FlexSolver* solver, FlexBuffer* indices, FlexBuffer* normals, int numTris);

/**
 * Set inflatable shapes, an inflatable is a range of dynamic triangles (wound CCW) that represent a closed mesh.
 * Each inflatable has a given rest volume, constraint scale (roughly equivalent to stiffness), and "over pressure"
 * that controls how much the shape is inflated.
 * 
 * @param[in] solver A valid solver
 * @param[in] startTris Pointer to a buffer of offsets into the solver's dynamic triangles for each inflatable, should be numInflatables in length
 * @param[in] numTris Pointer to a buffer of triangle counts for each inflatable, should be numInflatablesin length
 * @param[in] restVolumes Pointer to a buffer of rest volumes for the inflatables, should be numInflatables in length
 * @param[in] overPressures Pointer to a buffer of floats specifying the pressures for each inflatable, a value of 1.0 means the rest volume, > 1.0 means over-inflated, and < 1.0 means under-inflated, should be numInflatables in length
 * @param[in] constraintScales Pointer to a buffer of scaling factors for the constraint, this is roughly equivalent to stiffness but includes a constraint scaling factor from position-based dynamics, see helper code for details, should be numInflatables in length
 * @param[in] numInflatables Number of inflatables to set
 *
 */
FLEX_API void flexSetInflatables(FlexSolver* solver, FlexBuffer* startTris, FlexBuffer* numTris, FlexBuffer* restVolumes, FlexBuffer* overPressures, FlexBuffer* constraintScales, int numInflatables);

/**
 * Get the density values for fluid particles
 *
 * @param[in] solver A valid solver
 * @param[in] n The number of particle densities to return
 * @param[out] densities Pointer to a buffer of floats, should be maxParticles in length, density values are normalized between [0, 1] where 1 represents the rest density
 */
FLEX_API void flexGetDensities(FlexSolver* solver, FlexBuffer* densities, int n);
/**
 * Get the anisotropy of fluid particles, the particle distribution for a particle is represented
 * by 3 orthogonal vectors. Each 3-vector has unit length with the variance along that axis
 * packed into the w component, i.e.: x,y,z,lambda.
*
 * The anisotropy defines an oriented ellipsoid in worldspace that can be used for rendering
 * or surface extraction.
 *
 * @param[in] solver A valid solver
 * @param[out] q1 Pointer to a buffer of floats that receive the first basis vector and scale, should be 4*maxParticles in length
 * @param[out] q2 Pointer to a buffer of floats that receive the second basis vector and scale, should be 4*maxParticles in length
 * @param[out] q3 Pointer to a buffer of floats that receive the third basis vector and scale, should be 4*maxParticles in length
 */
FLEX_API void flexGetAnisotropy(FlexSolver* solver, FlexBuffer* q1, FlexBuffer* q2, FlexBuffer* q3);
/**
 * Get the state of the diffuse particles. Diffuse particles are passively advected by the fluid
 * velocity field.
 *
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of floats, should be 4*maxParticles in length, the w component represents the particles lifetime with 1 representing a new particle, and 0 representing an inactive particle
 * @param[out] v Pointer to a buffer of floats, should be 4*maxParticles in length, the w component is not used
 * @param[out] indices Pointer to a buffer of ints that specify particle indices in depth sorted order, should be maxParticles in length, see FlexParams::mDiffuseSortDir
 */
FLEX_API int flexGetDiffuseParticles(FlexSolver* solver, FlexBuffer* p, FlexBuffer* v, FlexBuffer* indices);
/**
 * Set the state of the diffuse particles. Diffuse particles are passively advected by the fluid
 * velocity field.
 *
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of floats, should be 4*n in length, the w component represents the particles lifetime with 1 representing a new particle, and 0 representing an inactive particle
 * @param[in] v Pointer to a buffer of floats, should be 4*n in length, the w component is not used
 * @param[in] n Number of diffuse particles to set
 *
 */
FLEX_API void flexSetDiffuseParticles(FlexSolver* solver, FlexBuffer* p, FlexBuffer* v, int n);

/**
 * Get the particle contact planes. Note this will only include contacts that were active on the last substep of an update, and will include all contact planes generated within FlexParam::mShapeCollisionMargin.
 *
 * @param[in] solver A valid solver
 * @param[out] planes Pointer to a destination buffer containing the contact planes for the particle, each particle can have up to 4 contact planes so this buffer should be 16*maxParticles in length
 * @param[out] velocities Pointer to a destination buffer containing the velocity of the contact point on the shape in world space, the index of the shape (corresponding to the shape in flexSetShapes() is stored in the w component), each particle can have up to 4 contact planes so this buffer should be 16*maxParticles in length
 * @param[out] indices Pointer to a buffer of indices into the contacts buffer, the first contact plane for the i'th particle is given by planes[indices[i]*sizeof(float)*4] and subsequent contacts for that particle are stored sequentially, this array should be maxParticles in length
 * @param[out] counts Pointer to a buffer of contact counts for each particle (will be <= 4), this buffer should be maxParticles in length
 */
FLEX_API void flexGetContacts(FlexSolver* solver, FlexBuffer* planes, FlexBuffer* velocities, FlexBuffer* indices, FlexBuffer* counts);

/**
 * Get the world space AABB of all particles in the solver, note that the bounds are calculated during the update (see flexUpdateSolver()) so only become valid after an update has been performed.
 * The returned bounds represent bounds of the particles in their predicted positions *before* the constraint solve.
 * 
 * @param[in] solver A valid solver
 * @param[out] lower Pointer to a buffer of 3 floats to receive the lower bounds
 * @param[out] upper Pointer to a buffer of 3 floats to receive the upper bounds
 */
FLEX_API void flexGetBounds(FlexSolver* solver, FlexBuffer* lower, FlexBuffer* upper);

/**
 *
 * @param[in] solver A valid solver
 * @return The time in seconds between the first and last GPU operations executed by the last flexUpdateSolver.
 *
 * @note This method causes the CPU to wait until the GPU has finished any outstanding work. 
 *		 To avoid blocking the calling thread it should be called after work has completed, e.g.: directly after a flexMap().
 */
FLEX_API float flexGetDeviceLatency(FlexSolver* solver);

/**
 * Fetch detailed GPU timers.
 *
 * @param[in] lib The library instance to use
 * @param[out] timers A struct containing the GPU latency of each stage in the physics pipeline.
 *
 * @note This method causes the CPU to wait until the GPU has finished any outstanding work.
 *		 To avoid blocking the calling thread it should be called after work has completed, e.g.: directly after a flexMap().
 *       To capture there timers you must pass true for enableTimers in flexUpdateSolver()
 */
FLEX_API void flexGetTimers(FlexLibrary* lib, FlexTimers* timers);

/**
 * Allocate a Flex buffer. Buffers are used to pass data to the API in an efficient manner.
 *
 * @param[in] lib The library instance to use
 * @param[in] elementCount The number of elements in the buffer
 * @param[in] elementByteStride The size of each element in bytes
 * @param[in] type The type of buffer to allocate, can be either host memory or device memory
 * @return A pointer to a FlexBuffer
 */
FLEX_API FlexBuffer* flexAllocBuffer(FlexLibrary* lib, int elementCount, int elementByteStride, FlexBufferType type);

/**
 * Free a Flex buffer
 *
 * @param[in] buf A buffer to free, must be allocated with flexAllocBuffer()
 */
FLEX_API void flexFreeBuffer(FlexBuffer* buf);

/**
 * Maps a buffer for reading and writing. When the buffer is created with FlexBufferType::eHost, then the returned pointer will be a host memory address
 * that can be read/written.
 * Mapping a buffer implicitly synchronizes with the GPU to ensure that any reads or writes from the buffer (e.g.: from the flexGet*() or flexSet*()) methods have completed.
 *
 * @param[in] buffer A buffer allocated with flexAllocBuffer()
 * @param[in] flags Hints to Flex how the buffer is to be accessed, typically this should be eFlexMapWait (0)
 * @return A pointer to the mapped memory
 */
FLEX_API void* flexMap(FlexBuffer* buffer, int flags);

/**
 * Unmaps a buffer that was mapped through flexMap(), note that buffers must be unmapped before they can be passed to a flexGet*() or flexSet*() method
 *
 * @param[in] buffer A valid buffer allocated through flexAllocBuffer()
 */
FLEX_API void flexUnmap(FlexBuffer* buffer);

/**
 * Registers an OpenGL buffer to Flex which can be used to copy directly into a graphics resource. Example usage is below
 *
 \code{.c}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW)

	FlexBuffer* vboBuffer = flexRegisterOGLBuffer(lib, vbo, n, sizeof(float)*4);

	// simulate 
	...

	// copy directly from Flex into render buffer
	flexGetParticles(vboBuffer, n);

	// render
	...

 \endcode
 *
 * @param[in] lib The library instance to use
 * @param[in] buf An OpenGL buffer identifier
 * @param[in] elementCount The number of elements in the buffer
 * @param[in] elementByteStride the size of each element in bytes
 * @return A valid FlexBuffer pointer that may be used with flexGet*() methods to populate the render buffer using direct GPU-GPU copies
 */
FLEX_API FlexBuffer* flexRegisterOGLBuffer(FlexLibrary* lib, int buf, int elementCount, int elementByteStride);

/**
 * Unregister a FlexBuffer allocated through flexRegisterOGLBuffer()
 *
 * @param[in] buf A valid buffer
 */FLEX_API void flexUnregisterOGLBuffer(FlexBuffer* buf);

/** 
 * Ensures that the CUDA context the library was initialized with is present on the current thread
 *
 * @param[in] lib The library instance to use
 */
FLEX_API void flexAcquireContext(FlexLibrary* lib);

/** 
 * Restores the CUDA context (if any) that was present on the last call to flexAcquireContext()
 * Note: the acquire/restore pair of calls must come from the same thread
 */
FLEX_API void flexRestoreContext(FlexLibrary* lib);

/** 
 * Returns a null-terminated string with the compute device name
 *
 * @param[in] lib The library instance to use
 */
FLEX_API const char* flexGetDeviceName(FlexLibrary* lib);

/** 
 * Retrieve the device and context for the the library.
 * On CUDA the context pointer will be filled with a pointer to a CUcontext structure
 * On D3D the device and context pointers will be filled with pointers to a NvFlex::Device, and NvFlex::Context wrapper
 *
 * @param[in] lib Pointer to a valid library returned from flexInit()
 * @param[out] device Pointer to a device pointer, see description
 * @param[out] context Pointer to a context pointer, see description
 */
FLEX_API void flexGetDeviceAndContext(FlexLibrary* lib, void** device, void** context);
 


/**
 * Force a pipeline flush to ensure any queued work is submitted to the GPU
 *
 * @param[in] lib The library instance to use
 */
FLEX_API void flexFlush(FlexLibrary* lib);

//! \cond HIDDEN_SYMBOLS

/**
 * Debug methods (unsupported)
 */
FLEX_API void flexSetDebug(FlexSolver* solver, bool enable);
FLEX_API void flexGetStaticTriangleGrid(FlexSolver* solver, int* counts, float* lower, float* cellEdge);
FLEX_API void flexGetStaticTriangleBVH(FlexSolver* solver, void* bvh);
FLEX_API void flexStartRecord(FlexSolver* solver, const char* file);
FLEX_API void flexStopRecord(FlexSolver* solver);


/**
* Copy a particle solver
*
* @param[out] dst The solver to copy to
* @param[in] dst The solver to copy from
*/
FLEX_API void flexCopySolver(FlexSolver* dst, FlexSolver* src);

//! \endcond

} // extern "C"

#endif // FLEX_H
