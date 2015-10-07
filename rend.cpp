/* CS580 Homework 3 */

#include <vector>
#include	"stdio.h"
#include	"math.h"
#include	"gz.h"
#include	"rend.h"

FILE *fopt = NULL;

int default_texFunc( float u, float v, GzColor color );

int GzUpdateMVPMatrix( GzRender *render );
int GzDrawTriangle( GzRender	*render, GzCoord *triangle, GzCoord *noraml, GzTextureIndex *uv );

int GzDrawSample( GzRender	*render, GzCoord *point, GzCoord *noraml, GzTextureIndex *uv, float zNear, float zFar );

void GzTextureIndexSwap( GzTextureIndex * a, GzTextureIndex * b);

void GzFloatSwap( float* a, float* b );
void GzCoordSwap( GzCoord *a, GzCoord *b );

void GzSortVerties( int numVert, GzCoord * verties, GzCoord * normals, GzTextureIndex *uv );

void swap( int* a,int* b );

//----------------------------------------------------------
// Begin main functions
int GzNewRender( GzRender **render, int width, int height )
{
  *render = (GzRender *)malloc(sizeof(GzRender));
  if (*render == NULL) return GZ_FAILURE;


  fopt = fopen( "rend.debug.txt", "w" );


  (*render)->open = false;
  (*render)->xres = width;
  (*render)->yres = height;
  (*render)->matlevel = 0;

  float halfXres = width / 2.0;
  float halfYres = height / 2.0;
  float Zres = INT_MAX;
  float m[16] = {  halfXres,         0,    0, halfXres,
                          0, -halfYres,    0, halfYres,
                          0,         0, Zres,        0,
                          0,         0,    0,        1 };
  (*render)->Xsp = GzMatrix(m);

  (*render)->Xps = GzMatrix(m);
  ((*render)->Xps).inverse();

 
  (*render)->numlights = 0;
  (*render)->interp_mode = GZ_RGB_COLOR;
  (*render)->Ks = DEFAULT_SPECULAR;
  (*render)->Kd = DEFAULT_DIFFUSE;
  (*render)->Ka = DEFAULT_AMBIENT;
  (*render)->ambientlight.color = GzColor( 0, 0, 0 );
  (*render)->spec = DEFAULT_SPEC;

  GzMatrix uniMat;
  uniMat.identity();
  
  for( int i = 0; i < MATLEVELS ; i++ ) {
    (*render)->Ximage[ i ] = uniMat;
    (*render)->Xnorm[ i ] = uniMat;
  }

  (*render)->tex_fun = NULL;
  (*render)->gbuf = NULL;

  (*render)->shader_func = NULL;
  (*render)->sharder_data = NULL;

  for( int i = 0; i < MAX_LIGHTS; i++ ) {
    (*render)->lights[i].enable = false;
  }

  (*render)->open = true;


  (*render)->samplesThreshold = 1;
  (*render)->numMicroView   = 0;
  (*render)->enable_sample  = false;
  (*render)->sampleMapping  = new std::map<GzCoord, bool> (); 
  (*render)->samples        = new std::vector<GzISMSample> (); 
  (*render)->microView      = new std::vector<int> (); 
  (*render)->sceneSize[0]   = GzCoord( INT_MAX, INT_MAX, INT_MAX );
  (*render)->sceneSize[1]   = GzCoord( INT_MAX, INT_MAX, INT_MAX ) * -1;
  
  (*render)->textureFileMapping  = new std::map< std::string, GzTextureFile > (); 
  

    
	return GZ_SUCCESS;
}

void GzSetSample( GzRender *render, bool enable ) 
{ 
  render->enable_sample = enable; 
}

int GzSetScreenSize( GzRender *render, int width, int height )
{
  if( render == NULL ) return GZ_FAILURE;
  float halfXres = width / 2.0;
  float halfYres = height / 2.0;
  float Zres = INT_MAX;
  float m[16] = {  halfXres,         0,    0, halfXres,
                          0, -halfYres,    0, halfYres,
                          0,         0, Zres,        0,
                          0,         0,    0,        1 };
  render->Xsp = GzMatrix(m);

  
  render->Xps = GzMatrix(m);
  render->Xps.inverse();

  render->xres = width;
  render->yres = height;
  GzUpdateMVPMatrix( render );
  return GZ_SUCCESS;
}


int GzFreeRender(GzRender *render)
{
/* 
-free all renderer resources
*/
  if( render->camera.ginfo.gbuf ) free( render->camera.ginfo.gbuf );
  
  for( int i = 0; i < render->numlights; i++ ) {
    if( render->lights[i].ginfo.gbuf ) free( render->lights[i].ginfo.gbuf );
  }

  for( int i = 0; i < (int) render->samples->size(); i++ ) {
    if( render->samples->at( i ).ISM.gbuf ) free( render->samples->at( i ).ISM.gbuf );
  }

  delete( render->sampleMapping );
  delete( render->samples );
  delete( render->microView );

  for( std::map< std::string, GzTextureFile >::iterator it = render->textureFileMapping->begin(); it != render->textureFileMapping->end(); ++it ) {
    GzTextureFile texture = it->second;
    free( texture.image );
  }
  delete( render->textureFileMapping );

  free( render );
	return GZ_SUCCESS;
}

GzMatrix GzMatrixLookAt( GzCoord eye, GzCoord center, GzCoord worldup )
{
  GzCoord forward, up, left, translation;
  forward = center - eye;
  translation = forward;
  forward = forward.normal();

  if( 1 == abs( worldup || forward ) ) worldup[0] = abs( worldup[0] ) + 1;

  left = ( worldup * forward ).normal();
  up = ( forward * left ).normal();
  translation = GzCoord( -1 * ( eye || left ), -1 * ( eye || up ), -1 * ( eye || forward ) );

  float m[16] = {     left[0],    left[1],    left[2], translation[0],
                        up[0],      up[1],      up[2], translation[1],
                   forward[0], forward[1], forward[2], translation[2],
                            0,          0,          0,              1 };

  GzMatrix matrix = m;
  return matrix;
}


GzMatrix GzFrustum( float left, float right, float bottom, float top, float znear, float zfar)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    float m[16] = { temp / temp2,          0.0, -1 * (right + left) / temp2,                     0.0,
                             0.0, temp / temp3, -1 * (top + bottom) / temp3,                     0.0,
                             0.0,          0.0,                zfar / temp4, (-znear * zfar) / temp4,
                             0.0,          0.0,                         1.0,                     0.0 };
    GzMatrix matrix = m;
    return matrix;
}

GzMatrix GzPersProjection( float fovyInDegrees, float aspectRatio, float znear, float zfar )
{
    float ymax, xmax;
    ymax = znear * tanf( fovyInDegrees * 3.141593f / 360.0 );
    xmax = ymax * aspectRatio;
    GzMatrix matrix = GzFrustum( -xmax, xmax, -ymax, ymax, znear, zfar);
    return matrix;
}

/* transform the cube, bounded by coordinate min and max into an -1~1 normalized cube  */
GzMatrix GzOrthProjection( GzCoord min, GzCoord max )
{
  float left, bottom, znear, right, top, zfar;

  left    = min[0];
  bottom  = min[1];
  znear   = min[2] - ( max[2] - min[2] );
  right   = max[0];
  top     = max[1];
  zfar    = max[2];
  

  float m[16] = {  2 / ( right - left ),                    0,                     0, -1 * ( right + left ) / ( right - left ),
                                      0, 2 / ( top - bottom ),                     0, -1 * ( top + bottom ) / ( top - bottom ),
                                      0,                    0,  2 / ( zfar - znear ), -1 * ( zfar + znear ) / ( zfar - znear ),
                                      0,                    0,                     0,                                        1 };
  GzMatrix matrix = m;
  return matrix;
}

