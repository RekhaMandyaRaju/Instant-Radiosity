/* Texture functions for cs580 GzLib	*/

#include	"stdio.h"

#include	"gz.h"
#include	"rend.h"


static FILE *fopt = NULL;
const static float DEG2RAD = 3.141593f / 180;

int shader_lighting_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( fopt == NULL ) fopt = fopen( "shader.debug.txt", "w" );


  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  GzCoord L, N, R, E;
  GzColor S = GzColor( 0, 0, 0 );
  GzColor D = GzColor( 0, 0, 0 );

  GzColor Ka = gbuf->Ka;
  GzColor Ks = gbuf->Ks;
  GzColor Kd = gbuf->Kd;

  /* normal is stored in world space. Have to convert to camera space */
  N = gbuf->normal;
  if( N[0] == 0 && N[1] == 0 && N[2] == 0 ) return GZ_SUCCESS;  /* skip pixel that is empty */ 

  GzMatrix mat = render->Xcw.normal();

  N = ( ( mat * N ).dW() ).normal();
  E = GzCoord( 0, 0, -1 );
  E = ( GzCoord(0,0,0) - ( render->Xcw * gbuf->pos ).dW() ).normal();

  float N_L, N_E, R_E;

  /* lighting */
  for( int k = 0; k < render->numlights; k++ ) {

    if( render->lights[k].enable == false ) continue;

    float visibility = 1.0;

    if( render->lights[k].type == GZ_SPOT_LIGHT ) {

      float angle = render->lights[k].angle; 
      GzCoord Vc; /* spotlight: light center vector */

      Vc = ( render->lights[k].direction ).normal();
      L = ( render->lights[k].position - gbuf->pos ).normal();

      float L_V = L || Vc;

      /* check angle */
      if( L_V < cosf( angle * DEG2RAD / 2 ) ) continue;
    
      visibility = pow( L_V, render->lights[k].exponent );

    } else if( render->lights[k].type == GZ_POINT_LIGHT ) {
      GzCoord direction = gbuf->pos  - render->lights[k].position;

      GzCoord tmp = direction.abs();
      if(      tmp[0] > tmp[1] && tmp[0] > tmp[2] ) direction = GzCoord( direction[0], 0, 0 ).normal();
      else if( tmp[1] > tmp[0] && tmp[1] > tmp[2] ) direction = GzCoord( 0, direction[1], 0 ).normal();
      else if( tmp[2] > tmp[0] && tmp[2] > tmp[1] ) direction = GzCoord( 0, 0, direction[2] ).normal();
//      else fprintf( fopt, "ERR(%d,%d) v(%f,%f,%f)\n", i, j, direction[0], direction[1], direction[2] );


      /* check the direction of the cube map */
      if( ( direction || render->lights[k].direction.normal() ) < 0.5 ) continue;

      L = ( render->lights[k].position - gbuf->pos ).normal();

    } else if( render->lights[k].type == GZ_DIRECTIONAL_LIGHT ) {
      L = render->lights[k].direction;
    }
        
    /* light is at world space. Need to convert to camera space */
    L = ( ( mat * L ).dW() ).normal();

    N_L = N || L;
    N_E = N || E;



    /* very close to zero is zero */
    if( N_L * N_E >= -1e-5 ) {

      if( N_L < 0 && N_E < 0 ) {
        N = N * -1;
        N_L *= -1;
      }

      R = ( N * ( 2 * N_L ) - L ).normal();
      R_E = pow( ( R || E ), render->spec );

      D = D + render->lights[k].color * N_L * visibility;
      S = S + render->lights[k].color * R_E * visibility;
    }

 //   fprintf( fopt, "(%d,%d) L(%f,%f,%f) N(%f,%f,%f)\tN_L = %f, N_E =%f  D(%f,%f,%f) S(%f,%f,%f)\n", i, j, L[0], L[1], L[2],  N[0], N[1], N[2], N_L, N_E, D[0], D[1], D[2], S[0], S[1], S[2] );

  }

  gbuf->flux = ( Ks * S + Kd * D + Ka * render->ambientlight.color ).cutoff();

  return GZ_SUCCESS;
}

