/**
 * Main entry point
 * USC csci 580 *
*/

#include <stdio.h>
#include <string.h>
#include "Application.h"

int main(int argc, const char * argv[])
{

  Application * application;
  if( argc == 3 ) application = new Application( argv[1], argv[2] );

  if( argc == 2 ) application = new Application( argv[1], NULL );

  else application = new Application();
  if( application->Initialize() ) printf("Error: initialize\n");
  printf("Start rendering\n");
  if( application->Render() ) printf("Error: render\n");    
  delete application;

  return 0;
}

