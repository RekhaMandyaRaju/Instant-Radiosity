/*
 * Gz.h - include file for the cs580 rendering library
 */
 
#include <math.h>
#include <stdint.h>
#include <limits>
#include <stdlib.h>

#define INT_MAX (std::numeric_limits<int32_t>::max())

/*
 * universal constants
 */
#define GZ_SUCCESS      0
#define GZ_FAILURE      1

/*
 * display classes
 */

#define GZ_RGBAZ_DISPLAY        1

/*
 * rendering classes
 */
#define GZ_Z_BUFFER_RENDER      1

/*
 * name list tokens
 */
#define GZ_NULL_TOKEN           0	/* triangle vert attributes */
#define GZ_POSITION             1
#define GZ_NORMAL               2
#define GZ_TEXTURE_INDEX        3

/* renderer-state default pixel color */
#define GZ_RGB_COLOR            99	

#define GZ_INTERPOLATE			95	/* define interpolation mode */
#define GZ_SHADER				96	/* define the shade mode */

#define GZ_VPLIGHT              83  /* VPL */
#define GZ_POINT_LIGHT		      82	/* point light */
#define GZ_SPOT_LIGHT		        81	/* spot light */
#define GZ_DIRECTIONAL_LIGHT		80	/* directional light */
#define GZ_LIGHT		            79	/* spot light */
#define GZ_AMBIENT_LIGHT		78	/* ambient light type */

#define GZ_AMBIENT_COEFFICIENT		1001	/* Ka material property */
#define GZ_DIFFUSE_COEFFICIENT		1002	/* Kd material property */
#define GZ_SPECULAR_COEFFICIENT		1003	/* Ks material property */
#define GZ_DISTRIBUTION_COEFFICIENT	1004	/* specular power of material */

#define	GZ_TEXTURE_MAP			      1010	/* pointer to texture routine */
#define	GZ_VERTEX_SHADER_FUNC			1011	/* pointer to texture routine */
#define	GZ_SHADER_FUNC			      1100	/* pointer to texture routine */
#define	GZ_DATA			              1101	/* pointer to outside data */

#define GZ_SAMPLE_COEFFICIENT	    1110	/* the area threshold for sampling */


/*
 * value-list attributes
 */

/* shade mode flags combine the bit fields below */
#define	GZ_NONE			0	/* flat shading only */
#define	GZ_AMBIENT			1	/* can be selected or not */
#define	GZ_DIFFUSE			2	/* can be selected or not */
#define GZ_SPECULAR			4	/* can be selected or not */

/* select interpolation mode of the shader */
#define GZ_FLAT			0	/* do flat shading with GZ_RBG_COLOR */
#define	GZ_COLOR			1	/* interpolate vertex color */
#define	GZ_NORMALS			2	/* interpolate normals */

typedef int     GzRenderClass;
typedef int     GzDisplayClass;
typedef int     GzToken;
typedef void    *GzPointer;

typedef short   GzIntensity;	/* 0-4095 in lower 12-bits for RGBA */
//typedef float   GzTextureIndex[2];
typedef float	  GzDepth;		/* signed z for clipping */
typedef float   GzRSMSample[3];

#ifndef GZ_COORDS

typedef struct GzTextureIndex {
  float v[2];
  GzTextureIndex()                           { v[0] = 0; v[1] = 0; }
  GzTextureIndex( float tu, float tv  )      { v[0] = tu; v[1] = tv; }
  GzTextureIndex( const GzTextureIndex &t )  { v[0] = t.v[0]; v[1] = t.v[1]; }
  
  GzTextureIndex operator +  ( const GzTextureIndex &t )  const { return GzTextureIndex( v[0] + t.v[0], v[1] + t.v[1] ); } 
  GzTextureIndex operator *  ( const float s           )  const { return GzTextureIndex( v[0] * s, v[1] * s ); } 
  GzTextureIndex operator /  ( const float s           )  const { return GzTextureIndex( v[0] / s, v[1] / s ); } 
  void           operator =  ( const GzTextureIndex &t )        {  v[0] = t.v[0]; v[1] = t.v[1]; } 
  float &        operator [] ( const int i             )        { return v[i]; } 
 
} GzTextureIndex;