int shader_shadow_func( GzPointer	data, int i, int j )
{
  static GzCoord poissonDisk[4];
  poissonDisk[0] = GzCoord( -0.94201624, -0.39906216, 0 );
  poissonDisk[1] = GzCoord(  0.94558609, -0.76890725, 0 );
  poissonDisk[2] = GzCoord( -0.09418411, -0.92938870, 0 );
  poissonDisk[3] = GzCoord(  0.34495938,  0.29387760, 0 );

  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;
  if( render->sharder_data == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);
  
  GzMatrix *lightXsw = ( GzMatrix *) render->sharder_data;

  GzCoord L, N, R, E;
  GzColor S = GzColor( 0, 0, 0 );
  GzColor D = GzColor( 0, 0, 0 );

  GzColor Ka = gbuf->Ka;
  GzColor Ks = gbuf->Ks;
  GzColor Kd = gbuf->Kd;

  /* normal is stored in world space. Have to convert to camera space */
  N = gbuf->normal;
  if( N[0] == 0 && N[1] == 0 && N[2] == 0 ) return GZ_SUCCESS;  /* skip pixel that is empty */ 

  GzMatrix mat = render->Xcw.normal();

  N = ( ( mat * N ).dW() ).normal();
  E = GzCoord( 0, 0, -1 );
  E = ( GzCoord(0,0,0) - ( render->Xcw * gbuf->pos ).dW() ).normal();

//  GzCoord G = ( mat * gbuf->pos ).dW();
//  E = ( E - G ).normal();


  float N_L, N_E, R_E;

  /* lighting */
  for( int k = 0; k < render->numlights; k++ ) {
    GzGeoPixel	*shadow = render->lights[k].ginfo.gbuf;
    int shadow_xres = render->lights[k].ginfo.xres;
    int shadow_yres = render->lights[k].ginfo.yres;

    if( render->lights[k].enable == false ) continue;

    float visibility = 1.0;

    if( render->lights[k].type == GZ_SPOT_LIGHT ) {

      float angle = render->lights[k].angle; 
      GzCoord Vc; /* spotlight: light center vector */

      Vc = ( render->lights[k].direction ).normal();
      L = ( render->lights[k].position - gbuf->pos ).normal();

      float L_V = L || Vc;

      /* check angle */
      if( L_V < cosf( angle * DEG2RAD / 2 ) ) continue;
    
      visibility = pow( L_V, render->lights[k].exponent );

    } else if( render->lights[k].type == GZ_POINT_LIGHT ) {

      GzCoord direction = gbuf->pos  - render->lights[k].position;
      GzCoord tmp = direction.abs();
      if(      tmp[0] > tmp[1] && tmp[0] > tmp[2] ) direction = GzCoord( direction[0], 0, 0 ).normal();
      else if( tmp[1] > tmp[0] && tmp[1] > tmp[2] ) direction = GzCoord( 0, direction[1], 0 ).normal();
      else if( tmp[2] > tmp[0] && tmp[2] > tmp[1] ) direction = GzCoord( 0, 0, direction[2] ).normal();

 //     else fprintf( fopt, "ERR(%d,%d) v(%f,%f,%f)\n", i, j, direction[0], direction[1], direction[2] );


      /* check the direction of the cube map */
      if( ( direction || render->lights[k].direction.normal() ) < 0.5 ) continue;

      L = ( render->lights[k].position - gbuf->pos ).normal();

    } else if( render->lights[k].type == GZ_DIRECTIONAL_LIGHT ) {
      L = render->lights[k].direction;
    }
        
    /* light is at world space. Need to convert to camera space */
    L = ( ( mat * L ).dW() ).normal();

    N_L = N || L;
    N_E = N || E;

    /* very close to zero is zero */
    if( N_L * N_E >= -1e-5 ) {

      if( N_L < 0 && N_E < 0 ) {
        N = N * -1;
        N_L *= -1;
      }

      /* shadow check */
      if( shadow ) {
          
        GzCoord cameraPos = ( lightXsw[k] * gbuf->pos ).dW();


        float bias = 0.015 * ( INT_MAX );         

        for( int i = 0; i < 4; i++ ){
          GzCoord lightPos;
          int x = round( cameraPos[0] + poissonDisk[i][0] );
          int y = round( cameraPos[1] + poissonDisk[i][1] );
          if( x >= shadow_xres ) x = shadow_xres - 1;
          if( y >= shadow_yres ) y = shadow_yres - 1;
          if( x < 0 ) x = 0;
          if( y < 0 ) y = 0;
          
          lightPos = ( lightXsw[k] * shadow[ x + y * shadow_xres ].pos ).dW();
          if ( lightPos[2]  <  cameraPos[2] - bias ){
            visibility -= 0.25;
          }
        }
      }

      if( visibility < 0.0 ) visibility = 0.0;
          
      R = ( N * ( 2 * N_L ) - L ).normal();
      R_E = pow( ( R || E ), render->spec );

      D = D + render->lights[k].color * N_L * visibility;
      S = S + render->lights[k].color * R_E * visibility;
    }
  }

//  gbuf->flux = ( Ks * S + Kd * D + Ka * render->ambientlight.color ).cutoff();
  gbuf->flux = ( gbuf->flux + Ks * S + Kd * D + Ka * render->ambientlight.color ).cutoff();

  return GZ_SUCCESS;
}