int GzUpdateMVPMatrix( GzRender *render )
{
  if( render == NULL ) return GZ_FAILURE;

  render->Ximage[ MATLEVELS - 1 ].identity();
  render->Xnorm[ MATLEVELS - 1 ].identity();

  render->Ximage[ MATLEVELS - 1 ] = render->Xpc * render->Xcw;

  render->Xws = render->Xsp * render->Ximage[ MATLEVELS - 1 ];
  render->Xws.inverse();
 
  render->Xnpc = render->Xpc.normal();
  render->Xncw = render->Xcw.normal();
  render->Xnorm[ MATLEVELS - 1 ] = render->Xncw;

  render->Xnws = render->Xnorm[ MATLEVELS - 1 ];
  render->Xnws.inverse();

  for( int i = 0; i < render->matlevel; i++ ) {
    render->Ximage[ MATLEVELS - 1 ] = render->Ximage[ MATLEVELS - 1 ] * render->Ximage[ i ];
    render->Xnorm[ MATLEVELS - 1 ] = render->Xnorm[ MATLEVELS - 1 ] * render->Xnorm[ i ];
  }

  return GZ_SUCCESS;
}

int GzSetViewMatrix( GzRender	*render, GzMatrix mat )
{
  if( render == NULL ) return GZ_FAILURE;
  render->Xcw = mat;
  GzUpdateMVPMatrix( render );
  return GZ_SUCCESS;
}

int GzSetProjectMatrix( GzRender	*render, GzMatrix mat )
{
  if( render == NULL ) return GZ_FAILURE;
  render->Xpc = mat;

 // render->Xpc.identity();
  GzUpdateMVPMatrix( render );
  return GZ_SUCCESS;
}

int GzPushMatrix(GzRender *render, GzMatrix	matrix)
{
/*
- push a matrix onto the Ximage stack
- check for stack overflow
- only used for model -> world transfer
*/
  // the render->Ximage[MATLEVELS-1] is the final matrix (merge all matrics in the stack)
  // the render->Xnorm[MATLEVELS-1]  is the final matrix (merge all matrics in the stack, except the bottom one). The bottom matrix is 

  if( render == NULL ) return GZ_FAILURE;
  if( render->matlevel + 1 >= MATLEVELS) return GZ_FAILURE;

  render->Ximage[ render->matlevel ] = matrix;
  render->Xnorm[ render->matlevel ] = matrix.normal();

  render->Ximage[ MATLEVELS - 1 ] = render->Ximage[ MATLEVELS - 1 ] * render->Ximage[ render->matlevel ];
  render->Xnorm[ MATLEVELS - 1 ] = render->Xnorm[ MATLEVELS - 1 ] * render->Xnorm[ render->matlevel ];

  render->matlevel += 1;

	return GZ_SUCCESS;
}

int GzPopMatrix(GzRender *render)
{
/*
- pop a matrix off the Ximage stack
- check for stack underflow
*/
  if( render == NULL ) return GZ_FAILURE;
  if( render->matlevel <= 0 ) return GZ_FAILURE;

  render->Ximage[ render->matlevel - 1 ].inverse();
  render->Ximage[ MATLEVELS - 1 ] = render->Ximage[ MATLEVELS - 1 ] * render->Ximage[ render->matlevel - 1 ];

  render->Xnorm[ render->matlevel - 1 ].inverse();
  render->Xnorm[ MATLEVELS - 1 ] = render->Xnorm[ MATLEVELS - 1 ] * render->Xnorm[ render->matlevel - 1 ];

  render->matlevel -= 1;
	return GZ_SUCCESS;
}

int GzClearMatrix(GzRender *render)
{
  if( render == NULL ) return GZ_FAILURE;
  while( GzPopMatrix( render ) == GZ_SUCCESS ) {}
  return GZ_SUCCESS;
}

int GzPutAttribute(GzRender	*render, int numAttributes, GzToken	*nameList, 
	GzPointer	*valueList) /* void** valuelist */
{
/*
- set renderer attribute states (e.g.: GZ_RGB_COLOR default color)
- later set shaders, interpolaters, texture maps, and lights
*/
  if( nameList == NULL ) return GZ_FAILURE;
  if( valueList == NULL ) return GZ_FAILURE;
  if( render == NULL ) return GZ_FAILURE;

  static const GzCoord cube[6] = { GzCoord( 1, 0, 0 ), GzCoord( -1, 0, 0 ), GzCoord( 0, 1, 0 ), GzCoord( 0, -1, 0 ), GzCoord( 0, 0, 1 ), GzCoord( 0, 0, -1 ) };

  for( int i = 0; i < numAttributes; i++ ) {

    if( nameList[i] == GZ_RGB_COLOR ) {
      GzColor *color = ( GzColor * ) valueList[i];
      render->flatcolor = (*color);

    } else if( nameList[i] == GZ_LIGHT ) {
      if (render->numlights > MAX_LIGHTS) return GZ_FAILURE;
      GzLight* light = ( GzLight * )( valueList[i] );

      if( light->type == GZ_POINT_LIGHT ) {
        for( int j = 0; j < 6; j++ ) {
          render->lights[ render->numlights ].type = light->type;
          render->lights[ render->numlights ].color = light->color;
          render->lights[ render->numlights ].direction = cube[j];
          render->lights[ render->numlights ].position = light->position;
          render->lights[ render->numlights ].angle = 90;
          render->lights[ render->numlights ].exponent = light->exponent;
          render->lights[ render->numlights ].ginfo.gbuf = NULL;
          render->lights[ render->numlights ].enable = true;
          render->numlights += 1;
        }
        
      } else {
        render->lights[ render->numlights ].type = light->type;
        render->lights[ render->numlights ].color = light->color;
        render->lights[ render->numlights ].direction = light->direction.normal();
        render->lights[ render->numlights ].position = light->position;
        render->lights[ render->numlights ].angle = light->angle;
        render->lights[ render->numlights ].exponent = light->exponent;
        render->lights[ render->numlights ].ginfo.gbuf = NULL;
        render->lights[ render->numlights ].enable = true;
        render->numlights += 1;
      }

    } else if( nameList[i] == GZ_AMBIENT_LIGHT ) {
      GzLight* light = ( GzLight * )( valueList[i] );
      render->ambientlight.color = light->color;

    } else if( nameList[i] == GZ_SPECULAR_COEFFICIENT ) {
      GzColor* color = ( GzColor * )( valueList[i] );
      render->Ks = *color;

    } else if( nameList[i] == GZ_DIFFUSE_COEFFICIENT ) {
      GzColor* color = ( GzColor * )( valueList[i] );
      render->Kd = *color;

    } else if( nameList[i] == GZ_AMBIENT_COEFFICIENT ) {
      GzColor* color = ( GzColor * )( valueList[i] );
      render->Ka = *color;

    } else if( nameList[i] == GZ_DISTRIBUTION_COEFFICIENT ) {
      float * v = ( float * )( valueList[i] );
			render->spec = *v;

    } else if( nameList[i] == GZ_INTERPOLATE ) {
      int * v = ( int * )( valueList[i] );
			render->interp_mode = *v;

    } else if( nameList[i] == GZ_TEXTURE_MAP ) {
      GzTexture texture = ( GzTexture ) ( valueList[i] );
			render->tex_fun = texture;

    } else if( nameList[i] == GZ_SHADER_FUNC ) {
      GzShader shader = ( GzShader ) ( valueList[i] );
			render->shader_func = shader;

    } else if( nameList[i] == GZ_DATA ) {
      GzShaderData * datalist = ( GzShaderData * ) ( valueList[i] );
			render->sharder_data = datalist;


    } else if( nameList[i] == GZ_SAMPLE_COEFFICIENT ) {
      float * v = ( float * )( valueList[i] );
			render->samplesThreshold = *v;

    }
  }
	return GZ_SUCCESS;
}

int GzEnableLight(GzRender	*render )
{
  if( render == NULL ) return GZ_FAILURE;
  for( int i = 0; i < render->numlights; i++ ) {
    render->lights[i].enable = true;
  }
  return GZ_SUCCESS;
}

