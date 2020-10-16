#include "UIFunctions.h"
#include "UIProxy.h"

#include <QThread>

#include <iostream>

#include "stubs.h"

// UIFunctions is a singleton

UIFunctions *UIFunctions::instance = NULL;

UIFunctions::UIFunctions(QObject *parent)
    : QObject(parent)
{
    // connect signals/slots from UIProxy to UIFunctions and vice-versa

    UIProxy *uiproxy = UIProxy::getInstance();
    connect(this, SIGNAL(error(const char*)), uiproxy, SLOT(onError(const char*)), Qt::BlockingQueuedConnection);
    connect(uiproxy, SIGNAL(import(const ImportOptions*)), this, SLOT(onImport(const ImportOptions*)));
}

UIFunctions::~UIFunctions()
{
    UIFunctions::instance = NULL;
}

UIFunctions * UIFunctions::getInstance(QObject *parent)
{
    if(!UIFunctions::instance)
    {
        UIFunctions::instance = new UIFunctions(parent);

        simThread();

        sim::addLog(sim_verbosity_debug, "UIFunctions constructed in thread %s", QThread::currentThreadId());
    }
    return UIFunctions::instance;
}

void UIFunctions::destroyInstance()
{
    if(UIFunctions::instance)
        delete UIFunctions::instance;
}

void UIFunctions::onImport(const ImportOptions *options)
{
    import_in in_args;
    options->copyTo(&in_args);
    import_out out_args;
    try
    {
        import(NULL, "<SDF import button>", &in_args, &out_args);
    }
    catch(std::string &ex)
    {
        emit error(ex.c_str());
    }
}

