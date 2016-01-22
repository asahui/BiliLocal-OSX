#ifndef BILIAPI_H
#define BILIAPI_H

#include "biliapi_global.h"
#include <QtCore>
#include <QtWidgets>


extern "C" BILIAPISHARED_EXPORT void regist(const QHash<QString, QObject *> &objects);
extern "C" BILIAPISHARED_EXPORT QString string(QString query);
extern "C" BILIAPISHARED_EXPORT void config(QWidget *parent);


class BiliAPI:public QObject
{

public:
    explicit BiliAPI(QObject* parent, QObject *load);

private:
};

#endif // BILIAPI_H
