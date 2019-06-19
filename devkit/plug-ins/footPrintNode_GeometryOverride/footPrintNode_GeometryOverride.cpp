
//-
// ==========================================================================
// Copyright 2015 Autodesk, Inc.  All rights reserved.
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
// ==========================================================================
//+

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// This plug-in demonstrates how to draw a simple mesh like foot Print in an efficient way.
//
// This efficient way is supported in Viewport 2.0.
//
// For comparison, you can also reference a Maya Developer Kit sample named footPrintNode.
// In that sample, we draw the footPrint using the MUIDrawManager primitives in footPrintDrawOverride::addUIDrawables()
//
// For comparison, you can also reference another Maya Developer Kit sample named rawfootPrintNode.
// In that sample, we draw the footPrint with OpenGL\DX in method rawFootPrintDrawOverride::draw().
//
//
// This plugin uses two different techniques to optimize the performance of the foot print draw
// in VP2.
// 
// Technique 1:
// In order to know when the FootPrintGeometryOverride geometry needs to be updated the
// footPrint node taps into dirty propagation and the evaluation manager to track when
// attributes which affect geometry change.  FootPrintGeometryOverride can then query
// footPrint to find out if a geometry update is necessary.
// 
// Technique 2:
// In order to know when the FootPrintGeometryOverride render items need to be updated
// the factors affecting how render items are drawn are stored.  When updateRenderItems()
// is called the current values can be compared against the previously used values,
// allowing the bulk of the updateRenderItems() work to only occur when necessary.
// 
// Evaluation Caching:
// footPrint is fully compatible with Evaluation Caching.  Supporting Evaluation Caching
// does add some additional, subtle requirements on the plug-in node.  Evaluation Caching
// automatically stores data for two types of attributes: output attributes and dynamic
// attributes, where output attributes are defined as any attribute which is affected by
// another attribute on the node.  The affects relationship can be created either by
// calling MPxNode::attributeAffects() or by returning affected attributes from
// MPxNode::setDependentsDirty().
// 
// Using Evaluation Caching with Evaluation Manager Parallel Update has an additional issue
// to be aware of.  When using Evaluation Manager Parallel Update some MPxGeometryOverride
// methods are called after the corresponding DAG node has been evaluated but before the 
// full evaluation graph has been evaluated.  When Evaluation Caching is enabled only
// cached DG values may be read before the full evaluation graph has been evaluated.  Therefore,
// when using Evaluation Manager Parallel Update and Evaluation Caching only cached DG values 
// may be read.  Attempting to read an uncached value before evaluation finishes results in 
// undefined behavior (incorrect data or crashes).
// 
// VP2 Custom Caching:
// FootPrintGeometryOverride is fully compatible with VP2 Custom Caching.  When using VP2
// custom caching the MPxGeometryOverride may be invoked in the normal context or in a 
// background thread using another context.  If playback and background evaluation occur
// concurrently Maya guarantees that all of the MPxGeoemtryOverride methods called for a
// context occur atomically without being interleaved with MPxGeometryOverride methods
// for the same DAG object in a different context.
// 
// However, Maya does not make any timing guarantee between the call(s) to evaluate
// or restore values to the MPxNode and calls to the MPxGeometryOverride.  For example,
// a postEvaluation call in the background evaluation thread may occur at the some time
// the foreground thread is using the MPxGeometryOverride.
// 
// This means that any communication which occurs between the MPxNode during evaluation
// and the MPxGeometryOverride must be context aware.  The communication channel must
// use different storage for each context.  The easiest way to implement this is to use
// internal attributes on the MPxNode which may be accessed by the MPxGeometryOverride.
// 
// Internal attributes are used as the communication channel between footPrint and
// FootPrintGeometryOverride as a part of Technique 1.
// 
// 
// Example usage is to load the plug-in and create the node using
// the createNode command:
//
// loadPlugin footPrintNode_GeometryOverride;
// createNode footPrint_GeometryOverride;
//
////////////////////////////////////////////////////////////////////////

#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/MFnPlugin.h>
#include <maya/MDistance.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MEvaluationNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagMessage.h>

// Viewport 2.0 includes
#include <maya/MDrawRegistry.h>
#include <maya/MPxGeometryOverride.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPointArray.h>

#include <unordered_map>

