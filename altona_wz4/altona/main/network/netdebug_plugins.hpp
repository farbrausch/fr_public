/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_NETWORK_NETDEBUG_PLUGINS_HPP
#define FILE_NETWORK_NETDEBUG_PLUGINS_HPP

#include "netdebug.hpp"
#include "http.hpp"

#if sNETDEBUG_ENABLE


/****************************************************************************/
/***                                                                      ***/
/***   Plug-in interface for HTTP debugging                                ***/
/***                                                                      ***/
/****************************************************************************/

// add a new plug-in
// "factory" points to a factory function for your plug-in
// "name" specifies how the plug-in is called in the bar at the top. If NULL,
//        the plug-in won't be visible (eg. if you want to embed custom images)
// "path" specifies an optional path (eg. "/myplugin") . If NULL, a default 
//        path will be chosen.

// Returns:
// -1 : Fatal Error / Not supported
//  0 : Server starting up. Retry later.
//  1 : OK
sInt sAddNetDebugPlugin(sHTTPServer::HandlerCreateFunc factory,const sChar *name, const sChar *path=0);

// add static memory buffer as file
// "path" specifies the URL for the file (eg. "/myplugin/stuff.jpg")
// "buffer" and "size" specify the memory buffer that gets served.

// Returns:
// -1 : Fatal Error / Not supported
//  0 : Server starting up. Retry later.
//  1 : OK
sInt sAddNetDebugFile(const sChar *path, const sU8* buffer, sDInt size);

/****************************************************************************/

#else // sNETDEBUG_ENABLE

sINLINE sInt sAddNetDebugPlugin(sHTTPServer::HandlerCreateFunc,const sChar *name,const sChar *path=0) { return sTRUE; }
sINLINE sInt sAddNetDebugFile(const sChar* path, const sU8* buffer, sDInt size) { return sTRUE; }

#endif

#endif // FILE_NETWORK_NETDEBUG_PLUGINS_HPP