int GzDisableLight(GzRender	*render )
{
  if( render == NULL ) return GZ_FAILURE;
  for( int i = 0; i < render->numlights; i++ ) {
    render->lights[i].enable = false;
  }
  return GZ_SUCCESS;
}

int GzEnableLight(GzRender	*render, int index )
{
  if( render == NULL ) return GZ_FAILURE;
  if( index < 0 ) return GZ_FAILURE;
  if( index >= render->numlights ) return GZ_FAILURE;
  render->lights[index].enable = true;
  return GZ_SUCCESS;
}
int GzDisableLight(GzRender	*render, int index )
{
  if( render == NULL ) return GZ_FAILURE;
  if( index < 0 ) return GZ_FAILURE;
  if( index >= render->numlights ) return GZ_FAILURE;
  render->lights[index].enable = false;
  return GZ_SUCCESS;
}

int GzGetLight(GzRender	*render, int index, GzLight * light ) 
{
  if( render == NULL ) return GZ_FAILURE;
  if( index >= render->numlights ) return GZ_FAILURE;
  light->color = render->lights[ index ].color;
  light->direction = render->lights[ index ].direction;
  light->position = render->lights[ index ].position;
  light->angle = render->lights[ index ].angle;
	return GZ_SUCCESS;
}

int GzSaveGInfo(GzRender *render, GzGeoInfo *ginfo )
{
  if( render == NULL ) return GZ_FAILURE;
  if( ginfo == NULL ) return GZ_FAILURE;

  ginfo->Xcw = render->Xcw;
  ginfo->Xpc = render->Xpc;
  ginfo->Xsp = render->Xsp;
  ginfo->gbuf = render->gbuf;
  ginfo->xres = render->xres;
  ginfo->yres = render->yres;
	return GZ_SUCCESS;
}

int GzBindMatrices(GzRender *render, GzGeoInfo ginfo )
{
  if( render == NULL ) return GZ_FAILURE;
//  if( ginfo.xres != render->xres ) return GZ_FAILURE;
//  if( ginfo.yres != render->yres ) return GZ_FAILURE;

  render->Xcw = ginfo.Xcw;
  render->Xpc = ginfo.Xpc;
  render->Xsp = ginfo.Xsp;
  GzUpdateMVPMatrix( render );
	return GZ_SUCCESS;
}



int GzBindMatrices(GzRender	*render, GzLight light )
{
  if( render == NULL ) return GZ_FAILURE;
  render->Xcw = light.ginfo.Xcw;
  render->Xpc = light.ginfo.Xpc;
  render->Xsp = light.ginfo.Xsp;
  GzUpdateMVPMatrix( render );
	return GZ_SUCCESS;
}

int GzBindMatrices(GzRender	*render, GzCamera camera )
{
  if( render == NULL ) return GZ_FAILURE;
  render->Xcw = camera.ginfo.Xcw;
  render->Xpc = camera.ginfo.Xpc;
  render->Xsp = camera.ginfo.Xsp;
  GzUpdateMVPMatrix( render );
	return GZ_SUCCESS;
}

void GzCoordIntersectZplan( GzCoord v1, GzCoord v2,
                            GzCoord n1, GzCoord n2, 
                            GzTextureIndex uv1, GzTextureIndex uv2, 
                            float z, GzCoord *result_v, GzCoord *result_n, GzTextureIndex * result_uv )

{
  float t = ( z - v1[2] ) / ( v2[2] - v1[2] );
  *result_v = v1 + ( v2 - v1 ) * t;

  GzCoord vector[2];
  vector[0] = *result_v - v1;
  vector[1] = *result_v - v2;

  float dist[2];
  dist[0] = sqrt( vector[0][0] * vector[0][0] + vector[0][1] * vector[0][1] + vector[0][2] * vector[0][2] );
  dist[1] = sqrt( vector[1][0] * vector[1][0] + vector[1][1] * vector[1][1] + vector[1][2] * vector[1][2] );

  float f = dist[1] / ( dist[0] + dist[1] );

  *result_n = n1 * f + n2 * ( 1 - f );

  *result_uv = uv1 *f + uv2 * ( 1 - f );

}


int GzClippingZnearPlan( GzRender	*render, GzCoord *triangle, GzCoord *normal, GzTextureIndex *uv )
{
  static const bool debug = false;

  for( int i = 0; i < 3 && debug; i++ ) {

    fprintf( stderr, "(%f,%f,%f,%f)(%f,%f,%f,%f) ->", triangle[i][0], triangle[i][1], triangle[i][2], triangle[i][3], normal[i][0], normal[i][1], normal[i][2], normal[i][3]);
    if( i == 2 ) fprintf( stderr, "-------------\n" );

  }
  
  float w[3];
  for( int i = 0; i < 3; i++ ) {
    triangle[i][3] = 1.0;
    triangle[i] = render->Ximage[ MATLEVELS - 1 ] * triangle[i];
    w[i] = triangle[i][3];
  }

  for( int i = 0; i < 3; i++ ) {
    normal[i][3] = 1.0; 
    normal[i] = ( render->Xnorm[ MATLEVELS - 1 ] * normal[i] ).dW();
  }

  float z = 0;

  float znear;
  if( render->Xpc[3][2] == 0 ) znear = 1;
  else znear = -1 * render->Xpc[2][3] / render->Xpc[2][2];
  
  // sort by z-axis

  if( triangle[0][2] > triangle[1][2] ) { GzCoordSwap( &(triangle[0]), &(triangle[1]) ); GzCoordSwap( &(normal[0]), &(normal[1]) ); GzTextureIndexSwap( &(uv[0]), &(uv[1]) ); GzFloatSwap( &w[0], &w[1] ); }
  if( triangle[1][2] > triangle[2][2] ) { GzCoordSwap( &(triangle[1]), &(triangle[2]) ); GzCoordSwap( &(normal[1]), &(normal[2]) ); GzTextureIndexSwap( &(uv[1]), &(uv[2]) ); GzFloatSwap( &w[1], &w[2] ); }
  if( triangle[0][2] > triangle[1][2] ) { GzCoordSwap( &(triangle[0]), &(triangle[1]) ); GzCoordSwap( &(normal[0]), &(normal[1]) ); GzTextureIndexSwap( &(uv[0]), &(uv[1])); GzFloatSwap( &w[0], &w[1] ); }

  for( int i = 0; i < 3 && debug; i++ ) {
    fprintf( stderr, "(%f,%f,%f,%f)(%f,%f)\n", triangle[i][0], triangle[i][1], triangle[i][2], triangle[i][3], uv[i][0], uv[i][1] );
    if( i == 2 ) fprintf( stderr, "-------------\n" );

  }

  GzCoord org_v[3];
  GzCoord org_n[3];
  GzTextureIndex org_uv[3];
  float org_w[3];
  for( int i = 0; i < 3; i++ ) {
    org_v[i] = triangle[i];
    org_n[i] = normal[i];

    org_uv[i] = uv[i];

    org_w[i] = w[i];
  }

  if( z < triangle[0][2] ) { 

//    for( int i = 0; i < 3; i++ ) triangle[i] = triangle[i] / w[i];
    for( int i = 0; i < 3; i++ ) triangle[i][3] = w[i];    
    

    for( int i = 0; i < 3 && debug; i++ ) {
      if( i == 0 ) fprintf( fopt, "\n^^^^^^[1]^^^^^^^\n" );
      fprintf( fopt, "(%f,%f,%f,%f)\t", triangle[i][0], triangle[i][1], triangle[i][2], w[i]);
      if( i == 2 ) fprintf( fopt, "\n^^^^^^^^^^^^^\n" );
    } 

    GzDrawTriangle( render, triangle, normal, uv );
  }
  else if( z < triangle[1][2] ) { 
    GzCoord new_v[2];
    GzCoord new_n[2];
    GzTextureIndex new_uv[2];    

    if( triangle[1][0] > triangle[2][0] ) { 

      GzCoordSwap( &(triangle[1]), &(triangle[2]) ); GzCoordSwap( &(normal[1]), &(normal[2]) ); GzTextureIndexSwap( &(uv[1]), &(uv[2]) ); GzFloatSwap( &w[1], &w[2] ); 
      org_v[1] = triangle[1]; org_n[1] = normal[1]; org_uv[1] = uv[1]; org_w[1] = w[1];
      org_v[2] = triangle[2]; org_n[2] = normal[2]; org_uv[2] = uv[2]; org_w[2] = w[2];
    }

    GzCoordIntersectZplan( triangle[0], triangle[1], normal[0], normal[1], uv[0], uv[1], z, &(new_v[0]), &(new_n[0]), &(new_uv[0]) );
    GzCoordIntersectZplan( triangle[0], triangle[2], normal[0], normal[2], uv[0], uv[2], z, &(new_v[1]), &(new_n[1]), &(new_uv[1]) );


    triangle[0] = new_v[0]; normal[0] = new_n[0]; uv[0] = new_uv[0]; w[0] = znear;

    for( int i = 0; i < 3 && debug; i++ ) {
      if( i == 0 ) fprintf( fopt, "\n^^^^^^[2.1]^^^^^^^\n" );
      fprintf( fopt, "(%f,%f,%f,%f)\t", triangle[i][0], triangle[i][1], triangle[i][2], w[i]);
      if( i == 2 ) fprintf( fopt, "\n^^^^^^^^^^^^^\n" );
    } 

//    for( int i = 0; i < 3; i++ ) triangle[i] = triangle[i] / w[i];
    for( int i = 0; i < 3; i++ ) triangle[i][3] = w[i];     
    GzDrawTriangle( render, triangle, normal, uv );

    triangle[0] = new_v[0]; normal[0] = new_n[0]; uv[0] = new_uv[0]; w[0] = znear;
    triangle[1] = new_v[1]; normal[1] = new_n[1]; uv[1] = new_uv[1]; w[1] = znear;
    triangle[2] = org_v[2]; normal[2] = org_n[2]; uv[2] = org_uv[2]; w[2] = org_w[2];

    for( int i = 0; i < 3 && debug; i++ ) {
      if( i == 0 ) fprintf( fopt, "\n^^^^^^[2.2]^^^^^^^\n" );
      fprintf( fopt, "(%f,%f,%f,%f)\t", triangle[i][0], triangle[i][1], triangle[i][2], w[i]);
      if( i == 2 ) fprintf( fopt, "\n^^^^^^^^^^^^^\n" );
    } 

//    for( int i = 0; i < 3; i++ ) triangle[i] = triangle[i] / w[i];
    for( int i = 0; i < 3; i++ ) triangle[i][3] = w[i];     
    GzDrawTriangle( render, triangle, normal, uv );

  } else if( z < triangle[2][2] ) { 
    GzCoordIntersectZplan( triangle[0], triangle[2], normal[0], normal[2], uv[0], uv[2], z, &(triangle[0]), &(normal[0]), &(uv[0]) );
    GzCoordIntersectZplan( triangle[1], triangle[2], normal[1], normal[2], uv[1], uv[2], z, &(triangle[1]), &(normal[1]), &(uv[1]) );

    w[0] = znear; w[1] = znear;
    for( int i = 0; i < 3 && debug; i++ ) {
      if( i == 0 ) fprintf( fopt, "\n^^^^^^[3]^^^^^^^\n" );
      fprintf( fopt, "(%f,%f,%f,%f)\t", triangle[i][0], triangle[i][1], triangle[i][2], w[i]);
      if( i == 2 ) fprintf( fopt, "\n^^^^^^^^^^^^^\n" );
    } 

//    for( int i = 0; i < 3; i++ ) triangle[i] = triangle[i] / w[i];
    for( int i = 0; i < 3; i++ ) triangle[i][3] = w[i];    

    GzDrawTriangle( render, triangle, normal, uv );

  } else {
  
    return GZ_FAILURE;
  }
  return GZ_SUCCESS;
}



