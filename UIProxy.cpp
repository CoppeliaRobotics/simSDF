#include "UIProxy.h"
#include "debug.h"

#include <QThread>
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QDialog>

#include "stubs.h"

// UIProxy is a singleton

UIProxy *UIProxy::instance = NULL;

UIProxy::UIProxy(QObject *parent)
    : QObject(parent)
{
}

UIProxy::~UIProxy()
{
    UIProxy::instance = NULL;
}

UIProxy * UIProxy::getInstance(QObject *parent)
{
    if(!UIProxy::instance)
    {
        UIProxy::instance = new UIProxy(parent);

        DBG << "UIProxy constructed in thread " << QThread::currentThreadId() << std::endl;

        uiThread();
    }
    return UIProxy::instance;
}

void UIProxy::destroyInstance()
{
    if(UIProxy::instance)
        delete UIProxy::instance;
}

//void UIProxy::onEvent(void *foo)
//{
//}

