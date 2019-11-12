#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <iostream>

#include <QThread>

#include "plugin.h"

extern Qt::HANDLE UI_THREAD;
extern Qt::HANDLE SIM_THREAD;

std::string threadNickname();
void uiThread();
void simThread();

#ifndef DEBUG_STREAM
#define DEBUG_STREAM std::cerr
#endif // DEBUG_STREAM

#ifdef __PRETTY_FUNCTION__
#define DBG_WHAT __PRETTY_FUNCTION__
#else
#define DBG_WHAT __func__
#endif

#ifndef NDEBUG
#define DEBUG_OUT DEBUG_STREAM << "\033[1;33m[" << PLUGIN_NAME << ":" << threadNickname() << "] \033[1;31m" << __FILE__ << ":" << __LINE__ << "  \033[1;32m" << DBG_WHAT << "\033[0m" << "  "
#else // NDEBUG
#define DEBUG_OUT if(true) {} else DEBUG_STREAM
#endif // NDEBUG

#endif // DEBUG_H_INCLUDED

