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
    //connect(this, SIGNAL(create(Proxy*)), uiproxy, SLOT(onCreate(Proxy*)), Qt::BlockingQueuedConnection);
    //connect(uiproxy, SIGNAL(buttonClick(Widget*)), this, SLOT(onButtonClick(Widget*)));
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

        DBG << "UIFunctions constructed in thread " << QThread::currentThreadId() << std::endl;

        simThread();
    }
    return UIFunctions::instance;
}

void UIFunctions::destroyInstance()
{
    if(UIFunctions::instance)
        delete UIFunctions::instance;
}

//void UIFunctions::onEvent(void *foo)
//{
//}