int shader_ISM_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  float *zValues = ( float *) render->sharder_data;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);
  
  GzCoord L, N, R, E;
  GzColor S = GzColor( 0, 0, 0 );
  GzColor D = GzColor( 0, 0, 0 );

  GzColor Ks = gbuf->Ks;
  GzColor Kd = gbuf->Kd;

  /* normal is stored in world space. Have to convert to camera space */
  N = gbuf->normal;
  if( N[0] == 0 && N[1] == 0 && N[2] == 0 ) return GZ_SUCCESS;  /* skip pixel that is empty */ 

  GzMatrix mat = render->Xcw.normal();

  N = ( ( mat * N ).dW() ).normal();
  E = GzCoord( 0, 0, -1 );
  E = ( GzCoord(0,0,0) - ( render->Xcw * gbuf->pos ).dW() ).normal();
  
  float N_L, N_E, R_E;

  /* lighting */
  for( int k = 0; k < (int) render->microView->size(); k++ ) {

    int sampleIdx = render->microView->at(k);
    GzColor flux = render->samples->at( sampleIdx ).sample.flux;
    float visibility = 1.0;
    
    L = ( render->samples->at( sampleIdx ).sample.pos - gbuf->pos ).normal();

    /* light is at world space. Need to convert to camera space */
    L = ( ( mat * L ).dW() ).normal();

    N_L = N || L;
    N_E = N || E;

    /* very close to zero is zero */
    if( N_L * N_E >= -1e-5 ) {

      if( N_L < 0 && N_E < 0 ) {
        N = N * -1;
        N_L *= -1;
      }
      
      /* shadow check */
      const float bias = 0.005 * ( INT_MAX );        

      int shadow_xres = render->samples->at( sampleIdx ).ISM.xres;
      int shadow_yres = render->samples->at( sampleIdx ).ISM.yres;
      GzCoord cameraPos = ( render->samples->at( sampleIdx ).ISM.Xcw * gbuf->pos ).dW();
      
      float len = cameraPos.length();
      cameraPos = cameraPos / len;
      cameraPos[2] = cameraPos[2] + 1;
      cameraPos[0] = cameraPos[0] / cameraPos[2];
      cameraPos[1] = cameraPos[1] / cameraPos[2];
      cameraPos[2] = ( len - zValues[0] ) / ( zValues[1] - zValues[0] );
      cameraPos[3]  = 1.0;

      cameraPos = ( render->samples->at( sampleIdx ).ISM.Xsp * cameraPos ).dW();

      int x = round( cameraPos[0] );
      int y = round( cameraPos[1] );
      if( x >= shadow_xres ) x = shadow_xres - 1;
      if( y >= shadow_yres ) y = shadow_yres - 1;
      if( x < 0 ) x = 0;
      if( y < 0 ) y = 0;

      GzCoord lightPos = render->samples->at( sampleIdx ).ISM.gbuf[ x + y * shadow_xres ].pos;
      if ( lightPos[2]  <  cameraPos[2] - 5 * bias ){
        visibility = 0.2;
      }

      if( visibility < 0.0 ) visibility = 0.0;
          
      R = ( N * ( 2 * N_L ) - L ).normal();
      R_E = pow( ( R || E ), render->spec );

      D = D + flux * N_L * visibility;
      S = S + flux * R_E * visibility;
    }
  }

