/* 
*  disp.cpp -- definition file for Display
*  USC csci 580 
*/


#include "gz.h"
#include "disp.h"
#include <stdlib.h>

int GzNewFrameBuffer(char** framebuffer, int width, int height)
{
  /* create a framebuffer:
   -- allocate memory for framebuffer : (sizeof)GzPixel x width x height
   -- pass back pointer
  */

  *framebuffer = ( char* ) malloc( 3 * width * height );
  if( *framebuffer == NULL ) return GZ_FAILURE;

  return GZ_SUCCESS;
}

int GzNewDisplay(GzDisplay **display, GzDisplayClass dispClass, int xRes, int yRes)
{
  /* create a display:
   -- allocate memory for indicated class and resolution
   -- pass back pointer to GzDisplay object in display
   */
   
  *display = ( GzDisplay * ) malloc( sizeof(GzDisplay) );
  (*display)->xres = xRes;
  (*display)->yres = yRes;
  (*display)->dispClass = dispClass;
  (*display)->open = 0;
  (*display)->fbuf = ( GzPixel * ) malloc( sizeof(GzPixel) * xRes * yRes );
  
  if( *display == NULL ) return GZ_FAILURE;
  return GZ_SUCCESS;
}


int GzFreeDisplay(GzDisplay *display)
{
  if( display == NULL ) return GZ_FAILURE;
  
  /* clean up, free memory */
  free( display->fbuf );
  free( display );
  
  return GZ_SUCCESS;
}


int GzGetDisplayParams(GzDisplay *display, int *xRes, int *yRes, GzDisplayClass	*dispClass)
{
  if( display == NULL ) return GZ_FAILURE;
    
  /* pass back values for an open display */
  *xRes = display->xres;
  *yRes = display->yres;
  *dispClass = display->dispClass;
  
  return GZ_SUCCESS;
}


int GzInitDisplay(GzDisplay *display)
{
  if( display == NULL ) return GZ_FAILURE;
    
  /* set everything to some default values - start a new frame */
  for( int i = 0; i < display->xres * display->yres; i++ ) {
    display->fbuf[i].red = 0 * 16;
    display->fbuf[i].green = 0 * 16;
    display->fbuf[i].blue = 0 * 16;
    display->fbuf[i].alpha = 0;
    display->fbuf[i].z = (float) INT_MAX;
  }
  
  return GZ_SUCCESS;
}


int GzPutDisplay(GzDisplay *display, int i, int j, GzIntensity r, GzIntensity g, GzIntensity b, GzIntensity a, GzDepth z)
{
  /* write pixel values into the display */
  if( i < 0 || i >= display->xres ) return GZ_FAILURE;
  if( j < 0 || j >= display->yres ) return GZ_FAILURE;
    
  display->fbuf[ i + j * display->xres ].red   = r;
  display->fbuf[ i + j * display->xres ].green = g;
  display->fbuf[ i + j * display->xres ].blue  = b;
  display->fbuf[ i + j * display->xres ].alpha = a;
  display->fbuf[ i + j * display->xres ].z     = z;  
  
  return GZ_SUCCESS;
}


int GzGetDisplay(GzDisplay *display, int i, int j, GzIntensity *r, GzIntensity *g, GzIntensity *b, GzIntensity *a, GzDepth *z)
{
  if( i < 0 || i >= display->xres ) return GZ_FAILURE;
  if( j < 0 || j >= display->yres ) return GZ_FAILURE;
  
  /* pass back pixel value in the display */
  /* check display class to see what vars are valid */
  
  *r = display->fbuf[ i + j * display->xres ].red;
  *g = display->fbuf[ i + j * display->xres ].green;
  *b = display->fbuf[ i + j * display->xres ].blue;
  *a = display->fbuf[ i + j * display->xres ].alpha;
  *z = display->fbuf[ i + j * display->xres ].z;
  
  return GZ_SUCCESS;
}


int GzFlushDisplay2File(FILE* outfile, GzDisplay *display)
{
  /* write pixels to ppm file based on display class -- "P6 %d %d 255\r" */
  fprintf( outfile, "P3\n" );
  fprintf( outfile, "%d %d\n", display->xres, display->yres );
  fprintf( outfile, "%d\n", 4095 );

  
  for( int i = 0; i < display->yres; i++ ) {
    for( int j = 0; j < display->xres; j++ ) {
      fprintf( outfile, "%d %d %d ", display->fbuf[ j + i * display->xres ].red   , 
                                     display->fbuf[ j + i * display->xres ].green , 
                                     display->fbuf[ j + i * display->xres ].blue  );
    }
    fprintf( outfile, "\n" );
  }

  return GZ_SUCCESS;
}

int GzFlushDisplay2FrameBuffer(char* framebuffer, GzDisplay *display)
{
  /* write pixels to framebuffer:
   - Put the pixels into the frame buffer
   - Caution: store the pixel to the frame buffer as the order of blue, green, and red
   - Not red, green, and blue !!!
  */
  for (int i = 0; i < display->yres; i++) {
    for (int j = 0; j < display->xres; j++) {
      int idx = (j + i * display->xres) * 3;
      framebuffer[idx + 0] = 255 * ( display->fbuf[j + i * display->xres].blue ) / 4095;
      framebuffer[idx + 1] = 255 * (display->fbuf[j + i * display->xres].green) / 4095;
      framebuffer[idx + 2] = 255 * (display->fbuf[j + i * display->xres].red) / 4095;
		}
	}
  return GZ_SUCCESS;
}

int GzFlushGbuf2File(FILE* outfile, GzGeoPixel *buf, int xres, int yres)
{
  fprintf( outfile, "P3\n" );
  fprintf( outfile, "%d %d\n", xres, yres );
  fprintf( outfile, "%d\n", 4095 );
  for( int i = 0; i < yres; i++ ) {
    for( int j = 0; j < xres; j++ ) {
      fprintf( outfile, "%d %d %d ", ( int ) ( buf[ j + i * xres ].flux[0] * 4095 ), 
                                     ( int ) ( buf[ j + i * xres ].flux[1] * 4095 ), 
                                     ( int ) ( buf[ j + i * xres ].flux[2] * 4095 ) );
    }
    fprintf( outfile, "\n" );
  }

  return GZ_SUCCESS;

}

int GzFlushGbuf2FrameBuffer(char* framebuffer, int width, int height, GzGeoPixel *buf, int xRes, int yRes, int xStart, int yStart )
{
  int base = xStart + yStart * width;
  for (int i = 0; i < yRes; i++) {
    for (int j = 0; j < xRes; j++) {
      int idx = ( base + j + i * width ) * 3;
      framebuffer[idx + 0] = 255 * buf[ j + i * xRes ].flux[2];
      framebuffer[idx + 1] = 255 * buf[ j + i * xRes ].flux[1];
      framebuffer[idx + 2] = 255 * buf[ j + i * xRes ].flux[0];
		}
	}
  return GZ_SUCCESS;
}