bool operator<(const GzCoord & lhs, const GzCoord & rhs) // lhs = left-hand side
                                                      // rhs = right-hand side
{
    if (lhs.v[0] != rhs.v[0]) return lhs.v[0] < rhs.v[0];
    else if (lhs.v[1] != rhs.v[1]) return lhs.v[1] < rhs.v[1];
    else return lhs.v[2] < rhs.v[2];
}




int GzSamplePoints( GzRender	*render, GzCoord *triangle, GzCoord *normal, GzTextureIndex *uv, float threshold )
{
  GzCoord w = triangle[1] - triangle[0];
  GzCoord v = triangle[2] - triangle[0];
  float area = 0.5 * ( w * v ).length();

  
  GzCoord centerVertex = ( triangle[0] + triangle[1] + triangle[2] ) / 3;
  GzCoord centerNormal = ( normal[0] + normal[1] + normal[2] ) / 3;
  GzTextureIndex centerTexture = ( uv[0] + uv[1] + uv[2] ) / 3;


  if( area > threshold  ) {

    float angle0 = ( ( triangle[1] - triangle[0] ) || ( triangle[2] - triangle[0] ) ) / ( ( triangle[1] - triangle[0] ).length() * ( triangle[2] - triangle[0] ).length() );
    float angle1 = ( ( triangle[0] - triangle[1] ) || ( triangle[2] - triangle[1] ) ) / ( ( triangle[0] - triangle[1] ).length() * ( triangle[2] - triangle[1] ).length() );
    float angle2 = ( ( triangle[1] - triangle[2] ) || ( triangle[0] - triangle[2] ) ) / ( ( triangle[1] - triangle[2] ).length() * ( triangle[0] - triangle[2] ).length() );

    if( angle0 < angle1 && angle0 < angle2 ) {
      GzCoord centerVertex = ( triangle[1] + triangle[2] ) / 2;
      GzCoord centerNormal = ( normal[1] + normal[2] ) / 2;

      GzTextureIndex centerTexture = ( uv[1] + uv[2] ) / 2;
      GzCoord newTriangle[3];
      GzCoord newNormal[3];    
      GzTextureIndex newTextureIndex[3];
  
      newTriangle[0] = triangle[0]; newTriangle[1] = triangle[1]; newTriangle[2] = centerVertex; 
      newNormal[0] = normal[0]; newNormal[1] = normal[1]; newNormal[2] = centerNormal; 
      newTextureIndex[0] = uv[0]; newTextureIndex[1] = uv[1]; newTextureIndex[2] = centerTexture; 
      GzSamplePoints( render, newTriangle, newNormal, newTextureIndex, threshold );
  
      newTriangle[0] = triangle[0]; newTriangle[1] = centerVertex; newTriangle[2] = triangle[2]; 
      newNormal[0] = normal[0]; newNormal[1] = centerNormal; newNormal[2] = normal[2]; 
      newTextureIndex[0] = uv[0]; newTextureIndex[1] = centerTexture; newTextureIndex[2] = uv[2]; 
      GzSamplePoints( render, newTriangle, newNormal, newTextureIndex, threshold );


    } else if( angle1 < angle0 && angle1 < angle2 ) {
      GzCoord centerVertex = ( triangle[0] + triangle[2] ) / 2;
      GzCoord centerNormal = ( normal[0] + normal[2] ) / 2;

      GzTextureIndex centerTexture = ( uv[0] + uv[2] ) / 2;
      GzCoord newTriangle[3];
      GzCoord newNormal[3];    
      GzTextureIndex newTextureIndex[3];
  
      newTriangle[0] = triangle[0]; newTriangle[1] = triangle[1]; newTriangle[2] = centerVertex; 
      newNormal[0] = normal[0]; newNormal[1] = normal[1]; newNormal[2] = centerNormal; 
      newTextureIndex[0] = uv[0]; newTextureIndex[1] = uv[1]; newTextureIndex[2] = centerTexture; 

      GzSamplePoints( render, newTriangle, newNormal, uv, threshold );
  
      newTriangle[0] = centerVertex; newTriangle[1] = triangle[1]; newTriangle[2] = triangle[2]; 
      newNormal[0] = centerNormal; newNormal[1] = normal[1]; newNormal[2] = normal[2]; 

      newTextureIndex[0] = centerTexture; newTextureIndex[1] = uv[1]; newTextureIndex[2] = uv[2]; 
      GzSamplePoints( render, newTriangle, newNormal, newTextureIndex, threshold );


    } else {
    
      GzCoord centerVertex = ( triangle[0] + triangle[1] ) / 2;
      GzCoord centerNormal = ( normal[0] + normal[1] ) / 2;

      GzTextureIndex centerTexture = ( uv[0] + uv[1] ) / 2;
      GzCoord newTriangle[3];
      GzCoord newNormal[3];    
      GzTextureIndex newTextureIndex[3];

      newTriangle[0] = triangle[0]; newTriangle[1] = centerVertex; newTriangle[2] = triangle[2]; 
      newNormal[0] = normal[0]; newNormal[1] = centerNormal; newNormal[2] = normal[2]; 
      newTextureIndex[0] = uv[0]; newTextureIndex[1] = centerTexture; newTextureIndex[2] = uv[2]; 

      GzSamplePoints( render, newTriangle, newNormal, uv, threshold );
  
      newTriangle[0] = centerVertex; newTriangle[1] = triangle[1]; newTriangle[2] = triangle[2]; 
      newNormal[0] = centerNormal; newNormal[1] = normal[1]; newNormal[2] = normal[2]; 

      newTextureIndex[0] = centerTexture; newTextureIndex[1] = uv[1]; newTextureIndex[2] = uv[2]; 
      GzSamplePoints( render, newTriangle, newNormal, newTextureIndex, threshold );

    }

  } else {
    
    GzISMSample sample;
    sample.ISM.gbuf = NULL;
    sample.sample.pos = ( ( render->Xws * render->Xsp * render->Ximage[ MATLEVELS - 1 ] ) * centerVertex ).dW();
    sample.sample.normal = ( ( render->Xnws * render->Xnorm[ MATLEVELS - 1 ] ) * centerNormal ).dW(); 
    
//    float r = 1.0 / 20.0;
    float r = 50.0;
    GzCoord filterKey = GzCoord( (int)(sample.sample.pos[0] * r), (int)(sample.sample.pos[1] * r), (int)(sample.sample.pos[2] * r), 1.0 );
//    printf("(%f,%f,%f)\n", filterKey[0], filterKey[1], filterKey[2]);
    
    /* texture lookup ?? */
    if( render->tex_fun ) {
      GzColor textColor = GzColor( 1.0, 1.0, 1.0 );
      render->tex_fun( render, centerTexture[0], centerTexture[1], &textColor );
      sample.sample.Ka = textColor;
      sample.sample.Kd = textColor;
    } else {
      sample.sample.Ka = render->Ka;
      sample.sample.Kd = render->Kd;
    }
    sample.sample.Ks = render->Ks;
    
    if( render->sampleMapping->find( filterKey ) == render->sampleMapping->end() ) {
      std::map< GzCoord, bool > *map = render->sampleMapping;
      (*map)[ filterKey ] = true;
      render->samples->push_back( sample );
    }
    

  }


  return GZ_SUCCESS;
}