//  gbuf->flux = ( Ks * S + Kd * D + Ka * render->ambientlight.color ).cutoff();
  gbuf->flux = ( gbuf->flux + Ks * S + Kd * D ).cutoff();

  return GZ_SUCCESS;
}


int shader_RSM_func( GzPointer	data, int i, int j )
{
  static GzCoord poissonDisk[4];
  poissonDisk[0] = GzCoord( -0.94201624, -0.39906216, 0 );
  poissonDisk[1] = GzCoord(  0.94558609, -0.76890725, 0 );
  poissonDisk[2] = GzCoord( -0.09418411, -0.92938870, 0 );
  poissonDisk[3] = GzCoord(  0.34495938,  0.29387760, 0 );

  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;
  if( render->sharder_data == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  GzColor Radiosity = GzColor( 0, 0, 0 );


  /* normal is stored in world space. Have to convert to camera space */
  GzCoord N = gbuf->normal;
  if( N[0] == 0 && N[1] == 0 && N[2] == 0 ) return GZ_SUCCESS;  /* skip pixel that is empty */ 


  /* radiosity */

  static GzRSMSample *pattern = NULL;
  static int pattern_n = 1000;
  if( pattern == NULL ) GzRSMSamplingPat( &pattern, pattern_n, 0.5 );

//  if( i==158 && j==59 ) fprintf( fopt, "(%d,%d)\tP(%f,%f,%f)\n", i, j, gbuf->pos[0], gbuf->pos[1], gbuf->pos[2]);
//  if( i==158 && j==59 ) fprintf( fopt, "(%d,%d)\tN(%f,%f,%f)\n", i, j, gbuf->normal[0], gbuf->normal[1], gbuf->normal[2]);


  int sampleNum = 0;
  float energyNorm = 0;
  for( int k = 0; k < render->numlights && 1; k++ ) {
    GzGeoPixel *light = render->lights[k].ginfo.gbuf;
    int light_xres = render->lights[k].ginfo.xres;
    int light_yres = render->lights[k].ginfo.yres;
    if( render->lights[k].enable == false ) continue;

    if( render->lights[k].type == GZ_POINT_LIGHT ) {
      GzCoord direction = gbuf->pos  - render->lights[k].position;
      GzCoord tmp = direction.abs();
      if(      tmp[0] > tmp[1] && tmp[0] > tmp[2] ) direction = GzCoord( direction[0], 0, 0 ).normal();
      else if( tmp[1] > tmp[0] && tmp[1] > tmp[2] ) direction = GzCoord( 0, direction[1], 0 ).normal();
      else if( tmp[2] > tmp[0] && tmp[2] > tmp[1] ) direction = GzCoord( 0, 0, direction[2] ).normal();
      else fprintf( fopt, "Radoisty skip light%d(%d,%d) v(%f,%f,%f)\n", k, i, j, direction[0], direction[1], direction[2] );
      if( ( direction || render->lights[k].direction.normal() ) < 0.5 ) continue;
    }

    GzCoord cameraPos = gbuf->pos;
    GzCoord cameraNormal = gbuf->normal;

    for( int p = 0; p < pattern_n; p++ ) {

      int x = round( pattern[p][0] * light_xres );
      int y = round( pattern[p][1] * light_yres );

      if( x < 0 || x >= light_xres || y < 0 || y >= light_yres ) continue;

      if( x >= light_xres ) x = light_xres - 1;
      if( y >= light_yres ) y = light_yres - 1;
      if( x < 0 ) x = 0;
      if( y < 0 ) y = 0;

      GzCoord lightPos = light[ x + y * render->xres ].pos;
      GzCoord lightNormal = light[ x + y * render->xres ].normal;
      GzColor lightFlux = light[ x + y * render->xres ].flux;

//      if( lightFlux[0] < 0.3 && lightFlux[1] < 0.3 && lightFlux[2] < 0.3  ) continue;

          
      GzCoord Vx_xp = cameraPos - lightPos;
      GzCoord Vxp_x = lightPos - cameraPos;
      float innerProduct1 = lightNormal || Vx_xp;
      float innerProduct2 = cameraNormal || Vxp_x;
      float max1 = ( innerProduct1 > 0 ) ? innerProduct1 : 0;
      float max2 = ( innerProduct2 > 0 ) ? innerProduct2 : 0;
      float length = sqrtf( Vx_xp[0] * Vx_xp[0] + Vx_xp[1] * Vx_xp[1] + Vx_xp[2] * Vx_xp[2] );

      float coef = ( max1 * max2 ) / pow( length, 2 );

//      if( i==158 && j==59 ) fprintf( stderr, "(%f,%f)\tR(%f,%f,%f)\n", coef, pattern[p][2], lightFlux[0], lightFlux[1], lightFlux[2]);



      if( coef > 0 && coef < 1.0 ) {
        sampleNum += 1;
        energyNorm += pattern[p][2];
        Radiosity = Radiosity + lightFlux * coef * pattern[p][2];
      }
    }   
  }

  if( energyNorm > 0 ) Radiosity = Radiosity / energyNorm;
//  if( sampleNum < 0 ) Radiosity = GzColor( 0, 0, 0 );


//  fprintf( stdout, "(%d,%d)\t(%f,%f,%f)\n", i, j, Radiosity[0], Radiosity[1], Radiosity[2]);

//  if( Radiosity[0] > 0 || Radiosity[1] > 0 || Radiosity[2] > 0  ) Radiosity = GzColor( 0.2,0.2,0.2 );
  gbuf->flux = ( gbuf->flux + gbuf->Kd * Radiosity ).cutoff();
  

  return GZ_SUCCESS;
};

int shader_depthMap_func( GzPointer	data, int i, int j )
{

  if( fopt == NULL ) fopt = fopen( "shader.debug.txt", "w" );
  GzRender * render = ( GzRender * ) data;
//  GzGeoPixel * sample = ( GzGeoPixel *) render->sharder_data;



  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  /* empty pixel, set to black */
  if( gbuf->normal[0] == 0 && gbuf->normal[1] == 0 && gbuf->normal[2] == 0 ) {
    gbuf->flux = GzColor( 0.0, 0.0, 0.0 );
    return GZ_SUCCESS; 
  }
  GzCoord depthP = ( render->Xpc * render->Xcw * gbuf->pos ).dW();

  gbuf->flux = GzColor( depthP[2], depthP[2], depthP[2] );

  GzDepth depth = gbuf->depth;
//  GzColor color = sample[0].flux * ( depth / INT_MAX );
  GzColor color = GzColor(1,1,1) * ( depth / INT_MAX );
//  fprintf( stdout, "(%d,%d)\t%f\n", i, j, ( depth / INT_MAX ) );
//  fprintf( stdout, "(%d,%d)\t%f,%f,%f\n", i, j, sample[0].flux[0], sample[0].flux[1], sample[0].flux[2] );
  

  gbuf->flux = color;
//  gbuf->flux = GzColor( depth / INT_MAX, depth / INT_MAX, depth / INT_MAX );

  //  gbuf->flux = GzColor( 1,1,1 );

  return GZ_SUCCESS;
}


int shader_materialMap_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  /* empty pixel, set to black */
  if( gbuf->normal[0] == 0 && gbuf->normal[1] == 0 && gbuf->normal[2] == 0 ) {
    gbuf->flux = GzColor( 0, 0, 0 );
    return GZ_SUCCESS; 
  }

  gbuf->flux = gbuf->Ka;
  return GZ_SUCCESS;
}

