/******************************************************************************
 * $Id: gdal_misc.cpp 13527 2008-01-14 09:07:44Z dron $
 *
 * Project:  GDAL Core
 * Purpose:  Functions for reading RPC and IMD formats, and normalizing.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) HER MAJESTY THE QUEEN IN RIGHT OF CANADA (2008)
 * as represented by the Canadian Nuclear Safety Commission
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
 ****************************************************************************/

#include "gdal.h"
#include "cpl_string.h"
#include "cplkeywordparser.h"

CPL_CVSID("$Id: gdal_misc.cpp 13527 2008-01-14 09:07:44Z dron $");

/************************************************************************/
/*                          GDALLoadRPBFile()                           */
/************************************************************************/

static const char *apszRPBMap[] = {
    "LINE_OFF",       "IMAGE.lineOffset",
    "SAMP_OFF",       "IMAGE.sampOffset",
    "HEIGHT_OFF",     "IMAGE.heightOffset",
    "LAT_OFF",        "IMAGE.latOffset",
    "LONG_OFF",       "IMAGE.longOffset",
    "LINE_SCALE",     "IMAGE.lineScale",
    "SAMP_SCALE",     "IMAGE.sampScale",
    "HEIGHT_SCALE",   "IMAGE.heightScale",
    "LAT_SCALE",      "IMAGE.latScale",
    "LONG_SCALE",     "IMAGE.longScale",
    "LINE_NUM_COEFF", "IMAGE.lineNumCoef",
    "LINE_DEN_COEFF", "IMAGE.lineDenCoef",
    "SAMP_NUM_COEFF", "IMAGE.sampNumCoef",
    "SAMP_DEN_COEFF", "IMAGE.sampDenCoef",
    NULL,             NULL };

char **CPL_STDCALL GDALLoadRPBFile( const char *pszFilename,
                                    char **papszSiblingFiles )

{
/* -------------------------------------------------------------------- */
/*      Try to identify the RPB file in upper or lower case.            */
/* -------------------------------------------------------------------- */
    CPLString osTarget = CPLResetExtension( pszFilename, "RPB" );
    
    if( papszSiblingFiles == NULL )
    {
        VSIStatBufL sStatBuf;
        
        if( VSIStatL( osTarget, &sStatBuf ) != 0 )
        {
            osTarget = CPLResetExtension( pszFilename, "rpb" );

            if( VSIStatL( osTarget, &sStatBuf ) != 0 )
                return NULL;
        }
    }
    else
    {
        int iSibling = CSLFindString( papszSiblingFiles, osTarget );
        if( iSibling < 0 )
            return NULL;

        osTarget = papszSiblingFiles[iSibling];
    }

/* -------------------------------------------------------------------- */
/*      Read file and parse.                                            */
/* -------------------------------------------------------------------- */
    CPLKeywordParser oParser;

    FILE *fp = VSIFOpenL( osTarget, "r" );

    if( fp == NULL )
        return NULL;
    
    if( !oParser.Ingest( fp ) )
    {
        VSIFCloseL( fp );
        return NULL;
    }

    VSIFCloseL( fp );

/* -------------------------------------------------------------------- */
/*      Extract RPC information, in a GDAL "standard" metadata format.  */
/* -------------------------------------------------------------------- */
    int i;
    char **papszMD = NULL;
    for( i = 0; apszRPBMap[i] != NULL; i += 2 )
    {
        const char *pszRPBVal = oParser.GetKeyword( apszRPBMap[i+1] );
        CPLString osAdjVal;

        if( pszRPBVal == NULL )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "%s file found, but missing %s field (and possibly others).",
                      osTarget.c_str(), apszRPBMap[i+1] );
            CSLDestroy( papszMD );
            return NULL;
        }

        if( strchr(pszRPBVal,',') == NULL )
            osAdjVal = pszRPBVal;
        else
        {
            // strip out commas and turn newlines into spaces.
            int j;

            for( j = 0; pszRPBVal[j] != '\0'; j++ )
            {
                switch( pszRPBVal[j] ) 
                {
                  case ',':
                  case '\n':
                  case '\r':
                    osAdjVal += ' ';
                    break;
                    
                  case '(':
                  case ')':
                    break;

                  default:
                    osAdjVal += pszRPBVal[j];
                }
            }
        }

        papszMD = CSLSetNameValue( papszMD, apszRPBMap[i], osAdjVal );
    }

    return papszMD;
}

