//
// Created by Derek Roberts on 2019-06-20
//
// This plug-in takes three user created locators placed at joint locations,
// computes the distance between the locators, computes the local joint angle
// at the middle joint, and outputs the local angle in the console.
//
// Initially created as a test plug-in to better understand the Maya API.
//

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MSimple.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MDagModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>
#include <vector>
#include <math.h>

DeclareSimpleCommand(limbLocalAngle, "Autodesk", "1.0")

MStatus limbLocalAngle::doIt(const MArgList &)
{
    MDagPath node;
    MObject component;
    MSelectionList locGroup;
    MFnDagNode nodeFn;
    std::vector<MVector> locations;
    std::vector<long double> distances;

    // Select the locators
    MGlobal::selectByName("locator*");
    MGlobal::getActiveSelectionList(locGroup);
    if(!locGroup.isEmpty()) {
        for (unsigned int i = 0; i < locGroup.length(); i++) {

            // Get the locations of the locators in the world space.
            locGroup.getDagPath(i, node, component);
            nodeFn.setObject(node);
            MFnTransform location(node);
            MVector translation = location.getTranslation(MSpace::kWorld);
            locations.push_back(translation);
            cout << nodeFn.name().asChar() << " is selected." << "\n";
        }
        for (unsigned int i = 0; i < locGroup.length(); i++) {
            if (!locations.empty()) {

                // Output locator position in centimeters.
                cout << "Locator " << i + 1 << ": " << "\n";
                cout << locations[i].x * 100 << " cm\n";
                cout << locations[i].y * 100 << " cm\n";
                cout << locations[i].z * 100 << " cm\n";

                if ((i + 1) % 3 == 0 && i > 0 && (i + 1) % 3 <= locGroup.length()) {

                    // If this is the third locator, calculate the distance between
                    // this locator and the first locator to get the third side
                    // of the triangle we'll use to calculate the local joint angle.
                    auto x = (locations[i].x - locations[i - 2].x) * 100;
                    auto y = (locations[i].y - locations[i - 2].y) * 100;
                    auto z = (locations[i].z - locations[i - 2].z) * 100;
                    auto distance = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
                    distances.push_back(distance);

                    cout << "Distance between Locator " << i - 1 << " and Locator " << i + 1 << ": " << distance
                         << " cm\n";

                    // Use the law of cosines to derive the angle we want.
                    auto theta = acos((pow(distances[i - 2], 2) + pow(distances[i - 1], 2) - pow(distances[i], 2)) /
                                      (2 * distances[i - 2] * distances[i - 1])) * 180 / 3.1415;

                    cout << "Computed momentary local angle " << theta << " degrees" << "\n";
                }
                if (i + 1 < locGroup.length() && (i + 1) % 3 != 0) {

                    // Get the other sides of the triangle.
                    auto x = (locations[i + 1].x - locations[i].x) * 100;
                    auto y = (locations[i + 1].y - locations[i].y) * 100;
                    auto z = (locations[i + 1].z - locations[i].z) * 100;
                    auto distance = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
                    distances.push_back(distance);

                    cout << "Computed delta-x: " << abs(x) << " cm" << " Computed delta-y: " << abs(y) << " cm"
                         << " Computed delta-z: " << abs(z) << " cm\n";
                    cout << "Distance between Locator " << i + 1 << " and Locator " << i + 2 << ": " << distance
                         << " cm\n";
                }
            }
        }
    }
    return MS::kSuccess;
}


