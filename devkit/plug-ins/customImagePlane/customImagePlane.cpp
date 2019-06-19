//-
// ==========================================================================
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

// Description: 
//   Demonstrates how to create your own custom image plane based on
//   Maya's internal image plane classes. This class works like
//   typical API nodes in that it can have a compute method and
//   can contain static attributes added by the API user.  This
//   example class overrides the default image plane behavior and
//   allows users to add transparency to some region of image plane
//   by manipulate the raw data of texture. Note, this code also 
//   illustrates how to use MImage to control the floating point
//   depth buffer.  When useDepthMap is set to true, depth is added
//   is added to the image such that half of the image is at the near 
//   clip plane and the remaining half is at the far clip plane. 
//
//   Note, once the image plane node has been created it you must
//   attached it to the camera shape that is displaying the node.
//   You need to use the imagePlane command to attach the image plane 
//   you created to the specified camera.
//
//	 To draw custom image plane in viewport 2.0, user should create an
//	 MPxImagePlaneOverride class and register this override when initialize
//	 the plug-in. There are 2 methods need to be realized by users.
//	 - updateDG();
//	 - updateColorTexture();
//
//   This example works only with renderers that use node evaluation
//   as a part of the rendering process, e.g. Maya Software. It does 
//   not work with 3rd party renderers that rely on a scene translation 
//   mechanism.
// 
//   For example, 
//     string $imageP = `createNode customImagePlane`;
//     imagePlane -edit -camera "persp" $imageP
//    

#include <maya/MPxImagePlane.h>
#include <maya/MFnPlugin.h>
#include <maya/MImage.h>
#include <maya/MString.h>
#include <maya/MFnNumericAttribute.h> 
#include <maya/MFnNumericData.h> 
#include <maya/MDataHandle.h> 
#include <maya/MPlug.h> 
#include <maya/MFnDependencyNode.h>
#include <maya/MRenderUtil.h>

// Vp2.0 headers
#include <maya/MViewport2Renderer.h>
#include <maya/MPxImagePlaneOverride.h>
#include <maya/MDrawRegistry.h>
#include <maya/MTextureManager.h>
#include <maya/MApiNamespace.h>

// Node Declaration
class customImagePlane : public MPxImagePlane
{
public:
						customImagePlane();
	MStatus				loadImageMap( const MString &fileName, int frame, MImage &image ) override;

	bool				getInternalValue( const MPlug&, MDataHandle&)		override;
    bool				setInternalValue( const MPlug&, const MDataHandle&) override;

	static  void*		creator();
	static  MStatus		initialize();

	static	MTypeId		id;				// The IFF type id
};

// Override Declaration
class customImagePlaneOverride : public MHWRender::MPxImagePlaneOverride
{
public:
	customImagePlaneOverride(const MObject& obj);
	virtual ~customImagePlaneOverride();

	static MHWRender::MPxImagePlaneOverride*	creator(const MObject& obj);
	virtual MHWRender::DrawAPI					supportedDrawAPIs() const;

	virtual void								updateDG();
	virtual void								updateColorTexture();

private:

	MObject					Object;
	MString					FileName;
	MHWRender::MTexture*	Texture;

};

//======================================== Node Implementation ========================================

customImagePlane::customImagePlane()
{
}

bool		
customImagePlane::getInternalValue( const MPlug & plug, MDataHandle & handle)
{		
	return MPxImagePlane::getInternalValue( plug, handle ); 
}

bool		
customImagePlane::setInternalValue( const MPlug & plug, const MDataHandle &	handle)
{
	return MPxImagePlane::setInternalValue( plug, handle );
}

MStatus	
customImagePlane::loadImageMap( 
	const MString &fileName, int frame, MImage &image )
{
	// Load and update depth map.
	image.readFromFile(fileName);
	
	unsigned int width, height;
	unsigned int i;
	image.getSize(width, height);

	MPlug depthMap( thisMObject(), useDepthMap ); 
	bool value; 
	depthMap.getValue( value ); 
	
	if ( value ) {
		float *buffer = new float[width*height];
		unsigned int c, j; 
		for ( c = i = 0; i < height; i ++ ) { 
			for ( j = 0; j < width; j ++, c++ ) { 
				if ( i > height/2 ) { 
					buffer[c] = -1.0f;
				} else { 
					buffer[c] = 0.0f;
				}
			}
		}
		image.setDepthMap( buffer, width, height ); 
		delete [] buffer;
	}

	return MStatus::kSuccess;
}

MTypeId customImagePlane::id( 0x1A19 );

void*
customImagePlane::creator()
{
	return new customImagePlane;
}

MStatus
customImagePlane::initialize()
{
	return MStatus::kSuccess;
}


//======================================== Override Implementation ========================================

MHWRender::MPxImagePlaneOverride* customImagePlaneOverride::creator(
	const MObject& obj)
{
	return new customImagePlaneOverride(obj);
}