int GzPutTriangle(GzRender	*render, int numParts, GzToken *nameList, GzPointer	*valueList )
{
  if( nameList == NULL ) return GZ_FAILURE;
  if( valueList == NULL ) return GZ_FAILURE;
  if( render == NULL ) return GZ_FAILURE;

  /* need to bind g-buffer first */
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzCoord *triangle = NULL;
  GzCoord *normal = NULL; 
  GzTextureIndex *uv = NULL;

  for( int i = 0; i < numParts; i++ ) {
    if( nameList[i] == GZ_POSITION ) triangle = ( GzCoord * ) valueList[i];
    else if( nameList[i] == GZ_NORMAL ) normal = ( GzCoord * ) valueList[i];
    else if( nameList[i] == GZ_TEXTURE_INDEX ) uv = ( GzTextureIndex * ) valueList[i];
    else if( nameList[i] == GZ_NULL_TOKEN ) { } 
  }

  for( int i = 0; i < 3; i++ ) {
    if( triangle ) triangle[i][3] = 1.0;
    if( normal ) normal[i][3] = 1.0;

      
    for( int j = 0; j < 3; j++ ) {
      if( triangle[i][j] < render->sceneSize[0][j] ) render->sceneSize[0][j] = triangle[i][j];
      if( triangle[i][j] > render->sceneSize[1][j] ) render->sceneSize[1][j] = triangle[i][j];
    }
  }
  
  if( render->enable_sample ) GzSamplePoints( render, triangle, normal, uv, render->samplesThreshold / 3.0 );


  GzClippingZnearPlan( render, triangle, normal, uv );

  return GZ_SUCCESS;
}