/************************************************************************/
/*                          GDALWriteRPBFile()                          */
/************************************************************************/

CPLErr CPL_STDCALL GDALWriteRPBFile( const char *pszFilename, char **papszMD )

{
    CPLString osRPBFilename = CPLResetExtension( pszFilename, "RPB" );
    

/* -------------------------------------------------------------------- */
/*      Read file and parse.                                            */
/* -------------------------------------------------------------------- */
    FILE *fp = VSIFOpenL( osRPBFilename, "w" );

    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to create %s for writing.\n%s", 
                  osRPBFilename.c_str(), CPLGetLastErrorMsg() );
        return CE_Failure;
    }
    
/* -------------------------------------------------------------------- */
/*      Write the prefix information.                                   */
/* -------------------------------------------------------------------- */
    VSIFPrintfL( fp, "%s", "satId = \"QB02\";\n" );
    VSIFPrintfL( fp, "%s", "bandId = \"P\";\n" );
    VSIFPrintfL( fp, "%s", "SpecId = \"RPC00B\";\n" );
    VSIFPrintfL( fp, "%s", "BEGIN_GROUP = IMAGE\n" );

/* -------------------------------------------------------------------- */
/*      Write RPC values from our RPC metadata.                         */
/* -------------------------------------------------------------------- */
    int i;

    for( i = 0; apszRPBMap[i] != NULL; i += 2 )
    {
        const char *pszRPBVal = CSLFetchNameValue( papszMD, apszRPBMap[i] );
        const char *pszRPBTag;

        if( pszRPBVal == NULL )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "%s field missing in metadata, %s file not written.",
                      apszRPBMap[i], osRPBFilename.c_str() );
            VSIFCloseL( fp );
            VSIUnlink( osRPBFilename );
            return CE_Failure;
        }

        pszRPBTag = apszRPBMap[i+1];
        if( EQUALN(pszRPBTag,"IMAGE.",6) )
            pszRPBTag += 6;

        if( strstr(apszRPBMap[i], "COEF" ) == NULL )
        {
            VSIFPrintfL( fp, "\t%s = %s;\n", pszRPBTag, pszRPBVal );
        }
        else
        {
            // Reformat in brackets with commas over multiple lines.

            VSIFPrintfL( fp, "\t%s = (\n", pszRPBTag );

            char **papszItems = CSLTokenizeStringComplex( pszRPBVal, " ,", 
                                                          FALSE, FALSE );

            if( CSLCount(papszItems) != 20 )
            {
                CPLError( CE_Failure, CPLE_AppDefined,
                          "%s field is corrupt (not 20 values), %s file not written.\n%s = %s",
                          apszRPBMap[i], osRPBFilename.c_str(),
                          apszRPBMap[i], pszRPBVal );
                VSIFCloseL( fp );
                VSIUnlink( osRPBFilename );
                return CE_Failure;
            }

            int j;

            for( j = 0; j < 20; j++ )
            {
                if( j < 19 )
                    VSIFPrintfL( fp, "\t\t\t%s,\n", papszItems[j] );
                else
                    VSIFPrintfL( fp, "\t\t\t%s);\n", papszItems[j] );
            }
            CSLDestroy( papszItems );
        }
    }

/* -------------------------------------------------------------------- */
/*      Write end part                                                  */
/* -------------------------------------------------------------------- */
    VSIFPrintfL( fp, "%s", "END_GROUP = IMAGE\n" );
    VSIFPrintfL( fp, "END;\n" );
    VSIFCloseL( fp );
    
    return CE_None;
}

/************************************************************************/
/*                           GDAL_IMD_AA2R()                            */
/*                                                                      */
/*      Translate AA version IMD file to R version.                     */
/************************************************************************/

static int GDAL_IMD_AA2R( char ***ppapszIMD )