namespace
{
	// Foot Data
	//
	float sole[][3] = { {  0.00f, 0.0f, -0.70f },
						{  0.04f, 0.0f, -0.69f },
						{  0.09f, 0.0f, -0.65f },
						{  0.13f, 0.0f, -0.61f },
						{  0.16f, 0.0f, -0.54f },
						{  0.17f, 0.0f, -0.46f },
						{  0.17f, 0.0f, -0.35f },
						{  0.16f, 0.0f, -0.25f },
						{  0.15f, 0.0f, -0.14f },
						{  0.13f, 0.0f,  0.00f },
						{  0.00f, 0.0f,  0.00f },
						{ -0.13f, 0.0f,  0.00f },
						{ -0.15f, 0.0f, -0.14f },
						{ -0.16f, 0.0f, -0.25f },
						{ -0.17f, 0.0f, -0.35f },
						{ -0.17f, 0.0f, -0.46f },
						{ -0.16f, 0.0f, -0.54f },
						{ -0.13f, 0.0f, -0.61f },
						{ -0.09f, 0.0f, -0.65f },
						{ -0.04f, 0.0f, -0.69f },
						{ -0.00f, 0.0f, -0.70f } };
	float heel[][3] = { {  0.00f, 0.0f,  0.06f },
						{  0.13f, 0.0f,  0.06f },
						{  0.14f, 0.0f,  0.15f },
						{  0.14f, 0.0f,  0.21f },
						{  0.13f, 0.0f,  0.25f },
						{  0.11f, 0.0f,  0.28f },
						{  0.09f, 0.0f,  0.29f },
						{  0.04f, 0.0f,  0.30f },
						{  0.00f, 0.0f,  0.30f },
						{ -0.04f, 0.0f,  0.30f },
						{ -0.09f, 0.0f,  0.29f },
						{ -0.11f, 0.0f,  0.28f },
						{ -0.13f, 0.0f,  0.25f },
						{ -0.14f, 0.0f,  0.21f },
						{ -0.14f, 0.0f,  0.15f },
						{ -0.13f, 0.0f,  0.06f },
						{ -0.00f, 0.0f,  0.06f } };
	int soleCount = 21;
	int heelCount = 17;

	// Viewport 2.0 specific data
	//
	const MString colorParameterName_ = "solidColor";
	const MString wireframeItemName_  = "footPrintLocatorWires";
	const MString shadedItemName_     = "footPrintLocatorTriangles";

	// Maintain a mini cache for 3d solid shaders in order to reuse the shader
	// instance whenever possible. This can allow Viewport 2.0 optimization e.g.
	// the GPU instancing system and the consolidation system to be leveraged.
	//
	struct MColorHash
	{
		std::size_t operator()(const MColor& color) const
		{
			std::size_t seed = 0;
			CombineHashCode(seed, color.r);
			CombineHashCode(seed, color.g);
			CombineHashCode(seed, color.b);
			CombineHashCode(seed, color.a);
			return seed;
		}

		void CombineHashCode(std::size_t& seed, float v) const
		{
			std::hash<float> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};

    std::unordered_map<MColor, MHWRender::MShaderInstance*, MColorHash> the3dSolidShaders;

	MHWRender::MShaderInstance* get3dSolidShader(const MColor& color)
	{
		// Return the shader instance if exists.
		auto it = the3dSolidShaders.find(color);
		if (it != the3dSolidShaders.end())
		{
			return it->second;
		}

		MHWRender::MShaderInstance* shader = NULL;

		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		if (renderer)
		{
			const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
			if (shaderMgr)
			{
				shader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
			}
		}

		if (shader)
		{
			float solidColor[] = { color.r, color.g, color.b, 1.0f };
			shader->setParameter(colorParameterName_, solidColor);

			the3dSolidShaders[color] = shader;
		}

		return shader;
	}

	MStatus releaseShaders()
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		if (renderer)
		{
			const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
			if (shaderMgr)
			{
				for (auto it = the3dSolidShaders.begin(); it != the3dSolidShaders.end(); it++)
				{
					shaderMgr->releaseShader(it->second);
				}

				the3dSolidShaders.clear();
				return MS::kSuccess;
			}
		}

		return MS::kFailure;
	}

    // Technique 2: Store per-instance draw information (such as if a given instance is 
    // selected).  Set up a data structure to hold this information.
    // 
    // VP2 Custom Caching: This information does not need to be stored in context aware storage
    // because this information is only used in requiresUpdateRenderItems() and
    // updateRenderItems() and those methods are not invoked from the background thread
    // during for VP2 Custom Caching.
    struct VP2InstanceDrawInfo
    {
        VP2InstanceDrawInfo() : mDisplayStatus(MHWRender::DisplayStatus::kNoStatus) {}