int GzDrawTriangle( GzRender	*render, GzCoord *triangle, GzCoord *noraml, GzTextureIndex *uv )
{
  /*
    vertiex and normal are in perspective space.
  
  */

  
  GzSortVerties( 3, triangle, noraml, uv );
  
  float invW[3];
  for( int i = 0; i < 3; i++ ) {
    invW[i] = 1.0 / triangle[i][3];
    triangle[i] = ( triangle[i] ).dW();
  }  

  float minX = std::min( triangle[0][0], std::min( triangle[1][0], triangle[2][0] ) );
  float maxX = std::max( triangle[0][0], std::max( triangle[1][0], triangle[2][0] ) );
  float minY = std::min( triangle[0][1], std::min( triangle[1][1], triangle[2][1] ) );
  float maxY = std::max( triangle[0][1], std::max( triangle[1][1], triangle[2][1] ) );
  float maxZ = std::max( triangle[0][2], std::max( triangle[1][1], triangle[2][2] ) );

  if( maxZ < 0 || 0 ) { /* behind the camera */ }
  else if( minX > 1.0 || maxX < -1.0 || minY > 1.0 || maxY < -1.0 ) { /* out of the screen */ }
  else {


    // det = 2 * area of the triangle
    float det = triangle[0][0] * triangle[1][1] + triangle[2][0] * triangle[0][1] + triangle[1][0] * triangle[2][1] - triangle[2][0] * triangle[1][1] - triangle[1][0] * triangle[0][1] - triangle[0][0] * triangle[2][1];
      
    // if the area < 0, the vertics is not CCW. Swap the verties
    if( det < 0 ) {
      GzCoordSwap( &(triangle[1]), &(triangle[2]) );
      if( noraml ) GzCoordSwap( &(noraml[1]), &(noraml[2]) );

      if( uv ) GzTextureIndexSwap( &(uv[1]), &(uv[2]) );
      GzFloatSwap( &(invW[1]), &(invW[2]) );

      det = det * -1;
    }  
    float invDet = 1 / det;

    // Edge equation: E(x,y) = Ax + By + C
    // E0(x,y) = A0x + B0y + C0 is from v1 and v2
    // E1(x,y) = A1x + B1y + C1 is from v2 and v0
    // E2(x,y) = A2x + B2y + C2 is from v0 and v1
    // Triangle interpolation
    float A0 = triangle[1][1] - triangle[2][1];
    float B0 = triangle[2][0] - triangle[1][0];
    float C0 = triangle[1][0] * triangle[2][1] - triangle[2][0] * triangle[1][1];
    float A1 = triangle[2][1] - triangle[0][1];
    float B1 = triangle[0][0] - triangle[2][0];
    float C1 = triangle[2][0] * triangle[0][1] - triangle[0][0] * triangle[2][1];
    float A2 = triangle[0][1] - triangle[1][1];
    float B2 = triangle[1][0] - triangle[0][0];
    float C2 = triangle[0][0] * triangle[1][1] - triangle[1][0] * triangle[0][1];
    float Az = ( triangle[0][2] * A0 + triangle[1][2] * A1 + triangle[2][2] * A2 ) * invDet;
    float Bz = ( triangle[0][2] * B0 + triangle[1][2] * B1 + triangle[2][2] * B2 ) * invDet;
    float Cz = ( triangle[0][2] * C0 + triangle[1][2] * C1 + triangle[2][2] * C2 ) * invDet;


    // Transform to screen space
    for( int i = 0; i < 3; i++ ) {
      triangle[i] = ( render->Xsp * triangle[i] ).dW();
    }


    // Normal vector interpolation
    float Anx, Bnx, Cnx, Any, Bny, Cny, Anz, Bnz, Cnz;
    Anx = ( noraml[0][0] * A0 + noraml[1][0] * A1 + noraml[2][0] * A2 ) * invDet;
    Bnx = ( noraml[0][0] * B0 + noraml[1][0] * B1 + noraml[2][0] * B2 ) * invDet;
    Cnx = ( noraml[0][0] * C0 + noraml[1][0] * C1 + noraml[2][0] * C2 ) * invDet;   
    Any = ( noraml[0][1] * A0 + noraml[1][1] * A1 + noraml[2][1] * A2 ) * invDet;
    Bny = ( noraml[0][1] * B0 + noraml[1][1] * B1 + noraml[2][1] * B2 ) * invDet;
    Cny = ( noraml[0][1] * C0 + noraml[1][1] * C1 + noraml[2][1] * C2 ) * invDet;   
    Anz = ( noraml[0][2] * A0 + noraml[1][2] * A1 + noraml[2][2] * A2 ) * invDet;
    Bnz = ( noraml[0][2] * B0 + noraml[1][2] * B1 + noraml[2][2] * B2 ) * invDet;
    Cnz = ( noraml[0][2] * C0 + noraml[1][2] * C1 + noraml[2][2] * C2 ) * invDet;    
 

    float Au, Bu, Cu, Av, Bv, Cv, Auv, Buv, Cuv;
    Au = Bu = Cu = Av = Bv = Cv = Auv = Buv = Cuv = 0.0;
    if( render->tex_fun ) {
      // UV interpolation
      GzTextureIndex UV[3];
      for( int i = 0; i < 3; i++ ) UV[i] = uv[i] * invW[i];
      Au = ( UV[0][0] * A0 + UV[1][0] * A1 + UV[2][0] * A2 ) * invDet;
      Bu = ( UV[0][0] * B0 + UV[1][0] * B1 + UV[2][0] * B2 ) * invDet;
      Cu = ( UV[0][0] * C0 + UV[1][0] * C1 + UV[2][0] * C2 ) * invDet;
      Av = ( UV[0][1] * A0 + UV[1][1] * A1 + UV[2][1] * A2 ) * invDet;
      Bv = ( UV[0][1] * B0 + UV[1][1] * B1 + UV[2][1] * B2 ) * invDet;
      Cv = ( UV[0][1] * C0 + UV[1][1] * C1 + UV[2][1] * C2 ) * invDet;
      Auv = ( invW[0] * A0 + invW[1] * A1 + invW[2] * A2 ) * invDet;
      Buv = ( invW[0] * B0 + invW[1] * B1 + invW[2] * B2 ) * invDet;
      Cuv = ( invW[0] * C0 + invW[1] * C1 + invW[2] * C2 ) * invDet;
    }

    minX = std::min( triangle[0][0], std::min( triangle[1][0], triangle[2][0] ) );
    maxX = std::max( triangle[0][0], std::max( triangle[1][0], triangle[2][0] ) );
    minY = std::min( triangle[0][1], std::min( triangle[1][1], triangle[2][1] ) );
    maxY = std::max( triangle[0][1], std::max( triangle[1][1], triangle[2][1] ) );


    if( minY < 0 ) minY = 0;
    if( maxY >= render->yres ) maxY = render->yres - 1;
    if( minX < 0 ) minX = 0;
    if( maxX >= render->xres ) maxX = render->xres - 1;

    for( int j = minY; j <= maxY; j++ ) {         
      for( int i = minX; i <= maxX; i++ ) {

        
        // Transfer to proj space, so that we can do interpolation
        GzCoord xy = ( render->Xps * GzCoord( i, j, 0 ) ).dW();

        float E0 = A0 * ( float ) xy[0] + B0 * ( float ) xy[1] + C0;
        float E1 = A1 * ( float ) xy[0] + B1 * ( float ) xy[1] + C1;
        float E2 = A2 * ( float ) xy[0] + B2 * ( float ) xy[1] + C2;


        if( E0 >= 0 && E1 >=0 && E2 >= 0 ) {

          GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

          GzDepth z = gbuf->depth;

          float zValuef = ( render->Xsp * GzCoord( xy[0], xy[1] ,Az * ( float ) xy[0] + Bz * ( float ) xy[1] + Cz  ).dW() )[2];


          if( ( GzDepth ) zValuef <= z && zValuef >= 0 ) {

            gbuf->depth = ( GzDepth ) zValuef;

            
            GzTextureIndex interp_uv;

            if( render->tex_fun ) {
              // texture color
              float p = 1 / ( Auv * ( float ) xy[0] + Buv * ( float ) xy[1] + Cuv );
              interp_uv = GzTextureIndex( ( Au * ( float ) xy[0] + Bu * ( float ) xy[1] + Cu ) * ( p ), ( Av * ( float ) xy[0] + Bv * ( float ) xy[1] + Cv ) * ( p ) );
              GzColor textColor = GzColor( 1.0, 1.0, 1.0 );
              render->tex_fun( render, interp_uv[0], interp_uv[1], &textColor );

              gbuf->Ka = textColor;
              gbuf->Kd = textColor;
              gbuf->Ks = render->Ks;
            } else {
              gbuf->Ka = render->Ka;
              gbuf->Kd = render->Kd;
              gbuf->Ks = render->Ks;
            }

            GzCoord interp_coord = GzCoord( i, j, zValuef );

            GzCoord interp_normal = GzCoord( Anx * ( float ) xy[0] + Bnx * ( float ) xy[1] + Cnx, Any * ( float ) xy[0] + Bny * ( float ) xy[1] + Cny, Anz * ( float ) xy[0] + Bnz * ( float ) xy[1] + Cnz );

            gbuf->pos = ( render->Xws * interp_coord ).dW();
            gbuf->normal = ( ( render->Xnws * interp_normal ).dW() ).normal();

          }
        }
      }
    }
/*
    GzDrawLine( render->gbuf, render->xres, render->yres, triangle[0][0], triangle[0][1], triangle[1][0], triangle[1][1], GzColor(1,1,1) );
    GzDrawLine( render->gbuf, render->xres, render->yres, triangle[1][0], triangle[1][1], triangle[2][0], triangle[2][1], GzColor(1,1,1) );
    GzDrawLine( render->gbuf, render->xres, render->yres, triangle[2][0], triangle[2][1], triangle[0][0], triangle[0][1], GzColor(1,1,1) );
*/
  }
//  fprintf( fopt, "\n");
	return GZ_SUCCESS;
}


int GzPutSample( GzRender	*render, float zNear, float zFar )