int shader_normalMap_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  /* empty pixel, set to black */
  if( gbuf->normal[0] == 0 && gbuf->normal[1] == 0 && gbuf->normal[2] == 0 ) {
    gbuf->flux = GzColor( 0, 0, 0 );
    return GZ_SUCCESS; 
  }

  GzColor color = GzColor( gbuf->normal[0], gbuf->normal[1], gbuf->normal[2] );
  gbuf->flux = ( color + 1.0 ) / 2.0;
  return GZ_SUCCESS;
}

int shader_positionMap_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);

  /* empty pixel, set to black */
  if( gbuf->normal[0] == 0 && gbuf->normal[1] == 0 && gbuf->normal[2] == 0 ) {
    gbuf->flux = GzColor( 0, 0, 0 );
    return GZ_SUCCESS; 
  }

  GzColor color = GzColor( gbuf->pos[0], gbuf->pos[1], gbuf->pos[2] );

  gbuf->flux = ( color + 2.0 ) / 4.0;

  return GZ_SUCCESS;
}

int shader_cubeMap_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;
  if( render->sharder_data == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);
  int *startIdx = ( int *) render->sharder_data;

  int xLen = render->xres / 4;
  int yLen = render->yres / 4;

  int index = -1;
  int x, y;

  if( j < yLen ) { 
    if( i < xLen ) {}
    else if( i < 2 * xLen) { index = 4; x = 4 * ( i - xLen ); y = 4 * j; }

  } else if( j < 2 * yLen ) { 
    if( i < xLen )         { index = 1; x = 4 * ( i - 0 * xLen ); y = 4 * ( j - yLen ); }
    else if( i < 2 * xLen) { index = 3; x = 4 * ( i - 1 * xLen ); y = 4 * ( j - yLen ); }
    else if( i < 3 * xLen) { index = 0; x = 4 * ( i - 2 * xLen ); y = 4 * ( j - yLen ); }
    else if( i < 4 * xLen) { index = 2; x = 4 * ( i - 3 * xLen ); y = 4 * ( j - yLen ); }

  } else if( j < 3 * yLen ) { 
    if( i < xLen ) {}
    else if( i < 2 * xLen) { index = 5; x = 4 * ( i - xLen ); y = 4 * ( j - 2 * yLen ); }
  }

  if( index != -1 ) gbuf->flux = render->lights[ *startIdx + index ].ginfo.gbuf[ x + y * render->xres ].flux;
  else gbuf->flux = GzColor( 0, 0, 0 );

  return GZ_SUCCESS;
}


int shader_ISMMap_func( GzPointer	data, int i, int j )
{
  GzRender * render = ( GzRender * ) data;

  if( render == NULL ) return GZ_FAILURE;
  if( render->gbuf == NULL ) return GZ_FAILURE;
  if( render->sharder_data == NULL ) return GZ_FAILURE;

  GzGeoPixel *gbuf = &(render->gbuf[ i + j * render->xres ]);
  
  int sampleIdx = render->microView->at(0);
  int width = render->samples->at(sampleIdx).ISM.xres;
  int height = render->samples->at(sampleIdx).ISM.yres;
  
  int nx = render->xres / width;
  int ny = render->yres / height;
  
  int ix = i / width;
  int iy = j / height;
  
  sampleIdx = render->microView->at( ix + iy * nx );
    
  int px = i - width * ix;
  int py = j - height * iy;
  
//  printf( "(%d,%d)\n", i, j,  );
  if( render->samples->at( sampleIdx ).ISM.gbuf ) gbuf->flux = render->samples->at(sampleIdx).ISM.gbuf[ px + width * py ].flux;
}