typedef struct GzColor {
  float v[3];

  GzColor()                                       { v[0] = 0; v[1] = 0; v[2] = 0; }
  GzColor( double r, double g, double b  )        { v[0] = r; v[1] = g; v[2] = b;  }
  GzColor( const GzColor &c )                     { v[0] = c.v[0]; v[1] = c.v[1]; v[2] = c.v[2]; }

  GzColor operator +  ( const GzColor &c )  const { return GzColor( v[0] + c.v[0], v[1] + c.v[1], v[2] + c.v[2] ); } 
  GzColor operator +  ( const double s   )  const { return GzColor( v[0] + s, v[1] + s, v[2] + s ); } 
  GzColor operator -  ( const GzColor &c )  const { return GzColor( v[0] - c.v[0], v[1] - c.v[1], v[2] - c.v[2] ); } 
  GzColor operator -  (                  )  const { return GzColor( -1 * v[0], -1 * v[1], -1 * v[2] ); } 
  GzColor operator *  ( const double s   )  const { return GzColor( v[0] * s, v[1] * s, v[2] * s ); } 
  GzColor operator *  ( const GzColor &c )  const { return GzColor( v[0] * c.v[0], v[1] * c.v[1], v[2] * c.v[2] ); } 
  GzColor operator /  ( const double s   )  const { return GzColor( v[0] / s, v[1] / s, v[2] / s ); } 
  void    operator =  ( const GzColor &c )        {  v[0] = c.v[0]; v[1] = c.v[1]; v[2] = c.v[2]; } 
  bool    operator == ( const GzColor &c )        { return v[0] == c.v[0] && v[1] == c.v[1] && v[2] == c.v[2]; } 
  float & operator [] ( const int i      )        { return v[i]; } 

  GzColor cutoff      ( void             )        { return GzColor( v[0] = ( v[0] > 1 ) ? 1 : ( v[0] < 0 ) ? 0 : v[0], v[1] = ( v[1] > 1 ) ? 1 : ( v[1] < 0 ) ? 0 : v[1], v[2] = ( v[2] > 1 ) ? 1 : ( v[2] < 0 ) ? 0 : v[2] ); }        

} GzColor;

typedef struct GzCoord {
  float v[4];

  GzCoord()                                                 { v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 0; }
  GzCoord( double x, double y, double z, double w = 1.0  )  { v[0] = x; v[1] = y; v[2] = z; v[3] = w;  }
  GzCoord( const GzCoord &c )                               { v[0] = c.v[0]; v[1] = c.v[1]; v[2] = c.v[2]; v[3] = c.v[3]; }

  GzCoord operator +  ( const GzCoord &c )  const { return GzCoord( v[0] + c.v[0], v[1] + c.v[1], v[2] + c.v[2], v[3] + c.v[3] ); } 
  GzCoord operator -  ( const GzCoord &c )  const { return GzCoord( v[0] - c.v[0], v[1] - c.v[1], v[2] - c.v[2], v[3] - c.v[3] ); } 
  GzCoord operator -  (                  )  const { return GzCoord( -1 * v[0], -1 * v[1], -1 * v[2], -1 * v[3] ); } 
  GzCoord operator *  ( const double s   )  const { return GzCoord( v[0] * s, v[1] * s, v[2] * s, v[3] * s ); } 
  GzCoord operator /  ( const double s   )  const { return GzCoord( v[0] / s, v[1] / s, v[2] / s, v[3] / s ); } 
  void    operator =  ( const GzCoord &c )        {  v[0] = c.v[0]; v[1] = c.v[1]; v[2] = c.v[2]; v[3] = c.v[3]; } 
  bool    operator == ( const GzCoord &c )        { return v[0] == c.v[0] && v[1] == c.v[1] && v[2] == c.v[2]; } 
  bool    operator != ( const GzCoord &c )        { return !(v[0] == c.v[0] && v[1] == c.v[1] && v[2] == c.v[2]); } 
  float & operator [] ( const int i      )        { return v[i]; } 

  float   length      ( void             )        { return sqrtf( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] ); }
  GzCoord abs         ( void             )        { return GzCoord( fabs(v[0]), fabs(v[1]), fabs(v[2]), fabs(v[3]) ); }
  GzCoord dW          ( void             )        { return GzCoord( v[0] / v[3], v[1] / v[3], v[2] / v[3], 1 ); }
  GzCoord normal      ( void             )        { float invLength = 1 / sqrtf( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
                                                    return GzCoord( v[0] * invLength, v[1] * invLength, v[2] * invLength, 1 ); }
  /* vector operations */
  GzCoord operator *  ( const GzCoord &c )  const { return GzCoord( v[1] * c.v[2] - v[2] * c.v[1], v[2] * c.v[0] - v[0] * c.v[2], v[0] * c.v[1] - v[1] * c.v[0] ); } 
  float   operator || ( const GzCoord &c )        { return v[0] * c.v[0] + v[1] * c.v[1] + v[2] * c.v[2];  } 

} GzCoord;