{
  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;


  GzGeoPixel *org_gbuf = render->gbuf;
  int org_xres = render->xres;
  int org_yres = render->yres;


  int nLevel = 4;
  GzGeoPixel **gbuf = ( GzGeoPixel ** ) malloc( sizeof( GzGeoPixel * ) * nLevel );
  for( int k = 0; k < nLevel; k++ ) {
    int factor = pow( 2, k );
    int width = org_xres / factor;
    int height = org_yres / factor;

    GzSetScreenSize( render, width, height );
    GzNewGBuffer( render, &(gbuf[k]) );  
	  GzBindGBuffer( render, gbuf[k] );
    
    for( int i = 0; i < (int) render->samples->size(); i++ ) {

      GzDrawSample( render, &(render->samples->at(i).sample.pos), &(render->samples->at(i).sample.normal), NULL, zNear, zFar );

    }
  }
  
  GzSetScreenSize( render, org_xres, org_yres );
  GzBindGBuffer( render, org_gbuf );
  
  for( int j = 0; j < render->yres; j++ ) {         
    for( int i = 0; i < render->xres; i++ ) {

      int factor = pow( 2, nLevel - 1 );
      int x = (int) ( i / factor );
      int y = (int) ( j / factor );
      GzCoord pos = gbuf[nLevel - 1][ x + y * ( render->xres / factor ) ].pos;
      GzDepth depth = gbuf[nLevel - 1][ x + y * ( render->xres / factor ) ].depth;


      for( int k = nLevel - 2; k >= 0 && 1; k-- ) {

        factor = pow( 2, k );
        x = (int) ( i / factor );
        y = (int) ( j / factor );
        if( gbuf[k][ x + y * ( render->xres / factor ) ].depth != INT_MAX ) {
          pos = ( pos + gbuf[k][ x + y * ( render->xres / factor ) ].pos ) / 2;
          depth = ( depth + gbuf[k][ x + y * ( render->xres / factor ) ].depth ) / 2;
        }
      }

      if( depth != INT_MAX ) {
        render->gbuf[ i + j * render->xres ].pos = pos;
        render->gbuf[ i + j * render->xres ].depth = depth;
        render->gbuf[ i + j * render->xres ].normal = GzCoord(1,0,0);
      }

//      fprintf( fopt, "(%d,%d)\t%f\n", i, j, depth );


    }
  }

  for( int k = 0; k < nLevel; k++ ) free( gbuf[k] );
  free( gbuf );
  

  return GZ_SUCCESS;
}



int GzDrawSample( GzRender	*render, GzCoord *point, GzCoord *normal, GzTextureIndex *uv, float zNear, float zFar )
{
  GzCoord pos = point[0];
  GzCoord nor = normal[0];
  
  pos = ( render->Xcw * pos ).dW();
  nor = ( render->Xncw * nor ).dW();


  if( pos[2] < 0.01 ) return GZ_FAILURE;

  float L = pos.length();
  pos = pos / L;
  pos[2] = pos[2] + 1;
  pos[0] = pos[0] / pos[2];
  pos[1] = pos[1] / pos[2];

  pos[2] = ( L - zNear ) / ( zFar - zNear );
  pos[3]  = 1.0;

  pos = ( render->Xsp * pos ).dW();


  int x = pos[0];
  int y = pos[1];
  float z = pos[2];

  if( z < 0 ) { /* behind the camera */ }
  else if( x >= render->xres || x < 0 || y >= render->yres || y < 0 ) { /* out of the screen */ }
  else {
    int width = 0;
    int minX = x - width;
    int maxX = x + width;
    int minY = y - width;
    int maxY = y + width;
    if( minY < 0 ) minY = 0;
    if( maxY >= render->yres ) maxY = render->yres - 1;
    if( minX < 0 ) minX = 0;
    if( maxX >= render->xres ) maxX = render->xres - 1;

    for( int j = minY; j <= maxY; j++ ) {         
      for( int i = minX; i <= maxX; i++ ) {

        GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);
        if( ( GzDepth ) z <= gbuf->depth && z >= 0 ) {
          gbuf->depth = ( GzDepth ) z;

//          fprintf( stdout, "ONE: (%d,%d,%f)\n", x, y, z );

//          gbuf->pos = ( render->Xws * GzCoord( x, y, z ) ).dW();
          gbuf->pos = GzCoord( x, y, z );
          gbuf->normal = ( ( render->Xnws * nor ).dW() ).normal();
//          fprintf( fopt, "TWO: (%f,%f,%f)(%f,%f,%f)\n", gbuf->pos[0], gbuf->pos[1], gbuf->pos[2], gbuf->normal[0], gbuf->normal[1], gbuf->normal[2] );
          gbuf->Ka = render->Ka;
          gbuf->Kd = render->Kd;
          gbuf->Ks = render->Ks;
        }
      }
    }
  }
	return GZ_SUCCESS;
}



int GzNewGBuffer( GzRender	*render, GzGeoPixel	**buf )
{
  if( render == NULL ) return GZ_FAILURE;

  *buf = ( GzGeoPixel * ) malloc( sizeof(GzGeoPixel) * render->xres * render->yres );
  if( *buf == NULL ) return GZ_FAILURE;
  for( int i = 0; i < render->xres * render->yres; i++ ) {
    (*buf)[i].pos = GzCoord( 0.0, 0.0, -1.0, 1.0 );
    (*buf)[i].normal = GzCoord( 0.0, 0.0, 0.0, 0.0 );
    (*buf)[i].Ka = GzColor( 0.0, 0.0, 0.0 );
    (*buf)[i].Kd = GzColor( 0.0, 0.0, 0.0 );
    (*buf)[i].Ks = GzColor( 0.0, 0.0, 0.0 );
    (*buf)[i].flux = GzColor( 0.0, 0.0, 0.0 );
    (*buf)[i].depth = (float) INT_MAX;
  }
  return GZ_SUCCESS;
}

int GzInitGBuffer( GzRender	*render, GzGeoPixel	*buf )
{
  if( render == NULL ) return GZ_FAILURE;

  for( int i = 0; i < render->xres * render->yres; i++ ) {
    buf[i].pos = GzCoord( 0.0, 0.0, -1.0, 1.0 );
    buf[i].normal = GzCoord( 0.0, 0.0, 0.0, 0.0 );
    buf[i].Ka = GzColor( 0.0, 0.0, 0.0 );
    buf[i].Kd = GzColor( 0.0, 0.0, 0.0 );
    buf[i].Ks = GzColor( 0.0, 0.0, 0.0 );
    buf[i].flux = GzColor( 0.0, 0.0, 0.0 );
    buf[i].depth = (float) INT_MAX;
  }
  return GZ_SUCCESS;
}

int GzDrawRectangle( GzGeoPixel	*buf, int xres, int yres, int x, int y, int width, int height, GzColor color )
{
  if( buf == NULL ) return GZ_FAILURE;
  for( int i = x - width / 2; i <= x + width / 2; i++ ) {
    for( int j = y - height / 2; j <= y + height / 2; j++ ) {
      if( i < 0 || i >= xres || j < 0 || j >= yres ) continue;
      buf[ i + j * xres ].flux = color;
    }
  }
  return GZ_SUCCESS;
}


int GzDrawLine( GzGeoPixel	*buf, int xres, int yres, int x1, int y1, int x2, int y2, GzColor color )
{
//  fprintf( fopt, "GzDrawLine, (%d,%d) - (%d,%d)\n", x1, y1, x2, y2 );
  bool slopegt1 = false;  
  int dx = abs( x1 - x2 );
  int dy = abs( y1 - y2 );

	if( dy > dx ) {
		swap( &x1, &y1 );
		swap( &x2, &y2 );
		swap( &dx, &dy );
		slopegt1 = true;
	}

  if( x1 > x2 ) { swap( &x1, &x2 ); swap( &y1, &y2 ); }

  int incrY = ( y1 > y2 ) ? -1 : 1;
	int d = 2 * dy - dx;
	int incrE = 2 * dy;
	int incrNE = 2 * ( dy - dx );

	while( x1 < x2 ) {
		if( d <= 0 ) d += incrE;
    else { d += incrNE; y1 += incrY; }
		x1 += 1;

		if( slopegt1 ) {
      if( y1 < 0 || y1 >= xres || x1 < 0 || x1 >= yres ) continue;
      buf[ y1 + x1 * xres ].flux = color;
    } else {
      if( x1 < 0 || x1 >= xres || y1 < 0 || y1 >= yres ) continue;
      buf[ x1 + y1 * xres ].flux = color;
    }
	}
	return GZ_SUCCESS;
}


int GzBindGBuffer( GzRender	*render, GzGeoPixel	*buf )
{
  if( buf == NULL ) return GZ_FAILURE;
  if( render == NULL ) return GZ_FAILURE;
  render->gbuf = buf;
  return GZ_SUCCESS;
}



int GzDeferredShading( GzRender	*render, bool showProgress )

