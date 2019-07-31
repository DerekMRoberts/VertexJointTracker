# VertexJointTracker

**Welcome to the Repo!**

This is a Maya plug-in project attempting to take volumetric motion capture data captured from a state of the art 4DViews capture system (imported it into Maya as an Alembic file), and use vertices from the animated mesh surface data to derive sub-surface joint motion from it. 

## Getting Started

There is a plethora of documentation for Maya and it's API. Most of this information can be found [here](https://help.autodesk.com/view/MAYAUL/2019/ENU)

- ***Installing Maya:***
  
  For this project, we are using the *Maya 2019.1 Update* version with a student license.
  https://www.autodesk.com/education/free-software/maya
  
  You will need to create an account with your student e-mail in order to get a student license if you don't already have a license.
- ***Download the Devkit:***
  - [Windows 64-Bit](https://s3-us-west-2.amazonaws.com/autodesk-adn-transfer/ADN+Extranet/M%26E/Maya/devkit+2019/Autodesk_Maya_2019_1_Update_DEVKIT_Windows.zip)
  - [Linux](https://s3-us-west-2.amazonaws.com/autodesk-adn-transfer/ADN+Extranet/M%26E/Maya/devkit+2019/Autodesk_Maya_2019_1_Update_DEVKIT_Linux.tgz)
  - [MacOS](https://s3-us-west-2.amazonaws.com/autodesk-adn-transfer/ADN+Extranet/M%26E/Maya/devkit+2019/Autodesk_Maya_2019_1_Update_DEVKIT_Mac.dmg)
  
- ***Downloading & Importing Volumetric Sample Data:***
  - [4DViews Volumetric Resource Download Page](https://www.4dviews.com/volumetric-resources)
  
  I downloaded *Somersault* as an Alembic file (.ABC) and imported it into Maya by following [this instructional video](https://youtu.be/yP1DGPlfjC8) from 4DViews.
  
- ***Installing & Setting up the Build Environment***
  - [Installing on Windows 64-Bit](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Setting_up_your_build_Windows_environment_64_bit_html)
  - [Installing on Linux](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Setting_up_your_build_Linux_environment_html)
  - [Installing on MacOS](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Setting_up_your_build_Mac_OS_X_environment_html)
  
## Building, Loading, and Running the Plug-in

- ***Building***
  - [The CMakeLists.txt File](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Building_Your_Own_Plugin_The_CMakeList_File_html)
  - [Building with CMake](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Building_Your_Own_Plugin_Building_with_cmake_html)
  
  Sample plug-ins are located at:
  
  `$DEVKIT_LOCATION/devkit/plug-ins`
  
  The plug-ins created for this project are located at:
  
  `$DEVKIT_LOCATION/plug-ins/plug-ins`
  
- ***Loading in Maya***
  - [Loading](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Loading_Samples_Plug_ins_Into_Maya_html)
  
  In our particular case, we manually load the plug-in after it's built from the build directory located at: 
  
  `$DEVKIT_LOCATION/plug-ins/plug-ins/jointRigAnim/build`

- ***Running***
  - [helloWorld Example](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_A_First_Plugin_HelloWorld_html)
  
  As you can see in the example, once the plug-in is loaded, type in the appropriate registered command in the Maya command window. 

  In our case, for jointRigAnim use the registered command `jointRig`. 

  Make sure that the right interpreter is selected. It should say "MEL" next to it. The command window interpreter can also be switched to Python.
  
## Understanding Basic Maya Command Plug-in Structure
- [Command Plug-in Overview](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_Command_plug_ins_html)

This should help you understand functions such as `initializePlugin()`, `uninitializePlugin()`, `cmdSyntax()`, `parseArgs`, `creator()` as well as some other Maya plug-in essential knowledge.
  
## Debugging
The current state of debugging is... less than ideal. As you will see in the code I am basically outputting values to check in the console with `cout`. When opening Maya in Windows, a console opens along with it. However, as far as I can tell, the Mac version does not do this. I'm not sure about the linux version either. Either way, you can `cd` into the Maya executable location in the terminal and open it from there to see the console output. On Mac it should be located at:

`~/Applications/Autodesk/maya2019/Maya.app/Contents/MacOS`

Then run it

`./Maya`

Ideally, unit tests should be created, but I didn't get around to it as I spent a long time figuring out the Maya API.

## Current Branches

- jointRigAnimation is a branch that features two plug-ins:
    
    `jointCreate` - This plug-in simply creates a joint system of user-specified length.
    
    `jointRigAnim` - This plug-in takes 3 user-selected vertex pairs as sets (named vp1, vp2, and vp3) to indicate joint locations, 3 user-created locators (locator1, locator2, locator3), and tracks the movement of the joint locations throughout the animation by keyframing the updated location of the joint locators.
   
- limbLocalAngle is a branch that features one plug-in:
  
    `limbLocalAngle` - This plug-in takes locators place along a limb in order and calculates the local angle on the middle joint based on trigonometry.
    
- animMeshData is a branch that features one plug-in:

    `animMeshData` - This plug-in finds the number of polygons in the mesh each frame, finds the lowest poly count mesh, the highest poly count mesh, and the average poly count throughout the sequence.

## Built With
- [CMake](https://cmake.org/) - Build System
- [CLion](https://www.jetbrains.com/clion/) - IDE

## Authors
- **Derek Roberts** - *Initial Work* - [DerekMRoberts](http://github.com/DerekMRoberts/)
- [Contributors](https://github.com/DerekMRoberts/VertexJointTracker/graphs/contributors)