typedef struct GzMatrix {

  GzCoord v[4];

  GzMatrix()                                                 { v[0] = GzCoord(); v[1] = GzCoord(); v[2] = GzCoord(); v[3] = GzCoord();  }
  GzMatrix( GzCoord m0, GzCoord m1, GzCoord m2, GzCoord m3 ) { v[0] = m0; v[1] = m1; v[2] = m2; v[3] = m3; }
  GzMatrix( const GzMatrix &m  )                             { v[0] = m.v[0]; v[1] = m.v[1]; v[2] = m.v[2]; v[3] = m.v[3];  }
  GzMatrix( float m00, float m01, float m02, float m03,    
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33     ) { v[0] = GzCoord( m00, m01, m02, m03 ); v[1] = GzCoord( m10, m11, m12, m13 ); v[2] = GzCoord( m20, m21, m22, m23 ); v[3] = GzCoord( m30, m31, m32, m33 ); }
  GzMatrix( const float *m                                 ) { v[0] = GzCoord( m[0], m[1], m[2], m[3] ); v[1] = GzCoord( m[4], m[5], m[6], m[7] ); v[2] = GzCoord( m[8], m[9], m[10], m[11] ); v[3] = GzCoord( m[12], m[13], m[14], m[15] ); }

  void      operator =  ( const GzMatrix &m )       { v[0] = m.v[0]; v[1] = m.v[1]; v[2] = m.v[2]; v[3] = m.v[3]; } 
  void      operator =  ( const float *m    )       { v[0] = GzCoord( m[0], m[1], m[2], m[3] ); v[1] = GzCoord( m[4], m[5], m[6], m[7] ); v[2] = GzCoord( m[8], m[9], m[10], m[11] ); v[3] = GzCoord( m[12], m[13], m[14], m[15] ); }
  

  GzCoord & operator [] ( const int i )             { return v[i]; } 

  /* matrix operations */
  GzCoord   operator *  ( const GzCoord &c  )       { GzCoord input = GzCoord( c.v[0], c.v[1], c.v[2], c.v[3] );
                                                      GzCoord output = GzCoord();
                                                      for( int i = 0; i < 4; i++ )
		                                                    for( int j = 0; j < 4; j++ )
                                                          output[ i ] += v[ i ][ j ] * input[ j ];
                                                      return output;
                                                    }
  
  GzMatrix  operator *  ( GzMatrix &m       )       { GzMatrix output = GzMatrix();
                                                      for( int i = 0; i < 4; i++ )
                                                        for( int j = 0; j < 4; j++ )
                                                          for( int k = 0; k < 4; k++ ) output.v[i][j] += v[i][k] * m.v[k][j]; 
                                                      return output;
                                                    }

  GzMatrix  normal      ( void              )       { float K = 1 / sqrtf( v[0][0] * v[0][0] + v[0][1] * v[0][1] + v[0][2] * v[0][2] );
                                                      return GzMatrix( GzCoord( v[0][0] * K, v[0][1] * K, v[0][2] * K, 0 ),
                                                                       GzCoord( v[1][0] * K, v[1][1] * K, v[1][2] * K, 0 ),
                                                                       GzCoord( v[2][0] * K, v[2][1] * K, v[2][2] * K, 0 ),
                                                                       GzCoord(          0 ,           0,           0, 1 ) );
                                                    }

  void      identity    ( void              )       { v[0] = GzCoord( 1, 0, 0, 0 ); v[1] = GzCoord( 0, 1, 0, 0 ); v[2] = GzCoord( 0, 0, 1, 0 ); v[3] = GzCoord( 0, 0, 0, 1 ); }         
  void      zero        ( void              )       { v[0] = GzCoord( 0, 0, 0, 0 ); v[1] = GzCoord( 0, 0, 0, 0 ); v[2] = GzCoord( 0, 0, 0, 0 ); v[3] = GzCoord( 0, 0, 0, 0 ); }         

  void      inverse     ( void              )       { GzMatrix inv;
                                                      float det;
                                                      inv[0][0] =  v[1][1] * v[2][2] * v[3][3] - v[1][1] * v[2][3] * v[3][2] - v[2][1] * v[1][2] * v[3][3] + v[2][1] * v[1][3] * v[3][2] + v[3][1] * v[1][2] * v[2][3] - v[3][1] * v[1][3] * v[2][2];
                                                      inv[1][0] = -v[1][0] * v[2][2] * v[3][3] + v[1][0] * v[2][3] * v[3][2] + v[2][0] * v[1][2] * v[3][3] - v[2][0] * v[1][3] * v[3][2] - v[3][0] * v[1][2] * v[2][3] + v[3][0] * v[1][3] * v[2][2];
                                                      inv[2][0] =  v[1][0] * v[2][1] * v[3][3] - v[1][0] * v[2][3] * v[3][1] - v[2][0] * v[1][1] * v[3][3] + v[2][0] * v[1][3] * v[3][1] + v[3][0] * v[1][1] * v[2][3] - v[3][0] * v[1][3] * v[2][1];
                                                      inv[3][0] = -v[1][0] * v[2][1] * v[3][2] + v[1][0] * v[2][2] * v[3][1] + v[2][0] * v[1][1] * v[3][2] - v[2][0] * v[1][2] * v[3][1] - v[3][0] * v[1][1] * v[2][2] + v[3][0] * v[1][2] * v[2][1];
                                                      inv[0][1] = -v[0][1] * v[2][2] * v[3][3] + v[0][1] * v[2][3] * v[3][2] + v[2][1] * v[0][2] * v[3][3] - v[2][1] * v[0][3] * v[3][2] - v[3][1] * v[0][2] * v[2][3] + v[3][1] * v[0][3] * v[2][2];
                                                      inv[1][1] =  v[0][0] * v[2][2] * v[3][3] - v[0][0] * v[2][3] * v[3][2] - v[2][0] * v[0][2] * v[3][3] + v[2][0] * v[0][3] * v[3][2] + v[3][0] * v[0][2] * v[2][3] - v[3][0] * v[0][3] * v[2][2];
                                                      inv[2][1] = -v[0][0] * v[2][1] * v[3][3] + v[0][0] * v[2][3] * v[3][1] + v[2][0] * v[0][1] * v[3][3] - v[2][0] * v[0][3] * v[3][1] - v[3][0] * v[0][1] * v[2][3] + v[3][0] * v[0][3] * v[2][1];
                                                      inv[3][1] =  v[0][0] * v[2][1] * v[3][2] - v[0][0] * v[2][2] * v[3][1] - v[2][0] * v[0][1] * v[3][2] + v[2][0] * v[0][2] * v[3][1] + v[3][0] * v[0][1] * v[2][2] - v[3][0] * v[0][2] * v[2][1];
                                                      inv[0][2] =  v[0][1] * v[1][2] * v[3][3] - v[0][1] * v[1][3] * v[3][2] - v[1][1] * v[0][2] * v[3][3] + v[1][1] * v[0][3] * v[3][2] + v[3][1] * v[0][2] * v[1][3] - v[3][1] * v[0][3] * v[1][2];
                                                      inv[1][2] = -v[0][0] * v[1][2] * v[3][3] + v[0][0] * v[1][3] * v[3][2] + v[1][0] * v[0][2] * v[3][3] - v[1][0] * v[0][3] * v[3][2] - v[3][0] * v[0][2] * v[1][3] + v[3][0] * v[0][3] * v[1][2];
                                                      inv[2][2] =  v[0][0] * v[1][1] * v[3][3] - v[0][0] * v[1][3] * v[3][1] - v[1][0] * v[0][1] * v[3][3] + v[1][0] * v[0][3] * v[3][1] + v[3][0] * v[0][1] * v[1][3] - v[3][0] * v[0][3] * v[1][1];
                                                      inv[3][2] = -v[0][0] * v[1][1] * v[3][2] + v[0][0] * v[1][2] * v[3][1] + v[1][0] * v[0][1] * v[3][2] - v[1][0] * v[0][2] * v[3][1] - v[3][0] * v[0][1] * v[1][2] + v[3][0] * v[0][2] * v[1][1];
                                                      inv[0][3] = -v[0][1] * v[1][2] * v[2][3] + v[0][1] * v[1][3] * v[2][2] + v[1][1] * v[0][2] * v[2][3] - v[1][1] * v[0][3] * v[2][2] - v[2][1] * v[0][2] * v[1][3] + v[2][1] * v[0][3] * v[1][2];
                                                      inv[1][3] =  v[0][0] * v[1][2] * v[2][3] - v[0][0] * v[1][3] * v[2][2] - v[1][0] * v[0][2] * v[2][3] + v[1][0] * v[0][3] * v[2][2] + v[2][0] * v[0][2] * v[1][3] - v[2][0] * v[0][3] * v[1][2];
                                                      inv[2][3] = -v[0][0] * v[1][1] * v[2][3] + v[0][0] * v[1][3] * v[2][1] + v[1][0] * v[0][1] * v[2][3] - v[1][0] * v[0][3] * v[2][1] - v[2][0] * v[0][1] * v[1][3] + v[2][0] * v[0][3] * v[1][1];
                                                      inv[3][3] =  v[0][0] * v[1][1] * v[2][2] - v[0][0] * v[1][2] * v[2][1] - v[1][0] * v[0][1] * v[2][2] + v[1][0] * v[0][2] * v[2][1] + v[2][0] * v[0][1] * v[1][2] - v[2][0] * v[0][2] * v[1][1];
                                                      det = v[0][0] * inv[0][0] + v[0][1] * inv[1][0] + v[0][2] * inv[2][0] + v[0][3] * inv[3][0];
 
                                                      if( det == 0 ) return;
 
                                                      det = 1.f / det;
                                                      for( int i = 0; i < 4; i++ ) 
                                                        for( int j = 0; j < 4; j++ ) 
                                                          v[i][j] = inv[i][j] * det;
                                                    }


} GzMatrix;


