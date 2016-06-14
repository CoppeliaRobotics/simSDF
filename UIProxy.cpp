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
#include <QMessageBox>

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

        uiThread();

        DBG << "UIProxy constructed in thread " << QThread::currentThreadId() << std::endl;
    }
    return UIProxy::instance;
}

void UIProxy::destroyInstance()
{
    if(UIProxy::instance)
        delete UIProxy::instance;
}

void UIProxy::onError(const char *msg)
{
    QWidget *mainWindow = (QWidget*)simGetMainWindow(1);
    QMessageBox::warning(mainWindow, "SDF Plugin", msg, QMessageBox::Ok);
}

