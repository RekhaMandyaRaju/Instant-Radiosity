// Application6.h: interface for the Application4 class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

struct GzDisplay;
struct GzRender;
#include "gz.h"

class Application  
{
public:
	Application();
	Application( const char *input, const char *output );
	virtual ~Application();
	
public:
  


  char *inputfile;
  char *outputfile;
  
	GzDisplay* m_pDisplay;		// the display
	GzRender*  m_pRender;		// the renderer
	char* m_pFrameBuffer;	// Frame Buffer
	int   m_nWidth;			// width of Frame Buffer
	int   m_nHeight;		// height of Frame Buffer
	int   m_rWidth;			// width of renderer
	int   m_rHeight;		// height of renderer


  GzCamera camera;
  

	int	Initialize();
	virtual int Render(); 
	
	int doRender();
};