#define GZ_COORDS
#endif




/* shading function */
typedef	int	    (*GzShader)( GzPointer	data, int x, int y );
typedef void    *GzShaderData;

/* texture function */

typedef	int	(*GzTexture)(GzPointer	data, float u, float v, GzColor *color);	/* pointer to texture sampling method */
typedef void *GzTextureData;

/* u,v parameters [0,1] are defined tex_fun(float u, float v, GzColor color) */



/* g-buffer */
#ifndef GZ_GEOPIXEL
typedef	struct GzGeoPixel {
  GzCoord pos;          /* world space */
  GzCoord normal;       /* world space */
  GzColor flux;
  GzColor Ka;
  GzColor Kd;
  GzColor Ks;
  GzDepth depth;

  void      operator =  ( const GzGeoPixel &g )       { pos = g.pos; normal = g.normal; flux = g.flux; Ka = g.Ka; Kd = g.Kd; Ks = g.Ks; depth = g.depth; } 
} GzGeoPixel;

typedef	struct GzGeoInfo {
  GzMatrix        Xcw;  		      /* xform from world to image space */
  GzMatrix        Xpc;            /* perspective projection xform */
  GzMatrix        Xsp;

  unsigned short	xres;
  unsigned short	yres;
  GzGeoPixel	   *gbuf;

//  GzGeoBuffer           ( unsigned short	width, unsigned short	height )  { xres = }
  void      operator =  ( const GzGeoInfo &g )       { Xcw = g.Xcw; Xpc = g.Xpc; Xsp = g.Xsp; gbuf = g.gbuf; xres = g.xres; yres = g.yres; } 
} GzGeoInfo;