{
    char **papszIMD = *ppapszIMD;

/* -------------------------------------------------------------------- */
/*      Verify that we have a new format file.                          */
/* -------------------------------------------------------------------- */
    const char *pszValue = CSLFetchNameValue( papszIMD, "version" );
    
    if( pszValue == NULL )
        return FALSE;
    
    if( EQUAL(pszValue,"\"R\"") )
        return TRUE;

    if( !EQUAL(pszValue,"\"AA\"") )
    {
        CPLDebug( "IMD", "The file is not the expected 'version = \"AA\"' format.\nProceeding, but file may be corrupted." );
    }

/* -------------------------------------------------------------------- */
/*      Fix the version line.                                           */
/* -------------------------------------------------------------------- */
    papszIMD = CSLSetNameValue( papszIMD, "version", "\"R\"" );

/* -------------------------------------------------------------------- */
/*      remove a bunch of fields.                                       */
/* -------------------------------------------------------------------- */
    int iKey;

    static const char *apszToRemove[] = {
        "productCatalogId",
        "childCatalogId",
        "productType",
        "numberOfLooks",
        "effectiveBandwidth",
        "mode",
        "scanDirection",
        "cloudCover",
        "productGSD",
        NULL };

    for( iKey = 0; apszToRemove[iKey] != NULL; iKey++ )
    {
        int iTarget = CSLFindName( papszIMD, apszToRemove[iKey] );
        if( iTarget != -1 )
            papszIMD = CSLRemoveStrings( papszIMD, iTarget, 1, NULL );
    }

/* -------------------------------------------------------------------- */
/*      Replace various min/mean/max with just the mean.                */
/* -------------------------------------------------------------------- */
    static const char *keylist[] = { 
        "CollectedRowGSD",
        "CollectedColGSD",
        "SunAz",
        "SunEl",
        "SatAz",
        "SatEl",
        "InTrackViewAngle",
        "CrossTrackViewAngle",
        "OffNadirViewAngle",
        NULL };

    for( iKey = 0; keylist[iKey] != NULL; iKey++ )
    {
        CPLString osTarget;
        int       iTarget;

        osTarget.Printf( "IMAGE_1.min%s", keylist[iKey] );
        iTarget = CSLFindName( papszIMD, osTarget );
        if( iTarget != -1 )
            papszIMD = CSLRemoveStrings( papszIMD, iTarget, 1, NULL );

        osTarget.Printf( "IMAGE_1.max%s", keylist[iKey] );
        iTarget = CSLFindName( papszIMD, osTarget );
        if( iTarget != -1 )
            papszIMD = CSLRemoveStrings( papszIMD, iTarget, 1, NULL );

        osTarget.Printf( "IMAGE_1.mean%s", keylist[iKey] );
        iTarget = CSLFindName( papszIMD, osTarget );
        if( iTarget != -1 )
        {
            CPLString osValue = CSLFetchNameValue( papszIMD, osTarget );
            CPLString osLine;
            
            osTarget.Printf( "IMAGE_1.%c%s", 
                             tolower(keylist[iKey][0]), 
                             keylist[iKey]+1 );

            osLine = osTarget + "=" + osValue;

            CPLFree( papszIMD[iTarget] );
            papszIMD[iTarget] = CPLStrdup(osLine);
        }
    }

    *ppapszIMD = papszIMD;
    return TRUE;
}

/************************************************************************/
/*                          GDALLoadIMDFile()                           */
/************************************************************************/

char ** CPL_STDCALL GDALLoadIMDFile( const char *pszFilename,
                                     char **papszSiblingFiles )

{
/* -------------------------------------------------------------------- */
/*      Try to identify the RPB file in upper or lower case.            */
/* -------------------------------------------------------------------- */
    CPLString osTarget = CPLResetExtension( pszFilename, "IMD" );
    
    if( papszSiblingFiles == NULL )
    {
        VSIStatBufL sStatBuf;
        
        if( VSIStatL( osTarget, &sStatBuf ) != 0 )
        {
            osTarget = CPLResetExtension( pszFilename, "imd" );

            if( VSIStatL( osTarget, &sStatBuf ) != 0 )
                return NULL;
        }
    }
    else
    {
        int iSibling = CSLFindString( papszSiblingFiles, osTarget );
        if( iSibling < 0 )
            return NULL;

        osTarget = papszSiblingFiles[iSibling];
    }

/* -------------------------------------------------------------------- */
/*      Read file and parse.                                            */
/* -------------------------------------------------------------------- */
    CPLKeywordParser oParser;

    FILE *fp = VSIFOpenL( osTarget, "r" );

    if( fp == NULL )
        return NULL;
    
    if( !oParser.Ingest( fp ) )
    {
        VSIFCloseL( fp );
        return NULL;
    }

    VSIFCloseL( fp );

/* -------------------------------------------------------------------- */
/*      Consider version changing.                                      */
/* -------------------------------------------------------------------- */
    char **papszIMD = CSLDuplicate( oParser.GetAllKeywords() );
    const char *pszVersion = CSLFetchNameValue( papszIMD, "version" );

    if( pszVersion == NULL )
    {
        /* ? */;
    }
    else if( EQUAL(pszVersion,"\"AA\"") )
    {
        GDAL_IMD_AA2R( &papszIMD );
    }

    return papszIMD;
}