        MHWRender::DisplayStatus mDisplayStatus;
        MColor mDisplayColor;
    };

    // Technique 2: Use a map to store the instance data rather than a vector because the
    // MDagPath::instanceNumber() is not necessarily monotonically increasing and starting
    // at 0.
    typedef std::unordered_map<unsigned int, VP2InstanceDrawInfo*> VP2InstancesDrawInfo;

    struct VP2DrawInfo
    {
        VP2DrawInfo() : mCallbackInitialized(false) {}

        VP2InstancesDrawInfo mInstanceInfo;
        MCallbackId mInstanceAddedCallbackId;
        MCallbackId mInstanceRemovedCallbackId;
        bool mCallbackInitialized;
    };
} // anonymous namespace

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Node implementation with standard viewport draw
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class footPrint : public MPxLocatorNode
{
public:
	footPrint();
	~footPrint() override;

	bool            isBounded() const override;
	MBoundingBox    boundingBox() const override;

	MSelectionMask getShapeSelectionMask() const override;

    MStatus setDependentsDirty(const MPlug& plug, MPlugArray& plugArray) override;
    MStatus postEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode, PostEvaluationType evalType) override;

	static  void *          creator();
	static  MStatus         initialize();

	static  MObject         size;         // The size of the foot

    /*
        Technique 1: Use an internal attribute to store if any attribute which affects
        the geometry created by FootPrintGeometryOverride has changed since the last time
        FootPrintGeometryOverride was executed.  Storing this information here is
        breaking our abstraction.  footPrint has to know about details of how
        FootPrintGeometryOverride works that it really shouldn't know.

        However, attributes are stored in the MDataBlock and the MDataBlock is context
        aware storage, so internal attributes are a good way to communicate between the
        MPxNode and the MPxGeometryOverride which is safe to use with VP2 Custom Caching.
    */
    static  MObject         sizeChangedSinceVP2Update;

    void setSizeChangedSinceVP2Update(bool sizeChanged);
    bool getSizeChangedSinceVP2Update();
public:
	static	MTypeId		id;
	static	MString		drawDbClassification;
	static	MString		drawRegistrantId;
};

class FootPrintGeometryOverride : public MHWRender::MPxGeometryOverride
{
public:
    static MHWRender::MPxGeometryOverride* Creator(const MObject& obj)
    {
        return new FootPrintGeometryOverride(obj);
    }

    ~FootPrintGeometryOverride() override;

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    bool hasUIDrawables() const override { return false; }
    bool requiresUpdateRenderItems(const MDagPath& path) const override;
    bool supportsEvaluationManagerParallelUpdate() const override;
    bool supportsVP2CustomCaching() const override;
    bool requiresGeometryUpdate() const override;

    void updateDG() override;
    bool isIndexingDirty(const MHWRender::MRenderItem &item) override { return false; }
    bool isStreamDirty(const MHWRender::MVertexBufferDescriptor &desc) override { return mFootPrintNode->getSizeChangedSinceVP2Update(); }
    void updateRenderItems(const MDagPath &path, MHWRender::MRenderItemList& list) override;
    void populateGeometry(const MHWRender::MGeometryRequirements &requirements, const MHWRender::MRenderItemList &renderItems, MHWRender::MGeometry &data) override;
    void cleanUp() override {};

    

    /*
    Tracing will look something like the following when in shaded mode:

        footPrintGeometryOverride: Geometry override DG update: footPrint1
        footPrintGeometryOverride: Start geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: - Call API to update render items
        footPrintGeometryOverride: End geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: Start geometry override update stream and indexing data: footPrint1
        footPrintGeometryOverride: - Update render item: soleLocatorTriangles
        footPrintGeometryOverride: - Update render item: heelLocatorTriangles
        footPrintGeometryOverride: End geometry override stream and indexing data: footPrint1
        footPrintGeometryOverride: End geometry override clean up: footPrint1

        at creation time.

        footPrintGeometryOverride: Geometry override DG update: footPrint1
        footPrintGeometryOverride: Start geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: - Call API to update render items
        footPrintGeometryOverride: End geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: End geometry override clean up: footPrint1

        on selection change.

        footPrintGeometryOverride: Geometry override DG update: footPrint1
        footPrintGeometryOverride: Start geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: - Call API to update render items
        footPrintGeometryOverride: End geometry override render item update: |transform1|footPrint1
        footPrintGeometryOverride: Geometry override dirty stream check: footPrint1
        footPrintGeometryOverride: Start geometry override update stream and indexing data: footPrint1
        footPrintGeometryOverride: End geometry override stream and indexing data: footPrint1
        footPrintGeometryOverride: End geometry override clean up: footPrint1

    for footprint size change.

    This is based on the existing stream and indexing dirty flags being used
    which attempts to minimize the amount of render item, vertex buffer and indexing update.
    */
    bool traceCallSequence() const override
    {
        // Return true if internal tracing is desired.
        return false;
    }
    void handleTraceMessage(const MString &message) const override
    {
        MGlobal::displayInfo("footPrintGeometryOverride: " + message);

        // Some simple custom message formatting.
        fputs("footPrintGeometryOverride: ", stderr);
        fputs(message.asChar(), stderr);
        fputs("\n", stderr);
    }

