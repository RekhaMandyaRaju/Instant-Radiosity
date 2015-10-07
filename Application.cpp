// Application6.cpp: implementation of the Application6 class.
//
//////////////////////////////////////////////////////////////////////

/*
 * application test code for homework assignment #4
*/

#define INFILE  "CornellBox.asc"
#define OUTFILE "output.ppm"

#include "Application.h"
#include "gz.h"
#include "disp.h"
#include "rend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


extern int tex_fun( GzPointer	data, float u, float v, GzColor * color ); /* image texture function */
extern int ptex_fun( GzPointer	data, float u, float v, GzColor *color); /* procedural texture function */


extern int shader_lighting_func( GzPointer	data, int i, int j );
extern int shader_shadow_func( GzPointer	data, int i, int j );
extern int shader_depthMap_func( GzPointer	data, int i, int j );
extern int shader_normalMap_func( GzPointer	data, int i, int j );
extern int shader_positionMap_func( GzPointer	data, int i, int j );

extern int shader_antialising_func( GzPointer	data, int i, int j );
extern int shader_cubeMap_func( GzPointer	data, int i, int j );


extern int shader_ISM_func( GzPointer	data, int i, int j );
extern int shader_RSM_func( GzPointer	data, int i, int j );

extern int shader_ISMMap_func( GzPointer	data, int i, int j );


void shade(GzCoord norm, GzCoord color);

float halton_seq( int index, int base)
{
  float result = 0;
  float f = 1.0 / (float) base;
  int i = index;
  while( i > 0 ) { 
    result = result + f * (i % base);
    i = floor( (float) i / (float) base);
    f = f / base;
  }
  return result;
}

Application::Application()
{
	m_pDisplay = NULL;		// the display
	m_pRender = NULL;		// the renderer
	m_pFrameBuffer = NULL;

	inputfile = strdup( INFILE );
	outputfile = strdup( OUTFILE );
}

Application::Application( const char *input, const char *output )
{
	m_pDisplay = NULL;		// the display
	m_pRender = NULL;		// the renderer
	m_pFrameBuffer = NULL;


	if( input ) inputfile = strdup( input );
	else inputfile = strdup( INFILE );
	if( output ) outputfile = strdup( output );
	else outputfile = strdup( OUTFILE );

}

Application::~Application()
{
	if(m_pFrameBuffer != NULL) free(m_pFrameBuffer);
	if(m_pRender != NULL)	GzFreeRender(m_pRender); 
	  
	free( inputfile );
	free( outputfile );
	  
}



