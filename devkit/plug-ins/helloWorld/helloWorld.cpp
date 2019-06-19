//
// Created by Derek Roberts on 2019-06-19.
//
#include <maya/MIOStream.h>
#include <maya/MSimple.h>

DeclareSimpleCommand( helloWorld, PLUGIN_COMPANY, "4.5")

MStatus helloWorld::doIt(const MArgList &) {
    cout << "Sup" << "\n";
    return MS::kSuccess;
}
