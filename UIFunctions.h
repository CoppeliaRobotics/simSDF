#ifndef UIFUNCTIONS_H_INCLUDED
#define UIFUNCTIONS_H_INCLUDED

#include <QObject>
#include <QString>

class UIFunctions : public QObject
{
    Q_OBJECT

public:
    virtual ~UIFunctions();

    static UIFunctions * getInstance(QObject *parent = 0);
    static void destroyInstance();

private:
    UIFunctions(QObject *parent = 0);

    static UIFunctions *instance;

public slots:

private slots:
    //void onEvent(void *foo);

signals:
    //void mySignal(void *bar);
};

#endif // UIFUNCTIONS_H_INCLUDED