int Application::Initialize( )
{
	GzToken		nameListShader[9]; 	    /* shader attribute names */
	GzPointer valueListShader[9];		  /* shader attribute pointers */
	GzToken   nameListLights[10];		  /* light info */
	GzPointer valueListLights[10];
	int			  interpStyle;
	float		  specpower;
	int		    status; 
 
	status = 0; 

	/* 
	 * initialize the display and the renderer 
	 */ 
 	m_nWidth = 1024+512;		  // frame buffer and display width
	m_nHeight = 512;    // frame buffer and display height
	status |= GzNewFrameBuffer(&m_pFrameBuffer, m_nWidth, m_nHeight);

  m_rWidth = 512;			// width of renderer
  m_rHeight = 512;		// height of renderer
	status |= GzNewRender(&m_pRender, m_rWidth, m_rHeight ); 


  /* Camera */
  camera.position[X] = 0.0;      
  camera.position[Y] = 1.0;
  camera.position[Z] = 2.5;
  camera.lookat[X] = 0.0;
  camera.lookat[Y] = 0.5;
  camera.lookat[Z] = -0.8;
  camera.worldup[X] = 0.0;
  camera.worldup[Y] = 1.0;
  camera.worldup[Z] = 0.0;
  camera.FOV = 45;


	/* Light */
	GzLight	light1, light2, light3;
	
	light1.type = GZ_SPOT_LIGHT;

	light1.color = GzColor( 1.0,1.0,1.0 );
	light1.direction = GzCoord( 1.0, -0.7, 0.0 );
	light1.position = GzCoord( 0.0, 1.0, 0 );
	light1.angle = 50;
	light1.exponent = 1;
	
	GzLight	ambientlight;
	ambientlight.type = GZ_AMBIENT_LIGHT;
	ambientlight.color = GzColor( 0.1, 0.1, 0.1);
	
	
	/* Material property */
	GzColor specularCoefficient = GzColor( 0.0, 0.0, 0.0 );

	GzColor ambientCoefficient  = GzColor( 0.4, 0.4, 0.4 );
	GzColor diffuseCoefficient  = GzColor( 0.5, 0.5, 0.5 );

/* 
  renderer is ready for frame --- define lights and shader at start of frame 
*/

/*
  * Tokens associated with light parameters
  */
  
  nameListLights[0] = GZ_LIGHT;
  valueListLights[0] = (GzPointer)&light1;
  nameListLights[1] =  GZ_LIGHT;
  valueListLights[1] = (GzPointer)&light2;
  nameListLights[2] = GZ_LIGHT;
  valueListLights[2] = (GzPointer)&light3;

  status |= GzPutAttribute(m_pRender, 1, nameListLights, valueListLights);


  nameListLights[0] = GZ_AMBIENT_LIGHT;
  valueListLights[0] = (GzPointer)&ambientlight;
  status |= GzPutAttribute(m_pRender, 1, nameListLights, valueListLights);

  /*
    * Tokens associated with shading 
    */
  nameListShader[0]  = GZ_DIFFUSE_COEFFICIENT;
  valueListShader[0] = (GzPointer)&diffuseCoefficient;

	/* 
	* Select either GZ_COLOR or GZ_NORMALS as interpolation mode  
	*/
  nameListShader[1]  = GZ_INTERPOLATE;
  interpStyle = GZ_NORMALS;         /* Phong shading */
  valueListShader[1] = (GzPointer)&interpStyle;

  nameListShader[2]  = GZ_AMBIENT_COEFFICIENT;
  valueListShader[2] = (GzPointer)&ambientCoefficient;
  nameListShader[3]  = GZ_SPECULAR_COEFFICIENT;
  valueListShader[3] = (GzPointer)&specularCoefficient;
  nameListShader[4]  = GZ_DISTRIBUTION_COEFFICIENT;
  specpower = 32;
  valueListShader[4] = (GzPointer)&specpower;

  nameListShader[5]  = GZ_TEXTURE_MAP;
#if 1   /* set up null texture function or valid pointer */
  valueListShader[5] = (GzPointer)0;
#else
  valueListShader[5] = (GzPointer)(tex_fun);	/* or use ptex_fun */
#endif


  float sampleThreshold = 0.0001;
  nameListShader[6]  = GZ_SAMPLE_COEFFICIENT;
  valueListShader[6] = (GzPointer)&sampleThreshold;

  status |= GzPutAttribute(m_pRender, 7, nameListShader, valueListShader);



	if (status) exit(GZ_FAILURE); 

	if (status) 
		return(GZ_FAILURE); 
	else 
		return(GZ_SUCCESS); 
}