    static void instancingChangedCallback(MDagPath& child, MDagPath& parent, void* clientData);

private:
    FootPrintGeometryOverride(const MObject& obj);
    void clearInstanceInfo();
    
    friend class footPrint;
    MObject mLocatorNode;
    float mMultiplier;

    // Technique 1: footPrint tracks when any attributes which affect the geometry change.
    // This information may be accessed at any time, so store a pointer to the associated
    // DAG node.
    footPrint* mFootPrintNode;
    
    // Technique 2: storage for last used values to track if render items need to update.
    VP2DrawInfo mVP2DrawInfo;
};

MObject footPrint::size;
MObject footPrint::sizeChangedSinceVP2Update;
MTypeId footPrint::id( 0x00080033 );
MString	footPrint::drawDbClassification("drawdb/geometry/light/footPrint_GeometryOverride");
static bool sMakeFootPrintDirLight = (getenv("MAYA_FOOTPRINT_GEOMETRY_OVERRIDE_AS_DIRLIGHT") != NULL);
static MString lightClassification("light:drawdb/geometry/light/footPrint_GeometryOverride:drawdb/light/directionalLight"); 
MString	footPrint::drawRegistrantId("FootprintNode_GeometryOverridePlugin");

footPrint::footPrint() {}
footPrint::~footPrint() {}

/*
    Technique 1: Use setDependentsDirty to tap into Maya's dirty propagation to track when
    the size plug changes so that FootPrintGeometryOverride can find out if it needs to
    update geometry.

    Warning: any time you implement setDependentsDirty you probably also need to implement
    something similar in preEvaluation() or postEvaluation() so the code works correctly 
    with Evaluation Manager enabled.
*/
MStatus footPrint::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
    if (plug.partialName() == "sz")
    {
        setSizeChangedSinceVP2Update(true);
    }

    return MStatus::kSuccess;
}

bool footPrint::isBounded() const
{
	return true;
}

MBoundingBox footPrint::boundingBox() const
{
	// Get the size
	//
	MObject thisNode = thisMObject();
	MPlug plug( thisNode, size );
	MDistance sizeVal;
	plug.getValue( sizeVal );

	double multiplier = sizeVal.asCentimeters();

	MPoint corner1( -0.17, 0.0, -0.7 );
	MPoint corner2( 0.17, 0.0, 0.3 );

	corner1 = corner1 * multiplier;
	corner2 = corner2 * multiplier;

	return MBoundingBox( corner1, corner2 );
}

MSelectionMask footPrint::getShapeSelectionMask() const
{
	return MSelectionMask("footPrintSelection");
}

/*
Technique 1: Use postEvaluation to tap into Evaluation Manager dirty information
to track when the size plug changes so that FootPrintNodeGeometryOverride can 
find out if it needs to update geometry.

Evaluation Caching: It is critical for Evaluation Caching that the EM dirty information
is accessed from postEvaluation rather than preEvaluation.  During Evaluation
Caching restore (or VP2 Custom Caching restore) preEvaluation will not be called,
causing the sizeChangedSinceVP2Update flag to be set incorrectly and preventing VP2
from updating to use the new data restored from the cache.

preEvaluation should be used to prepare for the drawing override calls
postEvaluation should be used to notify consumers of the data (VP2) that new data is ready.

Warning: any time you implement preEvaluation or postEvaluation and use dirtyPlugExists
you probably also need to implement something similar in setDependentsDirty() so
the code works correctly without Evaluation Manager.
*/
MStatus footPrint::postEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode, PostEvaluationType evalType)
{
    MStatus status;
    if (evaluationNode.dirtyPlugExists(size, &status) && status)
    {
        setSizeChangedSinceVP2Update(true);
    }

    return MStatus::kSuccess;
}

