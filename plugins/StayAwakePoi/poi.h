#ifndef POI_H
#define POI_H

#include "stayawakepoi_global.h"
#include <QtCore>
#include <QtWidgets>
#import <IOKit/pwr_mgt/IOPMLib.h>


extern "C" POI_EXPORT void regist(const QHash<QString, QObject *> &objects);

extern "C" POI_EXPORT QString string(QString query);

extern "C" POI_EXPORT void config(QWidget *parent);

class Poi: public QObject
{
    Q_OBJECT
public:
    explicit Poi(QObject* parent, QObject* aplayer);
    ~Poi();
private:
    int state;
    IOPMAssertionID assertionID;
public slots:
    void preventSleep(int state);
};

#endif // POI_H
