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
    MPoint getProjectedPoint(unsigned i, MPoint* prevPoint, MPoint* currentPoint);
    MPoint getAcceleration();
    MStatus setVelocity(MPoint* prePoint, MPoint* cPoint);
    MStatus setNextActualPoints();
    unsigned getMeshVertices(MTime& frame);
    unsigned m_vpIndex = 0;
    double m_startFrame = 1;
    double m_endFrame = 50;
    MDGModifier m_dagModifier;
    std::vector <MPoint> m_prevPoints;
    std::vector <MPoint> m_projPoints;
    std::vector <MPoint> m_velocities;
    MAnimControl m_animControl;
    unsigned m_prevMeshVertices;
    unsigned m_currentMeshVertices;
    unsigned m_nextMeshVertices;
    MTime m_prevFrame;
    MTime m_currentFrame;
    MTime m_nextFrame;
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

MPoint JointRigAnimateCommand::getAcceleration()
{
    MPoint currentAcceleration;

    currentAcceleration = m_velocities[1] - m_velocities[0];

    cout << "Current Acceleration: " << currentAcceleration.x << ", " << currentAcceleration.y << ", "
        << currentAcceleration.z << endl;

    return currentAcceleration;
}

MStatus JointRigAnimateCommand::setVelocity(MPoint* prePoint, MPoint* cPoint)
{
    MPoint currentVelocity;
    if(prePoint != NULL && cPoint != NULL)
    {
        MPoint currentPoint = *cPoint;
        MPoint prevPoint = *prePoint;

        currentVelocity = currentPoint - prevPoint;

        if ((int)m_currentFrame.value() % 3 == 0)
            m_velocities.clear();

        m_velocities.push_back(currentVelocity);

        cout << "Current Velocity: " << currentVelocity.x << ", " << currentVelocity.y << ", " << currentVelocity.z
            << endl;
    }
    return MS::kSuccess;
}

MPoint JointRigAnimateCommand::getProjectedPoint(unsigned vi, MPoint* prePoint, MPoint* cPoint)
{
    MPoint projPoint;
    int i;

    if(vi == 0)
       i = 2;
    else
        i = 3;

    if(prePoint != NULL && cPoint != NULL)
    {
        MPoint currentPoint = *cPoint;
        MPoint prevPoint = *prePoint;
        setVelocity(prePoint, cPoint);
        MPoint currentVelocity = m_velocities[i];
        MPoint acceleration = getAcceleration();

        projPoint.x = currentPoint.x + currentVelocity.x + (acceleration.x / 2);
        projPoint.y = currentPoint.y + currentVelocity.y + (acceleration.y / 2);
        projPoint.z = currentPoint.z + currentVelocity.z + (acceleration.z / 2);

    }

    return projPoint;
}

MStatus JointRigAnimateCommand::vertUpdateByClosest(std::vector<MPoint>& locations)
{
    cout << "\nvertUpdateByClosest() has been called." << endl;

    //Get the mesh by selection.
    MStatus status;
    MDagPath node;
    MObject component;
    MSelectionList meshList;
    MGlobal::selectByName("mesh_fdv");
    MGlobal::getActiveSelectionList(meshList);
    meshList.getDagPath(0, node, component);
    MFnMesh mesh(node, &status);

    //Use p1 and p2 to match the proper index values in
    //m_refrencePoints based on which vertexPair we're
    //working with.
    unsigned p1, p2;
    unsigned vi = 0;

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
        MPoint current;
        MPoint projected;

        cout << "m_projPoints size = " << m_projPoints.size() << endl;
        cout << "m_prevPoints size = " << m_prevPoints.size() << endl;

        //m_prevPoints and m_projPoint should hold all 3 vertex pairs.
        cout << "Previous Point " << i << ": " << m_prevPoints[i].x << ", " << m_prevPoints[i].y << ", "
             << m_prevPoints[i].z << endl;
        cout << "Previous Projected Point " << i << ": " << m_projPoints[i].x << ", " << m_projPoints[i].y << ", "
             << m_projPoints[i].z << endl;

        //Get closest current point on the mesh
        //to the projected point cached in the previous frame.
        mesh.getClosestPoint(m_projPoints[i], current, MSpace::kWorld);

        cout << "Actual Current Point: " << current.x << ", " << current.y << ", " << current.z << endl;

        projected = getProjectedPoint(i, &m_prevPoints[i], &current);

        cout << "Projected Point " << i << ": " << projected.x << ", " << projected.y << ", "
             << projected.z << endl;

        //Replace points.
        m_prevPoints.erase(m_prevPoints.begin()+i);
        m_prevPoints.emplace(m_prevPoints.begin()+i, current);
        m_projPoints.erase(m_projPoints.begin()+i);
        m_projPoints.emplace(m_projPoints.begin()+i, projected);
        locations.push_back(current);

        vi++;
    }

    return status;
}