/************************************************************************/
/*                       GDALWriteIMDMultiLine()                        */
/*                                                                      */
/*      Write a value that is split over multiple lines.                */
/************************************************************************/
 
static void GDALWriteIMDMultiLine( FILE *fp, const char *pszValue )

{
    char **papszItems = CSLTokenizeStringComplex( pszValue, "(,) ", 
                                                  FALSE, FALSE );
    int nItemCount = CSLCount(papszItems);
    int i;

    VSIFPrintfL( fp, "(\n" );

    for( i = 0; i < nItemCount; i++ )
    {
        if( i == nItemCount-1 )
            VSIFPrintfL( fp, "\t%s );\n", papszItems[i] );
        else
            VSIFPrintfL( fp, "\t%s,\n", papszItems[i] );
    }
    CSLDestroy( papszItems );
}

/************************************************************************/
/*                          GDALWriteIMDFile()                          */
/************************************************************************/

CPLErr CPL_STDCALL GDALWriteIMDFile( const char *pszFilename, char **papszMD )

{
    CPLString osRPBFilename = CPLResetExtension( pszFilename, "IMD" );
    

/* -------------------------------------------------------------------- */
/*      Read file and parse.                                            */
/* -------------------------------------------------------------------- */
    FILE *fp = VSIFOpenL( osRPBFilename, "w" );

    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to create %s for writing.\n%s", 
                  osRPBFilename.c_str(), CPLGetLastErrorMsg() );
        return CE_Failure;
    }

/* ==================================================================== */
/* -------------------------------------------------------------------- */
/*      Loop through all values writing.                                */
/* -------------------------------------------------------------------- */
/* ==================================================================== */
    int iKey;
    CPLString osCurSection;

    for( iKey = 0; papszMD[iKey] != NULL; iKey++ )
    {
        char *pszRawKey = NULL;
        const char *pszValue = CPLParseNameValue( papszMD[iKey], &pszRawKey );
        CPLString osKeySection, osKeyItem;
        char *pszDot = strchr(pszRawKey,'.');

/* -------------------------------------------------------------------- */
/*      Split stuff like BAND_P.ULLon into section and item.            */
/* -------------------------------------------------------------------- */
        if( pszDot == NULL )
        {
            osKeyItem = pszRawKey;
        }
        else
        {
            osKeyItem = pszDot+1;
            *pszDot = '\0';
            osKeySection = pszRawKey;
        }
        CPLFree( pszRawKey );

/* -------------------------------------------------------------------- */
/*      Close and/or start sections as needed.                          */
/* -------------------------------------------------------------------- */
        if( osCurSection.size() && !EQUAL(osCurSection,osKeySection) )
            VSIFPrintfL( fp, "END_GROUP = %s\n", osCurSection.c_str() );

        if( osKeySection.size() && !EQUAL(osCurSection,osKeySection) )
            VSIFPrintfL( fp, "BEGIN_GROUP = %s\n", osKeySection.c_str() );

        osCurSection = osKeySection;

/* -------------------------------------------------------------------- */
/*      Print out simple item.                                          */
/* -------------------------------------------------------------------- */
        if( osCurSection.size() )
            VSIFPrintfL( fp, "\t%s = ", osKeyItem.c_str() );
        else
            VSIFPrintfL( fp, "%s = ", osKeyItem.c_str() );

        if( pszValue[0] != '(' )
            VSIFPrintfL( fp, "%s;\n", pszValue );
        else
            GDALWriteIMDMultiLine( fp, pszValue );
    }

/* -------------------------------------------------------------------- */
/*      Close off.                                                      */
/* -------------------------------------------------------------------- */
    if( osCurSection.size() )
        VSIFPrintfL( fp, "END_GROUP = %s\n", osCurSection.c_str() );
    
    VSIFPrintfL( fp, "END;\n" );
    
    VSIFCloseL( fp );
    
    return CE_None;
}
