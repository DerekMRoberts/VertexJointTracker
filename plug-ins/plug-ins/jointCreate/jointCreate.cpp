//
// Created by Derek Roberts on 2019-06-27.
//
#include <vector>
#include <string>

#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>
#include <maya/MDagModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnIkEffector.h>
#include <maya/MAngle.h>
#include <maya/MArgParser.h>
#include <maya/MArgList.h>
#include <maya/MEulerRotation.h>
#include <maya/MSyntax.h>

class jointCreateCommand: public MPxCommand
{
    public:
        jointCreateCommand();
        ~jointCreateCommand() override;
        MStatus	parseArgs(const MArgList& args);
        MStatus doIt (const MArgList& args) override;
        MStatus redoIt() override;
        MStatus undoIt() override;
        MSyntax cmdSyntax();
        bool isUndoable() const override;
        static void* creator();

    private:
        const char *kLengthFlag = "-l";
        const char *kLengthLongFlag = "-length";
        unsigned int m_length = 3;
        unsigned int m_defaultLength = 3;
        int m_jointOrientation = 20;
        unsigned int m_jointDistance = 5;
        MDagModifier m_dagModifier;
        std::vector<MObject> m_jointObjects;
};
 jointCreateCommand::jointCreateCommand() {}
 jointCreateCommand::~jointCreateCommand() {}

void* jointCreateCommand::creator()
{
    return new jointCreateCommand;
}

bool jointCreateCommand::isUndoable() const
{
    return true;
}

MStatus jointCreateCommand::undoIt()
{
    return MS::kSuccess;
}

MStatus jointCreateCommand::parseArgs(const MArgList& args)
{
    MStatus status;
    MArgParser argData(cmdSyntax(), args, &status);
    unsigned int flagValue;

    if(args.asString(0) == "-l")
    {
        flagValue = args.asInt(1);
        if(flagValue > m_defaultLength)
            m_length = flagValue;
    }
    return status;
}

MStatus jointCreateCommand::doIt(const MArgList &args)
{
    MStatus status = parseArgs(args);
    if (status != MS::kSuccess) {
        return status;
    }

    for(unsigned int i = 0; i < m_length; i++)
    {
        if(i == 0) {
            MObject newJointObj = m_dagModifier.createNode("joint", MObject::kNullObj, &status);
            m_jointObjects.push_back(newJointObj);
        }
        else {
            MObject newJointObj = m_dagModifier.createNode("joint", m_jointObjects[i-1], &status);
            m_jointObjects.push_back(newJointObj);
        }
    }
    return redoIt();
}

MStatus jointCreateCommand::redoIt()
{
    MFnIkJoint jointFn;

    for(unsigned long i = 0; i < m_jointObjects.size(); i++)
    {
        //Set Rotation
        jointFn.setObject(m_jointObjects[i]);
        MAngle rotationAngle(m_jointOrientation, MAngle::kDegrees);
        MEulerRotation eulerRotation(rotationAngle.asRadians(), 0, 0, MEulerRotation::kXYZ);
        jointFn.setOrientation(eulerRotation);

        //Set Position/Translation
        MVector translation(0, m_jointDistance, 0);
        jointFn.setTranslation(translation, MSpace::kTransform);

    }

    //Create an IKHandle with a MEL command
    MString endEffectorArgument;
    endEffectorArgument.set(m_jointObjects.size());
    MString command = "ikHandle -sj joint1 -ee joint" + endEffectorArgument;
    m_dagModifier.commandToExecute(command);

    m_dagModifier.doIt();
    return MS::kSuccess;
}

MSyntax jointCreateCommand::cmdSyntax()
{
    MSyntax syntax;
    syntax.addFlag(kLengthFlag, kLengthLongFlag, MSyntax::kUnsigned);
    return syntax;
}

MStatus initializePlugin(MObject obj)
{
    MStatus stat;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

    stat = plugin.registerCommand( "createJoint", jointCreateCommand::creator);
    if (!stat) {
        stat.perror("registerCommand");
        return stat;
    }
    return stat;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus   stat;
    MFnPlugin plugin( obj );

    stat = plugin.deregisterCommand( "createJoint" );
    if (!stat) {
        stat.perror("deregisterCommand");
    }
    return stat;
}