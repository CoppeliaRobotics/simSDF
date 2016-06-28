#include "UIFunctions.h"
#include "debug.h"
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
    connect(uiproxy, SIGNAL(import(const char *,const ImportOptions*)), this, SLOT(onImport(const char *,const ImportOptions*)));
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

        DBG << "UIFunctions constructed in thread " << QThread::currentThreadId() << std::endl;
    }
    return UIFunctions::instance;
}

void UIFunctions::destroyInstance()
{
    if(UIFunctions::instance)
        delete UIFunctions::instance;
}

void UIFunctions::onImport(const char *fileName, const ImportOptions *options)
{
    try
    {
        import(NULL, std::string(fileName));
    }
    catch(std::string &ex)
    {
        emit error(ex.c_str());
    }
}