customImagePlaneOverride::customImagePlaneOverride(const MObject& obj)
	: MHWRender::MPxImagePlaneOverride(obj)
	, Object(obj)
	, FileName("")
{
	Texture = nullptr;
}

customImagePlaneOverride::~customImagePlaneOverride()
{
	if (Texture)
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		MHWRender::MTextureManager* textureManager = renderer ? renderer->getTextureManager() : NULL;
		if (textureManager) 
		{
			textureManager->releaseTexture(Texture);
			Texture = nullptr;
		}
	}
	
}

MHWRender::DrawAPI customImagePlaneOverride::supportedDrawAPIs() const
{
	return MHWRender::kAllDevices;
}

void customImagePlaneOverride::updateDG()
{
	// Pull the file name from the DG for use in updateTexture
	MStatus status;
	MFnDependencyNode node(Object, &status);
	if (status)
	{
		bool useFrameExt = false;
		node.findPlug("useFrameExtension", true).getValue(useFrameExt);

		FileName = getFileName(useFrameExt);
	}
}

// Update color texture here.
// For depth map, it should update in MPxImagePlane::loadImageMap() method.
void customImagePlaneOverride::updateColorTexture()
{
	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (renderer)
	{
		MHWRender::MTextureManager* textureManager = renderer->getTextureManager();
		if (textureManager)
		{
			Texture = textureManager->acquireTexture(FileName, "");

			if (Texture)
			{
				MHWRender::MTextureDescription desc;
				Texture->textureDescription(desc);

				unsigned int bpp = Texture->bytesPerPixel();
				if (bpp == 4 && 
					(desc.fFormat == MHWRender::kR8G8B8A8_UNORM ||
					desc.fFormat == MHWRender::kB8G8R8A8))
				{
					int rowPitch = 0;
					size_t slicePitch = 0;
					bool generateMipMaps = true;
					unsigned char* pixelData = (unsigned char *)Texture->rawData(rowPitch, slicePitch);
					unsigned char* val = NULL;

					if (pixelData && rowPitch > 0 && slicePitch > 0)
					{
						// User can set if need update entire image file.
						bool updateEntireImage = false;
						if (updateEntireImage)
						{
							for (unsigned int i=0; i<desc.fHeight; i++)
							{					
								val = pixelData + (i*rowPitch);
								for (unsigned int j=0; j<desc.fWidth*4; j+=4)
								{
									// After this step, we change red channel to 255.
									// This will make the image plane image look red.
									*val = 255;
									val += 4;
								}
							}

							Texture->update(pixelData, generateMipMaps, rowPitch);

						} else {
							unsigned int minX = desc.fWidth / 3;
							unsigned int maxX = (desc.fWidth * 2) / 3;
							unsigned int newWidth = maxX - minX;
							unsigned int minY = desc.fHeight/ 3;
							unsigned int maxY = (desc.fHeight *2) / 3;
							unsigned int newHeight = maxY - minY;

							unsigned char* newData = new unsigned char[newWidth * newHeight * 4];
							if (newData)
							{
								unsigned char* newVal = newData;
								for (unsigned int i=minY; i<maxY; i++)
								{
									val = pixelData + (i*rowPitch) + minX*4;
									for (unsigned int j=0; j<newWidth*4; j++)
									{
										// After this step, we change each channel in this image region
										// to mid-gray and half semi-transparent.
										*newVal = 124;
										val++;
										newVal++;
									}
								}

								MHWRender::MTextureUpdateRegion updateRegion;
								updateRegion.fXRangeMin = minX;
								updateRegion.fXRangeMax = maxX;
								updateRegion.fYRangeMin = minY;
								updateRegion.fYRangeMax = maxY;
								Texture->update(newData, generateMipMaps, newWidth*4, &updateRegion);

								delete [] newData;
							}
						}					
					}

					delete [] pixelData;
				}
			}
		}
	}
}

//======================================== Plugin Setup ========================================

static const MString sRegistrantId("customImagePlaneOverride");

// These methods load and unload the plugin, registerNode registers the
// new node type with maya
//
MStatus initializePlugin(MObject obj)
{
	const MString UserClassify( "geometry/imagePlane:drawdb/geometry/imagePlane/customImagePlane" );
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "7.0", "Any");

	CHECK_MSTATUS(
		plugin.registerNode( "customImagePlane", customImagePlane::id, 
		customImagePlane::creator,
		customImagePlane::initialize, 
		MPxNode::kImagePlaneNode,
		&UserClassify));

	CHECK_MSTATUS(
		MHWRender::MDrawRegistry::registerImagePlaneOverrideCreator(
		"drawdb/geometry/imagePlane/customImagePlane",
		sRegistrantId,
		customImagePlaneOverride::creator));
	return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin( obj );
	CHECK_MSTATUS(
		plugin.deregisterNode( customImagePlane::id ));

	CHECK_MSTATUS(
		MHWRender::MDrawRegistry::deregisterImagePlaneOverrideCreator(
		"drawdb/geometry/imagePlane/customImagePlane",
		sRegistrantId));
	return MS::kSuccess;
}

