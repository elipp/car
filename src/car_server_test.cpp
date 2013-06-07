// car_server_test.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include "net/server.h"

BOOL CtrlHandler( DWORD fdwCtrlType ) {
	switch(fdwCtrlType) {
		 case CTRL_CLOSE_EVENT:
			 Server::shutdown();
			 return TRUE;
			 break;
		 default:
			 return FALSE;
			 ;
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	timeBeginPeriod(1);
	if(!Server::init((unsigned short)50000)) { return EXIT_FAILURE; }	// start server thread
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
	timeEndPeriod(1);
	while (1) {
		Sleep(1000);	
	}// do nothing in main thread :P
	return 0;
}