MStatus JointRigAnimateCommand::setNextActualPoints()
{
    cout << "setNextActualPoint() has been called." << endl;
    //Select the vertex pair by name.
    MStatus status;
    MDagPath node;
    MObject component;
    MSelectionList vertexPair;
    MString vpArg;
    vpArg.set(m_vpIndex + 1);
    MString vpName = "vp" + vpArg;
    m_animControl.setCurrentTime(m_nextFrame);
    MGlobal::executeCommand("select " + vpName);
    MGlobal::getActiveSelectionList(vertexPair);
    unsigned i;

    switch(m_vpIndex) {
        case 0: i = 0;
                break;
        case 1: i = 2;
                break;
        case 2: i = 4;
                break;
    }

    //Get the mesh.
    vertexPair.getDagPath(0, node, component);
    MFnMesh mesh(node, &status);

    //Get the indices for the selected vertices so
    //we can iterate through them.
    MItMeshVertex vertIt(node, component, &status);

    for (; !vertIt.isDone(); vertIt.next())
    {
        MPoint next;
        int vid = vertIt.index(&status);
        mesh.getPoint(vid, next, MSpace::kWorld);

        //Push to m_projPoints for vertUpdateByClosest() to use.
        if(m_currentFrame.value() > m_startFrame)
        {
            m_projPoints.erase(m_projPoints.begin()+i);
            m_projPoints.emplace(m_projPoints.begin()+i, next);
        }else {
            m_projPoints.push_back(next);
        }
        cout << "\nNext Point " << i << ": " << m_projPoints[i].x << ", " << m_projPoints[i].y << ", "
             << m_projPoints[i].z << endl;
        i++;
    }

    //Time to go back to the present.
    m_animControl.setCurrentTime(m_currentFrame);

    return status;
}

MStatus JointRigAnimateCommand::vertUpdateBySelection(std::vector<MPoint>& locations)
{
    cout << "\nvertUpdateBySelection() has been called." << endl;
    //Select the vertex pair by name.
    MStatus status;
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

    unsigned i;

    switch(m_vpIndex) {
        case 0: i = 0;
            break;
        case 1: i = 2;
            break;
        case 2: i = 4;
            break;
    }

    for (; !vertIt.isDone(); vertIt.next())
    {
        MPoint current;
        int vid = vertIt.index(&status);
        mesh.getPoint(vid, current, MSpace::kWorld);
        locations.push_back(current);

        //Push to m_prevPoints for vertUpdateByClosest() to use.
        if(m_currentFrame.value() > m_startFrame)
        {
            setVelocity(&m_prevPoints[i], &current);
            m_prevPoints.erase(m_prevPoints.begin()+i);
            m_prevPoints.emplace(m_prevPoints.begin()+i, current);
        }else {
            m_prevPoints.push_back(current);
        }

        cout << "Vertex ID: " << vid << endl;
        cout << "New Location: " << current.x << ", " << current.y << ", " << current.z << endl;
        i++;
    }

    setNextActualPoints();
    return status;
}

MVector JointRigAnimateCommand::centroid()
{
    MStatus status;
    MVector centroid;
    MTime thirdFrame((double) m_startFrame + 2.0);
    cout << "m_currentFrame = " << m_currentFrame.value() << ", thirdFrame = " << thirdFrame.value() << endl;
    if(isNewMesh)
        cout << "isNewMesh" << endl;
    else
        cout << "!isNewMesh" << endl;

    if(isNewMeshNext)
        cout << "isNewMeshNext" << endl;
    else
        cout << "!isNewMeshNext" << endl;

    //Holds one vertex pair at a time.
    std::vector <MPoint> locations;

    if(m_currentFrame < thirdFrame)
        vertUpdateBySelection(locations);
    else
        vertUpdateByClosest(locations);

    /*
    //Update the vertex pair locations.
    if(isNewMesh && !isNewMeshNext)
        vertUpdateByClosest(locations);

    if(!isNewMesh && isNewMeshNext)
        vertUpdateBySelection(locations);

    if(isNewMesh && isNewMeshNext)
        vertUpdateByClosest(locations);

    if(!isNewMesh && !isNewMeshNext)
    {
        if(m_currentFrame == firstFrame)
            vertUpdateBySelection(locations);
        else
            vertUpdateByClosest(locations);
    }
    */

    centroid = (locations[0] + locations[1]) / 2;

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
    m_prevFrame.setValue((double)i-1);
    m_currentFrame.setValue((double)i);
    m_nextFrame.setValue((double)i+1);

    //Used in centroid to update vertices
    //by selection on first frame.

    if(i > 1)
        m_prevMeshVertices = getMeshVertices(m_prevFrame);

    m_currentMeshVertices = getMeshVertices(m_currentFrame);
    m_nextMeshVertices = getMeshVertices(m_nextFrame);

    m_animControl.setCurrentTime(m_currentFrame);
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
        for(unsigned i = m_startFrame; i < m_endFrame; i++)
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
                     << translation.y << " y, " << translation.z << " z) \n\n";

                //Set new location
                location.setTranslation(centroid(), MSpace::kWorld);
                MVector newTranslation = location.getTranslation(MSpace::kWorld);
                cout << "\nJoint " << j+1 << "'s new location is (" << newTranslation.x << " x, "
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