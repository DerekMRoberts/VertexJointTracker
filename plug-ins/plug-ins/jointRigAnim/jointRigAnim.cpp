//
// Created by Derek Roberts on 2019-07-09.
//
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MFnPlugin.h>
#include <maya/MPlug.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnMesh.h>
#include <maya/MAnimControl.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnSet.h>

#include <vector>
#include <math.h>

class JointRigAnimateCommand: public MPxCommand
{
public:
    JointRigAnimateCommand();
    ~JointRigAnimateCommand() override;
    MStatus	parseArgs(const MArgList& args);
    MStatus doIt (const MArgList& args) override;
    MStatus undoIt() override;
    MSyntax cmdSyntax();
    bool isUndoable() const override;
    static void* creator();
private:
    MVector centroid();
    MStatus meshBufferOps(unsigned i);
    MStatus vertUpdateByClosest(std::vector<MPoint>& locations);
    MStatus vertUpdateBySelection(std::vector<MPoint>& locations);
    unsigned getMeshVertices(MTime& frame);
    unsigned m_vpIndex = 0;
    double m_startFrame;
    double m_endFrame;
    MDGModifier m_dagModifier;
    std::vector <MPoint> m_referencePoints;
    MAnimControl m_animControl;
    unsigned m_prevMeshVertices;
    unsigned m_currentMeshVertices;
    unsigned m_nextMeshVertices;
    unsigned m_currentFrame;
    const char *kStartFrame;
    const char *kEndFrame;
    bool isNewMeshNext = false;
    bool isNewMesh = false;
};

JointRigAnimateCommand::JointRigAnimateCommand() {}
JointRigAnimateCommand::~JointRigAnimateCommand() {}

void* JointRigAnimateCommand::creator()
{
    return new JointRigAnimateCommand;
}

bool JointRigAnimateCommand::isUndoable() const
{
    return true;
}

MStatus JointRigAnimateCommand::undoIt()
{
    return MS::kSuccess;
}

MStatus JointRigAnimateCommand::parseArgs(const MArgList &args)
{
    //~~~~~~~~~FUNCTION NOT USED~~~~~~~~~~~//
    //Couldn't quite figure this out in the alotted time.
    //Tried to make it so that the user could put in the
    //start or endframe argument in either order.

    MStatus status;
    MArgParser argData(cmdSyntax(), args, &status);

    if(args.asString(0) == "-s")
    {
        if(args.asDouble(1) >= 0)
            m_startFrame = args.asDouble(1);
    }
    if(args.asString(0) == "-e")
    {
        if(args.asDouble(1) > 0 && args.asDouble(1) > m_startFrame)
            m_endFrame = args.asDouble(1);
    }
    if(args.asString(3) == "-s")
    {
        if(args.asDouble(4) >= 0)
            m_startFrame = args.asDouble(4);
    }
    if(args.asString(3) == "-e")
    {
        if(args.asDouble(4) > 0 && args.asDouble(4) > m_startFrame)
            m_endFrame = args.asDouble(4);
    }
    return status;
}

MStatus JointRigAnimateCommand::vertUpdateByClosest(std::vector<MPoint>& locations)
{
    //Get the mesh by selection.
    MDagPath node;
    MObject component;
    MStatus status;
    MSelectionList meshList;
    MGlobal::selectByName("mesh_fdv");
    MGlobal::getActiveSelectionList(meshList);
    meshList.getDagPath(0, node, component);
    MFnMesh mesh(node, &status);

    //Use p1 and p2 to match the proper index values in
    //m_refrencePoints based on which vertexPair we're
    //working with.
    unsigned p1, p2;

    switch(m_vpIndex) {
        case 0: p1 = 0;
            p2 = 1;
            break;
        case 1: p1 = 2;
            p2 = 3;
            break;
        case 2: p1 = 4;
            p2 = 5;
            break;
    }

    for(unsigned i = p1; i < p2+1; i++)
    {
        MPoint temp;

        //m_referencePoints should hold all 3 vertex pairs.
        cout << "Reference Point " << i << ": " << m_referencePoints[i].x << ", " << m_referencePoints[i].y << ", "
             << m_referencePoints[i].z << endl;

        //Get closest current point on the mesh
        //to the point cached in the previous frame.
        mesh.getClosestPoint(m_referencePoints[i], temp, MSpace::kWorld);
        locations.push_back(temp);

        //Replace points.
        m_referencePoints.erase(m_referencePoints.begin()+i);
        m_referencePoints.emplace(m_referencePoints.begin()+i, temp);

        cout << "Temp Location: " << temp.x << ", " << temp.y << ", " << temp.z << endl;
    }
    return status;
}