void footPrint::setSizeChangedSinceVP2Update(bool sizeChanged)
{
    /*
        Calling forceCache here should be fast.  Possible calling sites are:
         - setDependentsDirty() -> the normal context is current.
         - preparing the draw in VP2 -> the normal context is current.
         - background evaluation postEvaluation() -> datablock for background context already exists.
         - background evaluation for VP2 Custom Caching -> datablock for background context already exists.
    */
    MDataBlock block = forceCache();
    MDataHandle sizeChangedSinceVP2UpdateHandle = block.outputValue(sizeChangedSinceVP2Update);
    sizeChangedSinceVP2UpdateHandle.setBool(sizeChanged);
}

bool footPrint::getSizeChangedSinceVP2Update()
{
    MDataBlock block = forceCache();
    MDataHandle sizeChangedSinceVP2UpdateHandle = block.outputValue(sizeChangedSinceVP2Update);
    return sizeChangedSinceVP2UpdateHandle.asBool();
}

void* footPrint::creator()
{
	return new footPrint();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
FootPrintGeometryOverride::FootPrintGeometryOverride(const MObject& obj)
: MHWRender::MPxGeometryOverride(obj)
, mLocatorNode(obj)
, mMultiplier(0.0f)
, mFootPrintNode(nullptr)
{
    MStatus returnStatus;
    MFnDependencyNode dependNode(mLocatorNode, &returnStatus);
    if (returnStatus != MStatus::kSuccess) return;
    MPxNode* footPrintNode = dependNode.userNode(&returnStatus);
    if (returnStatus != MStatus::kSuccess) footPrintNode = nullptr;
    mFootPrintNode = dynamic_cast<footPrint*>(footPrintNode);
}

FootPrintGeometryOverride::~FootPrintGeometryOverride()
{
    // Technique 2: Clean up the memory allocated to store the per instance information.
    clearInstanceInfo();

    // Technique 2: Remove the callbacks added to track instancing changed messages.
    MMessage::removeCallback(mVP2DrawInfo.mInstanceAddedCallbackId);
    MMessage::removeCallback(mVP2DrawInfo.mInstanceRemovedCallbackId);
}

void FootPrintGeometryOverride::clearInstanceInfo()
{
    VP2InstancesDrawInfo& instanceInfo = mVP2DrawInfo.mInstanceInfo;
    for (auto it = instanceInfo.begin(); it != instanceInfo.end(); it++)
    {
        delete it->second;
    }
    instanceInfo.clear();
}

MHWRender::DrawAPI FootPrintGeometryOverride::supportedDrawAPIs() const
{
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

void FootPrintGeometryOverride::updateDG()
{
    // Technique 1: Only update mMultiplier when the footPrint node has signaled that mMultiplier has
    // changed.  In this trivial example it is possible to get the current value and compare it
    // to what is stored in mMultiplier.  In a real use case the value may be a poly
    // mesh with a million vertices, which would make such a comparison terrible for performance.
    if (mFootPrintNode->getSizeChangedSinceVP2Update())
    {
        MPlug plug(mLocatorNode, footPrint::size);
        if (!plug.isNull())
        {
            MDistance sizeVal;
            if (plug.getValue(sizeVal))
            {
                mMultiplier = (float)sizeVal.asCentimeters();
            }
        }
    }
}

void FootPrintGeometryOverride::instancingChangedCallback(MDagPath& child, MDagPath& parent, void* clientData)
{
    FootPrintGeometryOverride* geometryOverride = reinterpret_cast<FootPrintGeometryOverride*>(clientData);
    
    // Technique 2: Understanding the relationship between the list of old instances and the new instances
    // is very challenging.  Rather than writing some complex code to handle it, destroy all our per-instance
    // information.  This means updateRenderItems must be expected for all instances which may be slow.
    // Typically instancing changes are interactive changes triggered by the user, so this won't impact 
    // playback performance of the node.
    // 
    // If your plug-in is using a lot of DAG instancing and you need high performance you should consider using
    // MPxSubSceneOverride instead of MPxGeometryOverride.  MPxSubSceneOverride gives you the flexibility to make
    // the workflows which are important to you fast.
    geometryOverride->clearInstanceInfo();
}

bool FootPrintGeometryOverride::requiresUpdateRenderItems(const MDagPath& path) const
{
    // Technique 2: If the display status and display color have not changed, then don't do any
    // render item updates.  The first time updateRenderItems() is called the items must be added.
    // Initialize mDisplayStatus to kNoStatus to ensure this occurs.  The current display status
    // can never be kNoStatus.
    MStatus returnStatus;
    auto instanceDrawInfoIter = mVP2DrawInfo.mInstanceInfo.find(path.instanceNumber(&returnStatus));    
    if (returnStatus == MStatus::kFailure || (mVP2DrawInfo.mInstanceInfo.end() == instanceDrawInfoIter)) return true;

    const VP2InstanceDrawInfo* instanceDrawInfo = instanceDrawInfoIter->second;

    MHWRender::DisplayStatus currentDisplayStatus = MHWRender::MGeometryUtilities::displayStatus(path);
    MColor currentDisplayColor = MHWRender::MGeometryUtilities::wireframeColor(path);
    if (currentDisplayStatus == instanceDrawInfo->mDisplayStatus && currentDisplayColor == instanceDrawInfo->mDisplayColor) return false;

    return true;
}

bool FootPrintGeometryOverride::supportsEvaluationManagerParallelUpdate() const
{
    return true;
}

bool FootPrintGeometryOverride::supportsVP2CustomCaching() const
{
    return true;
}

bool FootPrintGeometryOverride::requiresGeometryUpdate() const
{
    return mFootPrintNode->getSizeChangedSinceVP2Update();
}

void FootPrintGeometryOverride::updateRenderItems( const MDagPath& path, MHWRender::MRenderItemList& list )
{
    // There should always be an instanceDrawInfo for path, because requiresUpdateRenderItems()
    // was called immediately before updateRenderItems() and requiresUpdateRenderItems() adds it.
    MStatus returnStatus;
    VP2InstanceDrawInfo* instanceDrawInfo = mVP2DrawInfo.mInstanceInfo[path.instanceNumber(&returnStatus)];
    if (returnStatus == MStatus::kFailure) return;

    if (!instanceDrawInfo)
    {
        instanceDrawInfo = new VP2InstanceDrawInfo;
        mVP2DrawInfo.mInstanceInfo[path.instanceNumber(&returnStatus)] = instanceDrawInfo;
        if (returnStatus == MStatus::kFailure) return;

        // Technique 2: Store information about each instance of the footPrint node.  If 
        // instances are added or removed then update our per-instance information.
        if (!mVP2DrawInfo.mCallbackInitialized)
        {
            mVP2DrawInfo.mCallbackInitialized = true;

            MDagPath& nonConstPath = const_cast<MDagPath&>(path);
            mVP2DrawInfo.mInstanceAddedCallbackId = MDagMessage::addInstanceAddedDagPathCallback(nonConstPath, &FootPrintGeometryOverride::instancingChangedCallback, this, &returnStatus);
            if (returnStatus == MStatus::kFailure) return;
            mVP2DrawInfo.mInstanceRemovedCallbackId = MDagMessage::addInstanceRemovedDagPathCallback(nonConstPath, &FootPrintGeometryOverride::instancingChangedCallback, this, &returnStatus);
            if (returnStatus == MStatus::kFailure) return;
        }
    }

    MHWRender::DisplayStatus displayStatus = MHWRender::MGeometryUtilities::displayStatus(path);
    MColor displayColor = MHWRender::MGeometryUtilities::wireframeColor(path);

    // Technique 2: updateRenderItems() is going to be called so record the values that 
    // affect which render items are drawn to avoid extracting the values from Maya twice.
    instanceDrawInfo->mDisplayStatus = displayStatus;
    instanceDrawInfo->mDisplayColor = displayColor;

    MHWRender::MShaderInstance* shader = get3dSolidShader(displayColor);
    if (!shader) return;

	unsigned int depthPriority;
	switch (displayStatus)
	{
	case MHWRender::kLead:
	case MHWRender::kActive:
	case MHWRender::kHilite:
	case MHWRender::kActiveComponent:
		depthPriority = MHWRender::MRenderItem::sActiveWireDepthPriority;
		break;
	default:
		depthPriority = MHWRender::MRenderItem::sDormantFilledDepthPriority;
		break;
	}

    MHWRender::MRenderItem* wireframeItem = NULL;

    int index = list.indexOf(wireframeItemName_);
    if (index < 0)
    {
        wireframeItem = MHWRender::MRenderItem::Create(
            wireframeItemName_,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines);
        wireframeItem->setDrawMode(MHWRender::MGeometry::kWireframe);
        list.append(wireframeItem);
    }
    else
    {
        wireframeItem = list.itemAt(index);
    }

    if(wireframeItem)
    {
		wireframeItem->setShader(shader);
		wireframeItem->depthPriority(depthPriority);
		wireframeItem->enable(true);
    }

    MHWRender::MRenderItem* shadedItem = NULL;

    index = list.indexOf(shadedItemName_);
    if (index < 0)
    {
		shadedItem = MHWRender::MRenderItem::Create(
            shadedItemName_,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kTriangles);
		shadedItem->setDrawMode((MHWRender::MGeometry::DrawMode)(MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured));
        list.append(shadedItem);
    }
    else
    {
		shadedItem = list.itemAt(index);
    }

    if(shadedItem)
    {
		shadedItem->setShader(shader);
		shadedItem->depthPriority(depthPriority);
		shadedItem->enable(true);
    }
}

void FootPrintGeometryOverride::populateGeometry(
    const MHWRender::MGeometryRequirements& requirements,
    const MHWRender::MRenderItemList& renderItems,
    MHWRender::MGeometry& data)
{
    MHWRender::MVertexBuffer* verticesBuffer       = NULL;

    float* vertices                     = NULL;

	const MHWRender::MVertexBufferDescriptorList& vertexBufferDescriptorList = requirements.vertexRequirements();
	const int numberOfVertexRequirments = vertexBufferDescriptorList.length();

	MHWRender::MVertexBufferDescriptor vertexBufferDescriptor;
	for (int requirmentNumber = 0; requirmentNumber < numberOfVertexRequirments; ++requirmentNumber)
	{
		if (!vertexBufferDescriptorList.getDescriptor(requirmentNumber, vertexBufferDescriptor))
		{
			continue;
		}

		switch (vertexBufferDescriptor.semantic())
		{
		case MHWRender::MGeometry::kPosition:
			{
				if (!verticesBuffer)
				{
					verticesBuffer = data.createVertexBuffer(vertexBufferDescriptor);
					if (verticesBuffer)
					{
						vertices = (float*)verticesBuffer->acquire(soleCount+heelCount, false);
					}
				}
			}
			break;
		default:
			// do nothing for stuff we don't understand
			break;
		}
	}

	int verticesPointerOffset = 0;

	// We concatenate the heel and sole positions into a single vertex buffer.
	// The index buffers will decide which positions will be selected for each render items.
	for (int currentVertex = 0 ; currentVertex < soleCount+heelCount; ++currentVertex)
	{
		if(vertices)
		{
			if (currentVertex < heelCount)
			{
				int heelVtx = currentVertex;
				vertices[verticesPointerOffset++] = heel[heelVtx][0] * mMultiplier;
				vertices[verticesPointerOffset++] = heel[heelVtx][1] * mMultiplier;
				vertices[verticesPointerOffset++] = heel[heelVtx][2] * mMultiplier;
			}
			else
			{
				int soleVtx = currentVertex - heelCount;
				vertices[verticesPointerOffset++] = sole[soleVtx][0] * mMultiplier;
				vertices[verticesPointerOffset++] = sole[soleVtx][1] * mMultiplier;
				vertices[verticesPointerOffset++] = sole[soleVtx][2] * mMultiplier;
			}
		}
	}

	if(verticesBuffer && vertices)
	{
		verticesBuffer->commit(vertices);
	}

	for (int i=0; i < renderItems.length(); ++i)
	{
		const MHWRender::MRenderItem* item = renderItems.itemAt(i);

		if (!item)
		{
			continue;
		}

		MHWRender::MIndexBuffer* indexBuffer = data.createIndexBuffer(MHWRender::MGeometry::kUnsignedInt32);

		if (item->name() == wireframeItemName_ )
		{
			int primitiveIndex = 0;
			int startIndex = 0;
			int numPrimitive = heelCount + soleCount - 2;
			int numIndex = numPrimitive * 2;

			unsigned int* indices = (unsigned int*)indexBuffer->acquire(numIndex);

			for (int i = 0; i < numIndex; )
			{
				if (i < (heelCount - 1) * 2)
				{
					startIndex = 0;
					primitiveIndex = i / 2;
				}
				else
				{
					startIndex = heelCount;
					primitiveIndex = i / 2 - heelCount + 1;
				}
				indices[i++] = startIndex + primitiveIndex;
				indices[i++] = startIndex + primitiveIndex + 1;
			}

			indexBuffer->commit(indices);
		}
		else if (item->name() == shadedItemName_ )
		{
			int primitiveIndex = 0;
			int startIndex = 0;
			int numPrimitive = heelCount + soleCount - 4;
			int numIndex = numPrimitive * 3;

			unsigned int* indices = (unsigned int*)indexBuffer->acquire(numIndex);

			for (int i = 0; i < numIndex; )
			{
				if (i < (heelCount - 2) * 3)
				{
					startIndex = 0;
					primitiveIndex = i / 3;
				}
				else
				{
					startIndex = heelCount;
					primitiveIndex = i / 3 - heelCount + 2;
				}
				indices[i++] = startIndex;
				indices[i++] = startIndex + primitiveIndex + 1;
				indices[i++] = startIndex + primitiveIndex + 2;
			}

			indexBuffer->commit(indices);
		}

		item->associateWithIndexBuffer(indexBuffer);
	}

    // Technique 1: Now that the current geometry reflects the current value of the size
    // attribute, clear the signal flag.
    mFootPrintNode->setSizeChangedSinceVP2Update(false);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Plugin Registration
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

MStatus footPrint::initialize()
{
	MFnUnitAttribute unitFn;
	MStatus			 stat;

	size = unitFn.create( "size", "sz", MFnUnitAttribute::kDistance );
	unitFn.setDefault( 1.0 );
    stat = addAttribute(size);

    if (!stat) {
        stat.perror("addAttribute");
        return stat;
    }

    /*
        VP2 Custom Caching: When using VP2 custom caching the MPxGeometryOverride associated
        with the footPrint node might be invoked in the normal context or in the background
        context.  If playback and background evaluation occur concurrently Maya guarantees
        that all of the MPxGeometryOverride methods called for a context occur atomically
        without being interleaved with MPxGeometryOverride methods for the same object in
        a different context.

        However, Maya does not make any timing guarantee between the call(s) to evaluate the
        MPxNode and calls to the MPxGeometryOverride.  For example, a postEvaluation call 
        in the background evaluation thread may occur at the same time that the foreground
        thread is using the MPxGeometryOverride.

        This means that any communication which occurs between the MPxNode during evaluation
        and the MPxGeometryOverride must be context aware.  The communication channel must
        use different storage for each context.  The easiest way to implement this to use 
        internal attributes on the MPxNode that the MPxGeometryOverride has access to.

        footPrint uses this technique here.  Don't create any affects relationships because
        sizeChangedSinceVP2Update doesn't use any Maya dirty management or evaluation.  Only
        access sizeChangedSinceVP2Update by getting the MDataHandle directly from the
        MDataBlock using outputValue().
    */
    MFnNumericAttribute attrFn;
    sizeChangedSinceVP2Update = attrFn.create("sizeChangedSinceVP2Update", "sd", MFnNumericData::kBoolean, true);
    attrFn.setStorable(false);
    attrFn.setHidden(true);
    attrFn.setConnectable(false);
    stat = addAttribute(sizeChangedSinceVP2Update);
    if (!stat) {
        stat.perror("addAttribute");
        return stat;
    }

	return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode(
				"footPrint_GeometryOverride",
				footPrint::id,
				&footPrint::creator,
				&footPrint::initialize,
				MPxNode::kLocatorNode,
				(sMakeFootPrintDirLight ? &lightClassification : &footPrint::drawDbClassification)); 
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = MHWRender::MDrawRegistry::registerGeometryOverrideCreator(
		footPrint::drawDbClassification,
		footPrint::drawRegistrantId,
		FootPrintGeometryOverride::Creator);
	if (!status) {
		status.perror("registerDrawOverrideCreator");
		return status;
	}

	// Register a custom selection mask with priority 2 (same as locators
	// by default).
	MSelectionMask::registerSelectionType("footPrintSelection", 2);
	status = MGlobal::executeCommand("selectType -byName \"footPrintSelection\" 1");
	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = MHWRender::MDrawRegistry::deregisterGeometryOverrideCreator(
		footPrint::drawDbClassification,
		footPrint::drawRegistrantId);
	if (!status) {
		status.perror("deregisterDrawOverrideCreator");
		return status;
	}

	status = releaseShaders();
	if (!status) {
		status.perror("releaseShaders");
		return status;
	}

	status = plugin.deregisterNode( footPrint::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	// Deregister custom selection mask
	MSelectionMask::deregisterSelectionType("footPrintSelection");

	return status;
}
