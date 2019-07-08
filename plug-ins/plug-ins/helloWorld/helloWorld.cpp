//
// Created by Derek Roberts on 2019-06-19.
//
#include <maya/MIOStream.h>
#include <maya/MSimple.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnIkJoint.h>

DeclareSimpleCommand( helloWorld, PLUGIN_COMPANY, "4.5")

MStatus helloWorld::doIt(const MArgList &) {
    cout << "Sup" << "\n";
    MStatus stat;
    MDagModifier dagMod;
    MFnIkJoint myJointFn;
    MDagPath jointDagPath;
    MObject jointObject = dagMod.createNode("Joint", MObject::kNullObj, &stat);
    myJointFn.setObject(jointObject);
    myJointFn.getPath(jointDagPath);
    cout <<  << endl;
    return MS::kSuccess;
}
