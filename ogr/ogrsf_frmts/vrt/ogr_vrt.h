/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/VRT driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2  2003/11/07 21:55:12  warmerda
 * complete fid support, relative dsname, fixes
 *
 * Revision 1.1  2003/11/07 17:50:09  warmerda
 * New
 *
 */

#ifndef _OGR_VRT_H_INCLUDED
#define _OGR_VRT_H_INLLUDED

#include "ogrsf_frmts.h"
#include "cpl_error.h"
#include "cpl_minixml.h"

typedef enum { 
    VGS_None,
    VGS_Direct,
    VGS_PointFromColumns, 
    VGS_WKT, 
    VGS_WKB
} OGRVRTGeometryStyle;

/************************************************************************/
/*                            OGRVRTLayer                                */
/************************************************************************/

class OGRVRTLayer : public OGRLayer
{
  protected:
    OGRFeatureDefn      *poFeatureDefn;

    OGRDataSource       *poSrcDS;
    OGRLayer            *poSrcLayer;
    int                 bNeedReset;

    // Layer spatial reference system, and srid.
    OGRSpatialReference *poSRS;

    OGRGeometry         *poFilterGeom;

    char                *pszQuery;

    int                 iFIDField; // -1 means pass through. 

    // Geometry interpretation related.
    OGRVRTGeometryStyle eGeometryType;
    
    int                 iGeomField; 

                        // VGS_PointFromColumn
    int                 iGeomXField, iGeomYField, iGeomZField; 

    // Attribute Mapping
    int                *panSrcField;
    int                *pabDirectCopy;

    OGRFeature         *TranslateFeature( OGRFeature * );

  public:
                        OGRVRTLayer();
    virtual             ~OGRVRTLayer();

    virtual int         Initialize( CPLXMLNode *psLTree, 
                                    const char *pszVRTDirectory );

    virtual void        ResetReading();
    virtual OGRFeature *GetNextFeature();

    virtual OGRFeature *GetFeature( long nFeatureId );
    
    virtual OGRFeatureDefn *GetLayerDefn() { return poFeatureDefn; }

    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         GetFeatureCount( int );

    virtual OGRGeometry *GetSpatialFilter() { return poFilterGeom; }
    virtual void        SetSpatialFilter( OGRGeometry * );

//    virtual OGRErr      SetAttributeFilter( const char * );

    virtual int         TestCapability( const char * );
};

/************************************************************************/
/*                           OGRVRTDataSource                            */
/************************************************************************/

class OGRVRTDataSource : public OGRDataSource
{
    OGRVRTLayer        **papoLayers;
    int                 nLayers;
    
    char               *pszName;

  public:
                        OGRVRTDataSource();
                        ~OGRVRTDataSource();

    int                 Initialize( CPLXMLNode *psXML, const char *pszName );

    const char          *GetName() { return pszName; }
    int                 GetLayerCount() { return nLayers; }
    OGRLayer            *GetLayer( int );

    int                 TestCapability( const char * );
};

/************************************************************************/
/*                             OGRVRTDriver                             */
/************************************************************************/

class OGRVRTDriver : public OGRSFDriver
{
  public:
                ~OGRVRTDriver();
                
    const char *GetName();
    OGRDataSource *Open( const char *, int );
    int         TestCapability( const char * );
};


#endif /* ndef _OGR_VRT_H_INCLUDED */


