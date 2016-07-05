#ifndef UIFUNCTIONS_H_INCLUDED
#define UIFUNCTIONS_H_INCLUDED

#include <QObject>
#include <QString>

#include "ImportOptions.h"

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
    void onImport(const ImportOptions *options);

signals:
    void error(const char *msg);
};

#endif // UIFUNCTIONS_H_INCLUDED