MStatus JointRigAnimateCommand::vertUpdateBySelection(std::vector<MPoint>& locations)
{
    //Select the vertex pair by name.
    MStatus status;
    int vid;
    MDagPath node;
    MObject component;
    MSelectionList vertexPair;
    MString vpArg;
    vpArg.set(m_vpIndex + 1);
    MString vpName = "vp" + vpArg;
    MGlobal::executeCommand("select " + vpName);
    MGlobal::getActiveSelectionList(vertexPair);

    //Get the mesh.
    vertexPair.getDagPath(0, node, component);
    MFnMesh mesh(node, &status);

    //Get the indices for the selected vertices so
    //we can iterate through them.
    MItMeshVertex vertIt(node, component, &status);

    for (; !vertIt.isDone(); vertIt.next())
    {
        MPoint temp;
        vid = vertIt.index(&status);
        mesh.getPoint(vid, temp, MSpace::kWorld);
        locations.push_back(temp);

        //Push to m_referencePoints for vertUpdateByClosest() to use.
        m_referencePoints.push_back(temp);

        cout << "Vertex ID: " << vid << endl;
        cout << "Location: " << temp.x << ", " << temp.y << ", " << temp.z << endl;
    }
    return status;
}

MVector JointRigAnimateCommand::centroid()
{
    MStatus status;
    MVector centroid;

    //Holds one vertex pair at a time.
    std::vector <MPoint> locations;

    //Update the vertex pair locations.
    if(isNewMesh && !isNewMeshNext)
        vertUpdateByClosest(locations);

    if(!isNewMesh && isNewMeshNext)
        vertUpdateBySelection(locations);

    if(isNewMesh && isNewMeshNext)
        vertUpdateByClosest(locations);

    if(!isNewMesh && !isNewMeshNext)
    {
        if(m_currentFrame == 1)
            vertUpdateBySelection(locations);
        else
            vertUpdateByClosest(locations);
    }

    //Calculate x value of centroid.
    if(locations[0].x < locations[1].x)
        centroid.x = locations[0].x + (abs(locations[1].x - locations[0].x) / 2);
    else
        centroid.x = locations[0].x - (abs(locations[0].x - locations[1].x) / 2);

    //Calculate y value of centroid.
    if(locations[0].y < locations[1].y)
        centroid.y = locations[0].y + (abs(locations[1].y - locations[0].y) / 2);
    else
        centroid.y = locations[0].y - (abs(locations[0].y - locations[1].y) / 2);

    //Calculate z value of centroid.
    if(locations[0].z < locations[1].z)
        centroid.z = locations[0].z + (abs(locations[1].z - locations[0].z) / 2);
    else
        centroid.z = locations[0].z - (abs(locations[0].z - locations[1].z) / 2);

    cout << "Centroid : (" << centroid.x << ", " << centroid.y << ", " << centroid.z << ")" << endl;

    //Clear locations for next vertex pair.
    locations.pop_back();
    locations.pop_back();

    if(m_vpIndex < 2)
        m_vpIndex++;
    else
        m_vpIndex = 0;

    MString vp;
    vp.set(m_vpIndex+1);
    MGlobal::unselectByName("vp" + vp);
    return centroid;
}

