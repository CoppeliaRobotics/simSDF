// Copyright 2016 Coppelia Robotics GmbH. All rights reserved. 
// marc@coppeliarobotics.com
// www.coppeliarobotics.com
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -------------------------------------------------------------------
// Authors:
// Federico Ferri <federico.ferri.it at gmail dot com>
// -------------------------------------------------------------------

#include "v_repExtSDF.h"
#include "debug.h"
#include "v_repLib.h"
#include "SDFDialog.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <map>

#include <boost/algorithm/string/predicate.hpp>

#include <QThread>

#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
    #endif
#endif /* _WIN32 */
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
#define _stricmp strcasecmp
#endif /* __linux || __APPLE__ */

#define CONCAT(x, y, z) x y z
#define strConCat(x, y, z)    CONCAT(x, y, z)

#define PLUGIN_NAME "SDF"

#define PLUGIN_VERSION 9    // 1 until 20/1/2013 (1 was a very early beta)
                            // 2 until 10/1/2014 (V-REP3.0.5)
                            // 3: new lock
                            // 4: since V-REP 3.1.2
                            // 5: since after V-REP 3.1.3
                            // 6: since V-REP 3.2.2
                            // 7: since V-REP 3.2.2 rev2
                            // 8: since V-REP 3.3.0 (headless mode detect)
                            // 9: since V-REP 3.3.1 (Using stacks to exchange data with scripts)

LIBRARY vrepLib; // the V-REP library that we will dynamically load and bind
SDFDialog *sdfDialog = NULL;

#include "tinyxml2.h"

#include "stubs.h"
#include "UIFunctions.h"
#include "UIProxy.h"

using namespace tinyxml2;

void import(SScriptCallBack *p, const char *cmd, import_in *in, import_out *out)
{
    XMLDocument xmldoc;
    XMLError err = xmldoc.LoadFile(in->fileName.c_str());
    if(err != XML_NO_ERROR)
        throw std::string("xml load error");

    XMLElement *root = xmldoc.FirstChildElement();
    std::cout << "XML root: <" << root->Name() << ">" << std::endl;
    for(XMLElement *e = root->FirstChildElement(); e; e = e->NextSiblingElement())
    {
        std::cout << "child: <" << e->Name() << ">" << std::endl;
    }
}

VREP_DLLEXPORT unsigned char v_repStart(void* reservedPointer, int reservedInt)
{
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL, curDirAndFile, 1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);
#ifdef _WIN32
    temp+="\\v_rep.dll";
#elif defined (__linux)
    temp+="/libv_rep.so";
#elif defined (__APPLE__)
    temp+="/libv_rep.dylib";
#endif /* __linux || __APPLE__ */
    vrepLib = loadVrepLibrary(temp.c_str());
    if(vrepLib == NULL)
    {
        std::cout << "Error, could not find or correctly load the V-REP library. Cannot start '" PLUGIN_NAME "' plugin.\n";
        return(0);
    }
    if(getVrepProcAddresses(vrepLib)==0)
    {
        std::cout << "Error, could not find all required functions in the V-REP library. Cannot start '" PLUGIN_NAME "' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    int vrepVer;
    simGetIntegerParameter(sim_intparam_program_version, &vrepVer);
    if(vrepVer < 30301) // if V-REP version is smaller than 3.03.01
    {
        std::cout << "Sorry, your V-REP copy is somewhat old. Cannot start '" PLUGIN_NAME "' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    if(simGetBooleanParameter(sim_boolparam_headless) > 0)
    {
        //std::cout << "V-REP runs in headless mode. Cannot start 'Urdf' plugin.\n";
        //unloadVrepLibrary(vrepLib);
        //return(0); // Means error, V-REP will unload this plugin
    }
    else
    {
        QWidget *mainWindow = (QWidget *)simGetMainWindow(1);
        sdfDialog = new SDFDialog(mainWindow);
        simAddModuleMenuEntry("", 1, &sdfDialog->dialogMenuItemHandle);
        simSetModuleMenuItemState(sdfDialog->dialogMenuItemHandle, 1, "SDF import...");
    }

    if(!registerScriptStuff())
    {
        std::cout << "Initialization failed.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    UIProxy::getInstance(); // construct UIProxy here (UI thread)

    return(PLUGIN_VERSION); // initialization went fine, we return the version number of this plugin (can be queried with simGetModuleName)
}

VREP_DLLEXPORT void v_repEnd()
{
    if(sdfDialog)
        delete sdfDialog;

    UIFunctions::destroyInstance();
    UIProxy::destroyInstance();

    unloadVrepLibrary(vrepLib); // release the library
}

VREP_DLLEXPORT void* v_repMessage(int message, int* auxiliaryData, void* customData, int* replyData)
{
    // Keep following 5 lines at the beginning and unchanged:
    static bool refreshDlgFlag = true;
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode, &errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode, sim_api_errormessage_ignore);
    void* retVal=NULL;

    static bool firstInstancePass = true;
    if(firstInstancePass && message == sim_message_eventcallback_instancepass)
    {
        firstInstancePass = false;
        UIFunctions::getInstance(); // construct UIFunctions here (SIM thread)
    }

    if(message == sim_message_eventcallback_simulationended)
    { // Simulation just ended
        // TODO: move this to sim_message_eventcallback_simulationabouttoend
    }

    if(message == sim_message_eventcallback_menuitemselected)
    { // A custom menu bar entry was selected
        if(auxiliaryData[0] == sdfDialog->dialogMenuItemHandle)
            sdfDialog->makeVisible(!sdfDialog->getVisible());
    }

    // Keep following unchanged:
    simSetIntegerParameter(sim_intparam_error_report_mode, errorModeSaved); // restore previous settings
    return(retVal);
}