int Application::Render(  ) 
{
  static FILE *fopt = stdout;
  if( fopt == NULL ) fopt = fopen( "render.txt", "w" );

  clock_t start_s, stop_s;

  int status = 0;
	FILE *outfile;
	if( (outfile  = fopen( outputfile , "wb" )) == NULL )
	{
    fprintf( stderr, "The output file was not opened\n" );
    return GZ_FAILURE;
	}

  GzToken		nameListShader[10]; 	    /* shader attribute names */
	GzPointer valueListShader[10];		  /* shader attribute pointers */

  /*
    Rendering steps,

      1. step up VP metrics
           a. GzBindMatrices, or
           b. GzSetProjectMatrix, GzSetViewMatrix and GzSetDepth

      2. bind a geo-buffer
           a. GzNewGBuffer
           b. GzBindGBuffer

      3. start put triangles, the render results are stored in g-buffer
           a. GzPutTriangle

      4. setup deferred shading function
           a. GzPutAttribute ( attr: GZ_SHADER_FUNC )
           b. GzSetShaderData ( extra data need to pass to the shader )

      5. start shading
           a. GzDeferredShading

      6. store MVP metrics, max depth and g-buffer
           a. GzSaveCameraMatrices, or
           b. GzSaveLightMatrices

      7. show the image
           a. GzFlushGbuf2File, or
           b. GzFlushGbuf2FrameBuffer
  */


  GzGeoPixel	*gbuf; 

  GzMatrix projectionM;
  GzMatrix viewM;
  GzMatrix iMatrix;
  iMatrix.identity();

  
  float zNear = 0.1;
  float zFar  = 4.0;
  
  bool enable_ISM = true;
  

  /* render at camera view */
  GzSetProjectMatrix( m_pRender, GzPersProjection( camera.FOV, 1, zNear, zFar ) );

  GzSetViewMatrix( m_pRender, GzMatrixLookAt( camera.position, camera.lookat, camera.worldup ) );

  start_s = clock();

  GzSetSample( m_pRender, true );
	status |= GzNewGBuffer( m_pRender, &gbuf );
	status |= GzBindGBuffer( m_pRender, gbuf );
  status |= doRender();
  status |= GzSaveGInfo( m_pRender, &(m_pRender->camera.ginfo) );
  GzSetSample( m_pRender, false );

  stop_s = clock();

  
  fprintf( fopt, "World boundary: (%.3f,%.3f,%.3f) - (%.3f,%.3f,%.3f)\n", m_pRender->sceneSize[0][0], m_pRender->sceneSize[0][1], m_pRender->sceneSize[0][2],
                                                                          m_pRender->sceneSize[1][0], m_pRender->sceneSize[1][1], m_pRender->sceneSize[1][2] );  

  fprintf( fopt, "Samples Created: %lu\n", m_pRender->samples->size() );
  fprintf( fopt, "Camera View Render: %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);

    
  start_s = clock();

  /* render at each light direction */
  for( int i = 0; i < m_pRender->numlights && 1; i++ ) {

    

    GzCoord direction = m_pRender->lights[i].direction;
    GzCoord position = m_pRender->lights[i].position;
    float angle = m_pRender->lights[i].angle;
    GzMatrix lightViewM;
    GzMatrix lightProjM;
    GzCoord zero = GzCoord( 0, 0, 0 );
    GzCoord up  = GzCoord( 0, 1, 0 );

    switch( m_pRender->lights[i].type ) {
      case GZ_POINT_LIGHT:
      {
        GzCoord lookat;
        if( direction ==  GzCoord( 0, 0, 1 ) ) up  = GzCoord( 0, 1, 0 );
        else if( direction ==  GzCoord( 0, 0, -1 ) ) up  = GzCoord( 0, -1, 0 );
        else up  = GzCoord( 0, 0, 1 );

        GzSetViewMatrix( m_pRender, GzMatrixLookAt( position, position + direction, up ) );

        GzSetProjectMatrix( m_pRender, GzPersProjection( 90, 1, zNear, zFar ) );

        break;

      }
      case GZ_SPOT_LIGHT:
      {
        GzSetViewMatrix( m_pRender, GzMatrixLookAt( position, position - direction, GzCoord( 0, 1, 0 ) ) );

        GzSetProjectMatrix( m_pRender, GzPersProjection( angle, 1, zNear, zFar )  );

        break;

      }
      case GZ_DIRECTIONAL_LIGHT:
      {
        GzSetViewMatrix( m_pRender, GzMatrixLookAt( direction, zero, up ) );

        /* have to bind the gbuffer of the camera view to get the image boundary */
        /* set light view with an addition margin */
        GzCoord lightViewMin, lightViewMax;         
        GzBindGBuffer( m_pRender, m_pRender->camera.ginfo.gbuf ); 

        GzPositionBounday( m_pRender, GzMatrixLookAt( direction, zero, up ), &lightViewMin, &lightViewMax );

        float offset = 0.05;
        GzCoord lightViewRange = lightViewMax - lightViewMin;
        lightViewMin = lightViewMin - lightViewRange * offset;
        lightViewMax = lightViewMax + lightViewRange * offset;

        GzSetProjectMatrix( m_pRender, GzOrthProjection( lightViewMin, lightViewMax ) );
        break;
      }
    }

    nameListShader[0]  = GZ_SHADER_FUNC;
    valueListShader[0] = (GzPointer)( shader_lighting_func );

	  status |= GzNewGBuffer( m_pRender, &gbuf );  
	  status |= GzBindGBuffer( m_pRender, gbuf );
    status |= GzDisableLight( m_pRender );
    status |= GzEnableLight( m_pRender, i );
    status |= doRender();
    status |= GzSaveGInfo( m_pRender, &(m_pRender->lights[i].ginfo) );

    status |= GzPutAttribute( m_pRender, 1, nameListShader, valueListShader );
    status |= GzDeferredShading( m_pRender );
  }

  stop_s = clock();
  fprintf( fopt, "Lights View Render: %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);

  
  // add extra samples from light views. these samples will become VPLs
  int nVPLs = 0;
  if( enable_ISM ){
    int nVPLsPerLight = 200;
    int seqIdx = 0;
    for( int i = 0; i < m_pRender->numlights; i++ ) {
      for( int j = 0; j < nVPLsPerLight; j++ ) {
        int x = halton_seq( seqIdx, 2 ) * m_pRender->lights[i].ginfo.xres;
        int y = halton_seq( seqIdx, 3 ) * m_pRender->lights[i].ginfo.yres;
        GzISMSample sample;
        sample.ISM.gbuf = NULL;
        sample.sample = m_pRender->lights[i].ginfo.gbuf[ x + y * m_pRender->lights[i].ginfo.xres ];
        sample.sample.flux = GzColor(0,0,0);
        m_pRender->samples->push_back( sample ); 
        seqIdx += 1;
        nVPLs += 1;
      }
    }
    fprintf( fopt, "Additional Samples Created: %lu\n", m_pRender->samples->size() );
  }
  
  
  // illuminate samples, convert the samples into a gbuf and call deferred shading
  if( enable_ISM ){
    start_s = clock();
    int nSamples = m_pRender->samples->size();
    int size = pow( 2, ( ( int ) ceil ( log2 ( sqrt( nSamples ) ) ) ) );
    GzMatrix lightXsw[MAX_LIGHTS];
    for( int i = 0; i < m_pRender->numlights; i++ ) {
      lightXsw[i] = m_pRender->lights[i].ginfo.Xsp * m_pRender->lights[i].ginfo.Xpc * m_pRender->lights[i].ginfo.Xcw;
    }
    status |= GzSetScreenSize( m_pRender, size, size );
    status |= GzSetProjectMatrix( m_pRender, GzPersProjection( camera.FOV, 1, zNear, zFar ) );
    status |= GzSetViewMatrix( m_pRender, GzMatrixLookAt( camera.position, camera.lookat, camera.worldup ) );
    status |= GzNewGBuffer( m_pRender, &gbuf );  
    status |= GzBindGBuffer( m_pRender, gbuf );
    for( int i = 0; i < nSamples; i++ ) {
      gbuf[i] = m_pRender->samples->at(i).sample;
    }
    nameListShader[0]  = GZ_SHADER_FUNC;
    valueListShader[0] = (GzPointer)( shader_shadow_func );
    nameListShader[1]  = GZ_DATA;
    valueListShader[1] = (GzPointer)( lightXsw );
    status |= GzEnableLight( m_pRender );
    status |= GzPutAttribute( m_pRender, 2, nameListShader, valueListShader );
    status |= GzDeferredShading( m_pRender );
    stop_s = clock();

    for( int i = 0; i < nSamples; i++ ) {
      m_pRender->samples->at(i).sample = gbuf[i];
    }
    free( gbuf );
    fprintf( fopt, "Illuminate samples: %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);
  }
  
  // select samples to be micro view
  float threshold = 0.3;
  for( int i = m_pRender->samples->size() - 1; i >= ( int ) m_pRender->samples->size() - nVPLs && enable_ISM; i=i-1 ) {
    if( m_pRender->samples->at(i).sample.flux[0] < threshold && m_pRender->samples->at(i).sample.flux[1] < threshold && m_pRender->samples->at(i).sample.flux[1] < threshold  ) continue;
    m_pRender->microView->push_back( i );
    m_pRender->numMicroView += 1;
  }
  for( int i = 0; i < (int) m_pRender->microView->size(); i++ ) {
    int sampleIdx = m_pRender->microView->at(i);
    m_pRender->samples->at( sampleIdx ).sample.flux = m_pRender->samples->at( sampleIdx ).sample.flux / ( m_pRender->numMicroView );
  }
  fprintf( fopt, "MicroView Selected: %d\n", m_pRender->numMicroView );

  start_s = clock();
  for( int i = 0; i < (int) m_pRender->microView->size() && enable_ISM; i++ ) {
    int sampleIdx = m_pRender->microView->at(i);
    
    GzSetScreenSize( m_pRender, 128, 128 );
    GzCoord position = m_pRender->samples->at(sampleIdx).sample.pos;
    GzCoord lookat = position + m_pRender->samples->at(sampleIdx).sample.normal;
    GzSetProjectMatrix( m_pRender, iMatrix );
    GzSetViewMatrix( m_pRender, GzMatrixLookAt( position, lookat, GzCoord(0,1,0) ) );

	  status |= GzNewGBuffer( m_pRender, &gbuf );  
	  status |= GzBindGBuffer( m_pRender, gbuf );
    status |= GzPutSample( m_pRender, zNear, zFar );
    status |= GzSaveGInfo( m_pRender, &(m_pRender->samples->at(sampleIdx).ISM) );

/*
    nameListShader[0]  = GZ_SHADER_FUNC;
    valueListShader[0] = (GzPointer)( shader_depthMap_func );
    nameListShader[1]  = GZ_DATA;
    valueListShader[1] = (GzPointer)( &(m_pRender->samples->at(sampleIdx).sample) );
    status |= GzPutAttribute( m_pRender, 2, nameListShader, valueListShader );
    status |= GzDeferredShading( m_pRender, false );
*/
    fprintf( stderr, "In progress... %.2f%%\r", 100 * (float)(i+1) / (float)(m_pRender->microView->size()) );
  }
  stop_s = clock();
  fprintf( fopt, "Create ISM from selected samples: %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);  


  GzSetScreenSize( m_pRender, m_rWidth, m_rHeight );
  
  start_s = clock();
  GzMatrix lightXsw[MAX_LIGHTS];
  for( int i = 0; i < m_pRender->numlights; i++ ) {
    lightXsw[i] = m_pRender->lights[i].ginfo.Xsp * m_pRender->lights[i].ginfo.Xpc * m_pRender->lights[i].ginfo.Xcw;
  }
  nameListShader[0]  = GZ_SHADER_FUNC;

  valueListShader[0] = (GzPointer)( shader_shadow_func );  
  nameListShader[1]  = GZ_DATA;
  valueListShader[1] = (GzPointer)( lightXsw );

  status |= GzBindMatrices( m_pRender, m_pRender->camera.ginfo );
  status |= GzBindGBuffer( m_pRender, m_pRender->camera.ginfo.gbuf );
  status |= GzEnableLight( m_pRender );
  status |= GzPutAttribute( m_pRender, 2, nameListShader, valueListShader );
  status |= GzDeferredShading( m_pRender );
  stop_s = clock();
  fprintf( fopt, "Camera View Deferred Render (Shadow): %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);

  if( enable_ISM ) {
    start_s = clock();
    float zValues[2] = { zNear, zFar };
    nameListShader[0]  = GZ_SHADER_FUNC;
    valueListShader[0] = (GzPointer)( shader_ISM_func );
//    valueListShader[0] = (GzPointer)( shader_RSM_func );
    nameListShader[1]  = GZ_DATA;
    valueListShader[1] = (GzPointer)( zValues );
    status |= GzPutAttribute( m_pRender, 2, nameListShader, valueListShader );
    status |= GzDeferredShading( m_pRender );
    stop_s = clock();
    fprintf( fopt, "Camera View Deferred Render (ISM): %f\n", (double)(stop_s - start_s)/CLOCKS_PER_SEC);
  }

/*
#if 1
  for( int i = 0; i < (int) m_pRender->microView->size(); i++ ) {
    int sampleIdx = m_pRender->microView->at(i);
#else
  for( int i = 0; i < (int) m_pRender->samples->size(); i++ ) {
    int sampleIdx = i;
#endif
    GzCoord pos = ( m_pRender->camera.ginfo.Xsp * m_pRender->camera.ginfo.Xpc * m_pRender->camera.ginfo.Xcw * m_pRender->samples->at( sampleIdx ).sample.pos ).dW();
    GzDrawRectangle( m_pRender->camera.ginfo.gbuf, m_pRender->camera.ginfo.xres, m_pRender->camera.ginfo.yres, (int)(pos[0]), (int)(pos[1]), 1, 1, GzColor(1,0,0) );
  }
*/

  GzFlushGbuf2File( outfile, m_pRender->gbuf, m_pRender->xres, m_pRender->yres );

	if( fclose( outfile ) ) fprintf( stderr, "The output file was not closed\n" );
	  
	return status;  
}


int Application::doRender() 
{
	GzToken		nameListTriangle[3]; 	  /* vertex attribute names */
	GzPointer	valueListTriangle[3]; 	/* vertex attribute pointers */
	GzCoord		vertexList[3];	        /* vertex position coordinates */ 
	GzCoord		normalList[3];	        /* vertex normals */ 
	GzTextureIndex  	uvList[3];		  /* vertex texture map indices */ 

  GzToken		nameListMaterial[3]; 
	GzPointer valueListMaterial[3];
	GzColor		colorList[3];	        /* material color */ 


	GzToken		nameListShader[2]; 	    /* shader attribute names */
	GzPointer valueListShader[2];		  /* shader attribute pointers */
	


  GzCoord	  transition;
  GzCoord	  scale;
  GzCoord	  rotate;

	char		dummy[256]; 

	char		texturefile[1024]; 

	int			status = 0; 

	/* 
	* Tokens associated with triangle vertex values 
	*/ 
	nameListTriangle[0] = GZ_POSITION; 
	nameListTriangle[1] = GZ_NORMAL; 
	nameListTriangle[2] = GZ_TEXTURE_INDEX;  
  nameListMaterial[0]  = GZ_AMBIENT_COEFFICIENT;
  nameListMaterial[1]  = GZ_DIFFUSE_COEFFICIENT;
  nameListMaterial[2]  = GZ_SPECULAR_COEFFICIENT;

	// I/O File open
	FILE *infile;
	if( (infile  = fopen( inputfile , "r" )) == NULL )
	{
     fprintf( stderr, "The input file was not opened\n" );
		 return GZ_FAILURE;
	}

  GzClearMatrix( m_pRender );

  /* 
	* Walk through the list of triangles, set color 
	* and render each triangle 
	*/ 
	while( fscanf(infile, "%s", dummy) == 1) { 	/* read in tri word */

    if( !strcmp( dummy, "triangle" ) ) {
	    fscanf(infile, "%f %f %f %f %f %f %f %f", 
		                                            &(vertexList[0][0]), &(vertexList[0][1]),  
		                                            &(vertexList[0][2]), 
		                                            &(normalList[0][0]), &(normalList[0][1]), 	
		                                            &(normalList[0][2]), 
		                                            &(uvList[0][0]), &(uvList[0][1]) ); 
	    fscanf(infile, "%f %f %f %f %f %f %f %f", 
		                                            &(vertexList[1][0]), &(vertexList[1][1]), 	
		                                            &(vertexList[1][2]), 
		                                            &(normalList[1][0]), &(normalList[1][1]), 	
		                                            &(normalList[1][2]), 
		                                            &(uvList[1][0]), &(uvList[1][1]) ); 
	    fscanf(infile, "%f %f %f %f %f %f %f %f", 
		                                            &(vertexList[2][0]), &(vertexList[2][1]), 	
		                                            &(vertexList[2][2]), 
		                                            &(normalList[2][0]), &(normalList[2][1]), 	
		                                            &(normalList[2][2]), 
		                                            &(uvList[2][0]), &(uvList[2][1]) ); 

	    /* 
	      * Set the value pointers to the first vertex of the 	
	      * triangle, then feed it to the renderer 
	      * NOTE: this sequence matches the nameList token sequence
	      */ 
	     valueListTriangle[0] = (GzPointer)vertexList; 
		   valueListTriangle[1] = (GzPointer)normalList; 
		   valueListTriangle[2] = (GzPointer)uvList; 
		   GzPutTriangle(m_pRender, 3, nameListTriangle, valueListTriangle); 

    } else if( !strcmp( dummy, "material" ) ) {

      /* in the order of Ka, Kd and Ks */

      fscanf(infile, "%s %f %f %f", dummy, &(colorList[0][0]), &(colorList[0][1]), &(colorList[0][2]) ); 
      fscanf(infile, "%s %f %f %f", dummy, &(colorList[1][0]), &(colorList[1][1]), &(colorList[1][2]) ); 
      fscanf(infile, "%s %f %f %f", dummy, &(colorList[2][0]), &(colorList[2][1]), &(colorList[2][2]) ); 
      valueListMaterial[0] = (GzPointer)&(colorList[0]); 
      valueListMaterial[1] = (GzPointer)&(colorList[1]); 
      valueListMaterial[2] = (GzPointer)&(colorList[2]); 
      GzPutAttribute(m_pRender, 3, nameListMaterial, valueListMaterial); 
      
      fscanf(infile, "%s %s", dummy, texturefile ); 
      
      FILE * fd = fopen( texturefile, "rb" );
      if (fd == NULL) {
        printf( "%s not found\n", texturefile );
        nameListShader[0]  = GZ_TEXTURE_MAP;
        valueListShader[0] = (GzPointer)0;
        GzPutAttribute(m_pRender, 1, nameListShader, valueListShader);
      } else {
        nameListShader[0]  = GZ_TEXTURE_MAP;
        valueListShader[0] = (GzPointer)(tex_fun);
        nameListShader[1]  = GZ_DATA;
        valueListShader[1] = (GzPointer)(texturefile);
        fclose( fd );
        GzPutAttribute(m_pRender, 2, nameListShader, valueListShader);
      }

		  fscanf(infile, "%s %s", dummy, texturefile ); 
      

    } else if( !strcmp( dummy, "matrix" ) ) {
      
      /* in the order of  transition, scale and ( rotate x, rotate y, rotate z ) */
      fscanf(infile, "%f %f %f", &(transition[0]), &(transition[1]), &(transition[2]) ); 
      fscanf(infile, "%f %f %f", &(scale[0]), &(scale[1]), &(scale[2]) ); 
      fscanf(infile, "%f %f %f", &(rotate[0]), &(rotate[1]), &(rotate[2]) ); 
      GzPushMatrix( m_pRender, GzTrxMat( transition ) );
      GzPushMatrix( m_pRender, GzScaleMat( scale ) );
      GzPushMatrix( m_pRender, GzRotXMat( rotate[0] ) );
      GzPushMatrix( m_pRender, GzRotYMat( rotate[1] ) );
      GzPushMatrix( m_pRender, GzRotZMat( rotate[2] ) );

    } else if( !strcmp( dummy, "removematrix" ) ) {
      /* remove five matries */
      GzPopMatrix( m_pRender );
      GzPopMatrix( m_pRender );
      GzPopMatrix( m_pRender );
      GzPopMatrix( m_pRender );
      GzPopMatrix( m_pRender );
    }
	} 

	/* 
	 * Close file
	 */ 

	if( fclose( infile ) )
      fprintf( stderr, "The input file was not closed\n" );
 
	if (status) 
		return(GZ_FAILURE); 
	else 
		return(GZ_SUCCESS); 
}