#define GZ_GEOPIXEL
#endif

#ifndef GZ_ISM
typedef	struct GzISMSample {
  GzGeoPixel sample;
  GzGeoInfo  ISM;
  void      operator =  ( const GzISMSample &i )       { sample = i.sample; ISM = i.ISM; } 
} GzISMSample;
#define GZ_ISM
#endif

/*
 * Gz camera definition
 */
#ifndef GZCAMERA
#define GZCAMERA
typedef struct  GzCamera
{

  GzCoord         position;       /* position of image plane origin */
  GzCoord         lookat;         /* position of look-at-point */
  GzCoord         worldup;        /* world up-vector (almost screen up) */
  float           FOV;            /* horizontal field of view */
  GzGeoInfo       ginfo;
  void            operator =  ( const GzCamera &c )       { position = c.position; lookat = c.lookat; worldup = c.worldup; FOV = c.FOV; ginfo = c.ginfo; } 


} GzCamera;
#endif

#ifndef GZLIGHT
#define GZLIGHT

typedef struct  GzLight             
{
  int               type;
  GzColor           color;		    /* light color intensity */
  GzCoord           direction; 	/* vector from surface to light */     
  GzCoord           position; 
  float             angle;
  float             exponent;

  GzGeoInfo         ginfo;

  bool              enable;
  
  void              operator =  ( const GzLight &l )       { type      = l.type; 
                                                             color     = l.color; 
                                                             direction = l.direction; 
                                                             position  = l.position; 
                                                             angle     = l.angle; 
                                                             exponent  = l.exponent; 
                                                             ginfo     = l.ginfo; 
                                                             enable    = l.enable;  } 
} GzLight;


#endif

#ifndef GZINPUT
#define GZINPUT
typedef struct  GzInput
{

    GzCoord     rotation;       /* object rotation */
		GzCoord			translation;	/* object translation */
		GzCoord			scale;			/* object scaling */
		GzCamera		camera;			/* camera */
} GzInput;

#endif

#define RED     0               /* array indicies for color vector */
#define GREEN   1
#define BLUE    2

#define X       0               /* array indicies for position vector */
#define Y       1
#define Z       2

#define U       0               /* array indicies for texture coords */
#define V       1
