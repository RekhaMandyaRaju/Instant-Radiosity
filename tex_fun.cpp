/* Texture functions for cs580 GzLib	*/

#include	"stdio.h"



#include	"gz.h"
#include	"rend.h"



/* Image texture function */
int tex_fun( GzPointer	data, float u, float v, GzColor *color)

{
  unsigned char		pixel[3];
  unsigned char     dummy;
  char  		foo[8];
  FILE			*fd;


  GzRender * render = ( GzRender * ) data;
  std::string name = ( char * ) render->sharder_data;
  std::map< std::string, GzTextureFile > *textureFileMapping = render->textureFileMapping;
  GzColor	*image;
  int xs, ys;
    
  std::map< std::string, GzTextureFile >::iterator it = textureFileMapping->find( name );
  
  if( it == textureFileMapping->end() ) {
    fprintf( stderr, "New texture [%s]\n", name.c_str() );
    fd = fopen ( name.c_str(), "rb");
    if (fd == NULL) {
      fprintf (stderr, "texture file (%s) not found\n", name.c_str() );
      exit(-1);
    }
    fscanf (fd, "%s %d %d %c", foo, &xs, &ys, &dummy);
    
    fprintf( stderr, "Texture read: %d,%d\n", xs, ys );
    

    image = (GzColor*)malloc(sizeof(GzColor)*((xs+1)*(ys+1)+xs+1));
    if (image == NULL) {
      fprintf (stderr, "malloc for texture image failed\n");
      exit(-1);
    }

    for ( int i = 0; i < xs*ys; i++) {	/* create array of GzColor values */
      fread(pixel, sizeof(pixel), 1, fd);
      image[i][RED] = (float)((int)pixel[RED]) * (1.0 / 255.0);
      image[i][GREEN] = (float)((int)pixel[GREEN]) * (1.0 / 255.0);
      image[i][BLUE] = (float)((int)pixel[BLUE]) * (1.0 / 255.0);

    }

	  fclose(fd);
	  
	  GzTextureFile texture;
	  texture.xs    = xs;
	  texture.ys    = ys;
	  texture.image = image;
	  texture.name  = name;
	  
    (*textureFileMapping)[ name ] = texture;
    
  } else {
    
    GzTextureFile texture = it->second;
    xs    = texture.xs;
    ys    = texture.ys;
    image = texture.image;
    
  }



/* bounds-test u,v to make sure nothing will overflow image array bounds */
/* determine texture cell corner values and perform bilinear interpolation */
/* set color to interpolated GzColor value and return */
//  u = ( u > 1 ) ? 1 : ( u < 0 ) ? 0 : u;
//  v = ( v > 1 ) ? 1 : ( v < 0 ) ? 0 : v;

  bool p = false;
  
//  if( u > 1 || u < 0 ) p = true;
//  if( v > 1 || v < 0 ) p = true;
    
  if( p ) printf( "%f,%f -> ", u,v );
    
  if( u > 0 ) u = u - floor(u);
  else u = ceil( -1 * u ) + u;
  if( v > 0 ) v = v - floor(v);
  else v = ceil( -1 * v ) + v;
    
  if( p ) printf( "%f,%f\n", u,v );
    
//  u = 1.0 - u;
//  v = 1.0 - v;
    


  float x = u * ( xs - 1 );
  float y = v * ( ys - 1 );
  float s = x - floor( x );
  float t = y - floor( y );
  float a = s * t;
  float b = t - a;
  float c = s - a;
  float d = 1 - s - t + a;
  int D = floor( x ) + floor( y ) * xs;
  int A = D + xs + 1;
  int B = D + xs;
  int C = D + 1;

  (*color) = image[A] * a + image[B] *b + image[C] *c + image[D] *d;

  return GZ_SUCCESS;
}
/*
  Perlin Noise
  http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/

float Persistence = 0.5;
int Number_Of_Octaves = 4;

float Noise( int x, int y)
{
  int n = x + y * 57;
  n = ( n << 13 ) ^ n;
  return ( ( float ) ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0 ) / 2;    
}

float SmoothNoise( float x, float y )
{
  float corners = ( Noise( x - 1, y - 1 ) + Noise( x + 1, y - 1 ) + Noise( x - 1, y + 1 ) + Noise( x + 1, y+ 1 ) ) / 16;
  float sides   = ( Noise( x - 1, y) + Noise( x + 1, y) + Noise( x, y - 1 ) + Noise( x , y + 1 ) ) / 8;
  float center  =  Noise( x, y) / 4;
  return corners + sides + center;
}


float Interpolate( float a, float b, float f )
{
  return ( 1 - f ) * a + f * b;
}

float InterpolatedNoise( float x, float y )
{
  int integer_X = int( x );
  float fractional_X = x - integer_X;

  int integer_Y = int( y );
  float fractional_Y = y - integer_Y;

  float v1 = SmoothNoise( integer_X,     integer_Y     );
  float v2 = SmoothNoise( integer_X + 1, integer_Y     );
  float v3 = SmoothNoise( integer_X,     integer_Y + 1 );
  float v4 = SmoothNoise( integer_X + 1, integer_Y + 1 );

  float i1 = Interpolate(v1 , v2 , fractional_X);
  float i2 = Interpolate(v3 , v4 , fractional_X);

  return Interpolate(i1 , i2 , fractional_Y);
}

float PerlinNoise_2D(float x, float y)
{
  float total = 0;
  float p = Persistence;
  int n = Number_Of_Octaves - 1;

  for( int i = 0; i < n; i++ ) {
    float frequency = pow( 2.0, i );
    float amplitude = pow( p, i );
    total = total + InterpolatedNoise( x * frequency, y * frequency ) * amplitude;
  }
  return total;
}

/* Procedural texture function */

int ptex_fun( GzPointer	data, float u, float v, GzColor *color)

{
  u = 20 * u;
  v = 20 * v;

  float value = PerlinNoise_2D( u, v );
  *color = GzColor( value, value, value );
  /*
  color[0] = PerlinNoise_2D( u, v );
	color[1] = PerlinNoise_2D( u, v );
	color[2] = PerlinNoise_2D( u, v );
  */
  return GZ_SUCCESS;
}

