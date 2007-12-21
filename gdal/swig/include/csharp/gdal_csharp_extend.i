/******************************************************************************
 * $Id$
 *
 * Name:     gdal_csharp_extend.i
 * Project:  GDAL CSharp Interface
 * Purpose:  C# specific GDAL extensions
 * Author:   Tamas Szekeres, szekerest@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2007, Tamas Szekeres
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
 *****************************************************************************/
 
 
/******************************************************************************
 * GDAL raster R/W support                                                    *
 *****************************************************************************/

%extend GDALRasterBandShadow 
{
	%apply (void *buffer_ptr) {void *buffer};
	CPLErr ReadRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int pixelSpace, int lineSpace) {
       return GDALRasterIO( self, GF_Read, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, pixelSpace, lineSpace );
    }
    CPLErr WriteRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int pixelSpace, int lineSpace) {
       return GDALRasterIO( self, GF_Write, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, pixelSpace, lineSpace );
    }
    %clear void *buffer;
}

%extend GDALDatasetShadow 
{
	%apply (void *buffer_ptr) {void *buffer};
	CPLErr ReadRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int bandCount, int pixelSpace, int lineSpace, int bandSpace) {
       return GDALDatasetRasterIO( self, GF_Read, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, bandCount, 
		        NULL, pixelSpace, lineSpace, bandSpace);
    }
    CPLErr WriteRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int bandCount, int pixelSpace, int lineSpace, int bandSpace) {
       return GDALDatasetRasterIO( self, GF_Write, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, bandCount, 
		        NULL, pixelSpace, lineSpace, bandSpace);
    }
    %clear void *buffer;
}