unsigned JointRigAnimateCommand::getMeshVertices(MTime& frame)
{
    MStatus status;
    MDagPath node;
    MObject component;
    MSelectionList meshList;

    m_animControl.setCurrentTime(frame);
    MGlobal::selectByName("mesh_fdv");
    MGlobal::getActiveSelectionList(meshList);
    meshList.getDagPath(0, node, component);
    MFnMesh mesh(node, &status);

    return mesh.numVertices();
}

MStatus JointRigAnimateCommand::meshBufferOps(unsigned i)
{
    MStatus status;
    MDagPath node;
    MObject component;
    MTime prevFrame((double)i-1);
    MTime currentFrame((double)i);
    MTime nextFrame((double)i+1);

    //Used in centroid to update vertices
    //by selection on first frame.
    m_currentFrame = i;

    if(i > 1)
        m_prevMeshVertices = getMeshVertices(prevFrame);

    m_currentMeshVertices = getMeshVertices(currentFrame);
    m_nextMeshVertices = getMeshVertices(nextFrame);

    m_animControl.setCurrentTime(currentFrame);
    cout << "\n---------\n";
    cout << "Frame: " << m_animControl.currentTime() << "\n";
    cout << "---------\n";

    return status;
}

MStatus JointRigAnimateCommand::doIt(const MArgList& args)
{
    MStatus status;
    MDagPath node;
    MObject component;
    MSelectionList locGroup;
    MFnDagNode nodeFn;
    MGlobal::selectByName(/*"*joint*"*/"*locator*");
    MGlobal::getActiveSelectionList(locGroup);
    cout << "locGroup size: " << locGroup.length() << "\n";

    if(!locGroup.isEmpty())
    {
        for(unsigned i = /*m_startFrame*/1; i < 20/*m_endFrame+1*/; i++)
        {
            meshBufferOps(i);

            //Using total number of vertices in mesh
            //to figure out if it's a brand new mesh
            //or not.
            if(m_currentMeshVertices == m_nextMeshVertices && m_prevMeshVertices == m_currentMeshVertices) {
                isNewMeshNext = false;
                isNewMesh = false;
            }
            if(m_currentMeshVertices != m_nextMeshVertices)
                isNewMeshNext = true;
            if(m_prevMeshVertices != m_currentMeshVertices)
                isNewMesh = true;

            for (unsigned j = 0; j < 3; j++)
            {
                locGroup.getDagPath(j, node, component);
                nodeFn.setObject(node);
                MFnTransform location(node);

                cout << "-----------------------------------------------------------" << endl;
                cout << "Joint " << j+1 << ":\n" << endl;

                //Get current location of the joint
                MVector translation = location.getTranslation(MSpace::kWorld);
                cout << "Joint " << j+1 << "'s old location is (" << translation.x << " x, "
                     << translation.y << " y, " << translation.z << " z) \n";

                //Set new location
                location.setTranslation(centroid(), MSpace::kWorld);
                MVector newTranslation = location.getTranslation(MSpace::kWorld);
                cout << "Joint " << j+1 << "'s new location is (" << newTranslation.x << " x, "
                     << newTranslation.y << " y, " << newTranslation.z << " z) \n";
            }

            //Set the keyframe.
            MGlobal::selectByName(/*"*joint*"*/"*locator*");
            MGlobal::executeCommand("setKeyframe");
        }
    }
    return MS::kSuccess;
}

MSyntax JointRigAnimateCommand::cmdSyntax()
{
    MSyntax syntax;
    syntax.addFlag(kStartFrame, kEndFrame, MSyntax::kUnsigned);
    return syntax;
}

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

    status = plugin.registerCommand( "jointRig", JointRigAnimateCommand::creator);
    if (!status)
    {
        status.perror("registerCommand");
        return status;
    }
    return status;
}

MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterCommand("jointRig");
    if (!status) {
        status.perror("deregisterCommand");
    }
    return status;
}