{
  if( render == NULL ) return GZ_FAILURE;
  if( render->shader_func == 0 ) return GZ_FAILURE;

  /* need to bind g-buffer first */
  if( render->gbuf == NULL ) return GZ_FAILURE;

  int n_pixel = render->xres * render->yres;
  int cnt = 0;

  for( int j = 0; j < render->yres; j++ ) {         
    

    if( showProgress ) fprintf( stderr, "In progress... %.2f%%\r", 100 * (float)cnt / (float)n_pixel );

    for( int i = 0; i < render->xres; i++ ) {
      if( render->shader_func( render, i, j ) == GZ_FAILURE ) return GZ_FAILURE;
    }
    cnt = cnt + render->xres;
  }
  return GZ_SUCCESS;
}

int GzPositionBounday( GzRender	*render, GzMatrix mat, GzCoord *min, GzCoord *max )
{
  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == 0 ) return GZ_FAILURE;

  *min = GzCoord( 1.0e+038, 1.0e+038, 1.0e+038 );
  *max = GzCoord( -1.0e+038, -1.0e+038, -1.0e+038 );

  GzCoord gbufmin = GzCoord( 1.0e+038, 1.0e+038, 1.0e+038 );
  GzCoord gbufmax = GzCoord( -1.0e+038, -1.0e+038, -1.0e+038 );

  
  GzPrintMatrix( stderr, mat );



  for( int j = 0; j < render->yres; j++ ) {         
    for( int i = 0; i < render->xres; i++ ) {

      GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

      /* world space to "mat" space */
      GzCoord pos = mat * gbuf->pos;

      for( int k = 0; k < 3; k++ ) {
        if( pos[k] > (*max)[k] ) (*max)[k] = pos[k];
        if( pos[k] < (*min)[k] ) (*min)[k] = pos[k];
        if( gbuf->pos[k] > gbufmax[k] ) gbufmax[k] = gbuf->pos[k];
        if( gbuf->pos[k] < gbufmin[k] ) gbufmin[k] = gbuf->pos[k];

      }
    }
  }
  

  fprintf( stderr, "=====image boundary==(worldspace space)====\n");
  fprintf( stderr, "( %f, %f, %f ) - ( %f, %f, %f )\n", gbufmin[0], gbufmin[1], gbufmin[2], gbufmax[0], gbufmax[1], gbufmax[2]);
  fprintf( stderr, "=====image boundary==(light space)====\n");
  fprintf( stderr, "( %f, %f, %f ) - ( %f, %f, %f )\n", (*min)[0], (*min)[1], (*min)[2], (*max)[0], (*max)[1], (*max)[2]);

  return GZ_SUCCESS;
}




int GzRSMSamplingPat( GzRSMSample **pattern, int num, float radius )
{
  *pattern = ( GzRSMSample * ) malloc( sizeof( GzRSMSample ) * num );

  if( radius > 1.0 ) radius = 1.0;
  if( radius < 0.0 ) radius = 0.0;

  const float TWOPI = 2 * 3.141593f ;

  for( int i = 0; i < num; i++ ) {
    float v1 = (float) rand() / RAND_MAX;
    float v2 = (float) rand() / RAND_MAX;
    (*pattern)[i][0] = radius * v1 * sin( TWOPI * v2 );
    (*pattern)[i][1] = radius * v1 * cos( TWOPI * v2 );
    (*pattern)[i][2] = v1 * v1;
  }

  return GZ_SUCCESS;
}

/* NOT part of API - just for general assistance */

short	ctoi(float color)		/* convert float color to GzIntensity short */
{
  return(short)((int)(color * ((1 << 12) - 1)));
}


/* self defined functions */


// Sort ( increament ) verties by y value. If y values are the same, sort by x values.
void GzSortVerties( int numVert, GzCoord * verties, GzCoord * normals, GzTextureIndex *uv )
{
  for( int i = 0; i < numVert; i++ ) {
    for( int j = i + 1; j < numVert; j++ ) {
      if( ( verties[i][1] > verties[j][1] ) || ( verties[i][1] == verties[j][1] && verties[i][0] > verties[j][0] ) ) {
        GzCoordSwap( &(verties[i]), &(verties[j]) );
        if( normals ) GzCoordSwap( &(normals[i]), &(normals[j]) );

        if( uv ) GzTextureIndexSwap( &(uv[i]), &(uv[j]) );

      }
    }
  }  
}

void GzPrintMatrix( FILE *f, GzMatrix matrix )
{
  for( int i = 0; i < 4; i++ ) {
		fprintf( f, "[ %f  %f  %f  %f ]\n", matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3] );
  }
  fprintf( f, "\n" );
}

void GzPrintCoord( FILE *f, GzCoord coord )
{
 	fprintf( f, "[ %f  %f  %f %f]\n", coord[0], coord[1], coord[2], coord[3] );
  fprintf( f, "\n" );
}

void swap(int* a,int* b)
{
	int t=*a;
	*a=*b;
	*b=t;
}

void GzFloatSwap( float* a, float* b )
{
	float t=*a;
	*a=*b;
	*b=t;
}

void GzCoordSwap(GzCoord *a, GzCoord *b)
{
	GzCoord tmp = *a;
  *a = *b;
  *b = tmp;
}


void GzTextureIndexSwap( GzTextureIndex *a, GzTextureIndex *b)
{
	GzTextureIndex tmp = *a;
  *a = *b;
  *b = tmp;
}



// |1  0   0| | Cy  0 Sy| |Cz -Sz 0|   | CyCz        -CySz         Sy  |
// |0 Cx -Sx|*|  0  1  0|*|Sz  Cz 0| = | SxSyCz+CxSz -SxSySz+CxCz -SxCy|
// |0 Sx  Cx| |-Sy  0 Cy| | 0   0 1|   |-CxSyCz+SxSz  CxSySz+SxCz  CxCy|

GzMatrix GzRotXMat(float degree)
{
// Create rotate matrix : rotate along x axis
// Pass back the matrix using mat value
  const float DEG2RAD = 3.141593f / 180;
  float sx, cx, theta;

  theta = degree * DEG2RAD;
  sx = sinf(theta);
  cx = cosf(theta);

  float m[16] = { 1,  0,   0,  0,
                  0, cx, -sx,  0,
                  0, sx,  cx,  0,
                  0,  0,   0,  1 };

	return GzMatrix( m );
}


GzMatrix GzRotYMat(float degree)
{
// Create rotate matrix : rotate along y axis
// Pass back the matrix using mat value
  const float DEG2RAD = 3.141593f / 180;
  float sy, cy, theta;

  theta = degree * DEG2RAD;
  sy = sinf(theta);
  cy = cosf(theta);

  float m[16] = {  cy,  0,  sy,  0,
                    0,  1,   0,  0,
                  -sy,  0,  cy,  0,
                    0,  0,   0,  1 };

	return GzMatrix( m );
}

GzMatrix GzRotZMat(float degree)
{
// Create rotate matrix : rotate along z axis
// Pass back the matrix using mat value
  const float DEG2RAD = 3.141593f / 180;
  float sz, cz, theta;

  theta = degree * DEG2RAD;
  sz = sinf(theta);
  cz = cosf(theta);

  float m[16] = {  cz, -sz,   0,  0,
                   sz,  cz,   0,  0,
                    0,   0,   1,  0,
                    0,   0,   0,  1 };

	return GzMatrix( m );
}


GzMatrix GzTrxMat(GzCoord translate)
{
// Create translation matrix
// Pass back the matrix using mat value
  float m[16] = {  1,  0,  0,  translate[0],
                   0,  1,  0,  translate[1],
                   0,  0,  1,  translate[2],
                   0,  0,  0,             1 };

	return GzMatrix( m );
}


GzMatrix GzScaleMat(GzCoord scale)
{
// Create scaling matrix
// Pass back the matrix using mat value
  float m[16] = {  scale[0],         0,         0,  0,
                   0       ,  scale[1],         0,  0,
                   0       ,         0,  scale[2],  0,
                   0       ,         0,         0,  1 };

	return GzMatrix( m );
}
