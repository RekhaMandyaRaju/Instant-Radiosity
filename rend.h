#include "disp.h" /* include your own disp.h file (e.g. hw1)*/

/* Camera defaults */
#define	DEFAULT_FOV		35.0
#define	DEFAULT_IM_Z	(-10.0)  /* world coords for image plane origin */
#define	DEFAULT_IM_Y	(5.0)    /* default look-at point = 0,0,0 */
#define	DEFAULT_IM_X	(-10.0)

#define	DEFAULT_AMBIENT	  ( GzColor(0.1, 0.1, 0.1) )
#define	DEFAULT_DIFFUSE	  ( GzColor(0.7, 0.6, 0.5) )
#define	DEFAULT_SPECULAR	( GzColor(0.2, 0.3, 0.4) )
#define	DEFAULT_SPEC		32

#define	MATLEVELS	100		/* how many matrix pushes allowed */
#define	MAX_LIGHTS	4096		/* how many lights allowed */

/* Dummy definition : change it later */
//#ifndef GzLight
//#define GzLight		GzPointer
//#endif

//#ifndef GzTexture
//#define GzTexture	GzPointer
//#endif

#include <vector>

#include  <map>
#include	<string>

#ifndef GZRENDER
#define GZRENDER

typedef struct GzTextureFile
{
  std::string name;
  GzColor	*image;
  int xs, ys;
} GzTextureFile;

typedef struct GzRender {			/* define a renderer */

  unsigned short	xres;
  unsigned short	yres;

  short		    open;

  int			    numlights;
  GzLight		  lights[MAX_LIGHTS];
  GzCamera		camera;

  short		    matlevel;	          /* top of stack - current xform */
  GzMatrix		Ximage[MATLEVELS];	/* stack of xforms (Xwm) */
  GzMatrix		Xnorm[MATLEVELS];	  /* xforms for norms (Xnwm) */

  /*
    Xwm is generated from the stack
  
  */

  GzMatrix		Xwm;		          /* model -> world, (M) */
  GzMatrix		Xcw;		          /* world -> camera, (V) */
  GzMatrix		Xpc;		          /* camera -> perspective, (P) */
  GzMatrix		Xsp;		          /* perspective -> screen */

  GzMatrix		Xncw;		          /* normal, world -> camera, (V) */
  GzMatrix		Xnpc;		          /* normal, camera -> perspective, (P) */


  GzMatrix		Xps;		          /* screen -> perspective */


  GzColor		  flatcolor;          /* color state for flat shaded triangles */
  int			    interp_mode;


  GzLight		  ambientlight;
  GzColor		  Ka, Kd, Ks;
  float		    spec;		/* specular power */
  GzTexture		tex_fun;    /* tex_fun(float u, float v, GzColor color) */

  GzGeoPixel	*gbuf;		/* frame buffer array */
  
  GzMatrix    Xws;     /* screen space to world space */
  GzMatrix    Xnws;    /* screen space to world space */

  GzShader		  shader_func;
  GzShaderData *sharder_data;

  
  GzCoord     sceneSize[2];

  float                     samplesThreshold;
  std::map< GzCoord, bool > * sampleMapping;

  std::vector<GzISMSample>  * samples;
  std::vector<int>          * microView;    
  int                       numMicroView;
  bool                      enable_sample;

  
  std::map< std::string, GzTextureFile > * textureFileMapping;
  GzTextureData *texture_data;


}  GzRender;
#endif

// Function declaration
int GzNewRender( GzRender **render, int width, int height );
int GzFreeRender(GzRender *render);
int GzPutAttribute(GzRender	*render, int numAttributes, GzToken	*nameList, GzPointer *valueList);
int GzPutTriangle(GzRender *render, int	numParts, GzToken *nameList, GzPointer *valueList);

int GzPutSample( GzRender	*render, float zNear, float zFar );

int GzSetScreenSize( GzRender *render, int width, int height );

void GzSetSample( GzRender *render, bool enable );

GzMatrix GzRotXMat(float degree);
GzMatrix GzRotYMat(float degree);
GzMatrix GzRotZMat(float degree);
GzMatrix GzTrxMat(GzCoord translate);
GzMatrix GzScaleMat(GzCoord scale);


int GzEnableLight(GzRender	*render );
int GzDisableLight(GzRender	*render );
int GzEnableLight(GzRender	*render, int index );
int GzDisableLight(GzRender	*render, int index );
int GzGetLight(GzRender	*render, int index, GzLight * light );


int GzSaveGInfo(GzRender *render, GzGeoInfo *gbuf );
int GzBindMatrices(GzRender *render, GzGeoInfo ginfo );


int GzSetViewMatrix( GzRender	*render, GzMatrix mat );
int GzSetProjectMatrix( GzRender	*render, GzMatrix mat );


GzMatrix GzPersProjection( float fovyInDegrees, float aspectRatio, float znear, float zfar );
GzMatrix GzFrustum( float left, float right, float bottom, float top, float znear, float zfar );
GzMatrix GzOrthProjection( GzCoord min, GzCoord max );
GzMatrix GzMatrixLookAt( GzCoord eye, GzCoord center, GzCoord up );

int GzNewGBuffer( GzGeoInfo	**buf, unsigned short	width, unsigned short	height );
int GzNewGBuffer( GzRender	*render );
int GzNewGBuffer( GzRender	*render, GzGeoPixel	**buf );
int GzBindGBuffer( GzRender	*render, GzGeoPixel	*buf );
int GzInitGBuffer( GzRender	*render, GzGeoPixel	*buf );
int GzDrawRectangle( GzGeoPixel	*buf, int xres, int yres, int x, int y, int width, int height, GzColor color );
int GzDrawLine( GzGeoPixel	*buf, int xres, int yres, int sx, int sy, int ex, int ey, GzColor color );


int GzDeferredShading( GzRender	*render, bool showProgress = true );


int GzPositionBounday( GzRender	*render, GzMatrix mat, GzCoord *min, GzCoord *max );

// HW3
int GzPutCamera(GzRender *render, GzCamera *camera);
int GzPushMatrix(GzRender *render, GzMatrix	matrix);
int GzPopMatrix(GzRender *render);
int GzClearMatrix(GzRender *render);




void GzPrintMatrix( FILE *f, GzMatrix matrix );
void GzPrintCoord( FILE *f, GzCoord coord );



int GzRSMSamplingPat( GzRSMSample **pattern, int num, float radius );

