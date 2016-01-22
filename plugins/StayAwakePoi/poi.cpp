#include "poi.h"
#include "../BiliLocal/src/Player/APlayer.h"

#import <IOKit/pwr_mgt/IOPMLib.h>
// kIOPMAssertionTypeNoDisplaySleep prevents display sleep,
// kIOPMAssertionTypeNoIdleSleep prevents idle sleep

// reasonForActivity is a descriptive string used by the system whenever it needs
// to tell the user why the system is not sleeping. For example,
// "Mail Compacting Mailboxes" would be a useful string.

// NOTE: IOPMAssertionCreateWithName limits the string to 128 characters.

enum QueryName
{
    Name,
    Version,
    Description,
    Author
};

static QHash<QString, QueryName> queryNameMap;

void regist(const QHash<QString, QObject *> &objects)
{
    Poi* poi = new Poi(objects["Interface"], objects["APlayer"]);
}

QString string(QString query)
{
    queryNameMap["Name"] = Name;
    queryNameMap["Version"] = Version;
    queryNameMap["Description"] = Description;
    queryNameMap["Author"] = Author;
    switch(queryNameMap[query]) {
    case Name:
        return QString("StayAwakePoi");
        break;
    case Version:
        return QString("0.4.2");
        break;
    case Description:
        return QString("Poooooi!");
        break;
    case Author:
        return QString("asahui");
        break;
    default:
        break;
    }
}

void config(QWidget *parent)
{
    QDialog d(parent);
    if (d.exec()) {

    }
}

Poi::Poi(QObject *parent, QObject *aplayer):QObject(parent)
{
    state = 0;
    assertionID = 0;

    connect(aplayer,
            aplayer->metaObject()->method(aplayer->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("stateChanged(int)"))),
            this,
            this->metaObject()->method(this->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("preventSleep(int)")))
            );
}

void Poi::preventSleep(int _state)
{
    CFStringRef reasonForActivity= CFSTR("Playing: Prevent Sleeping");
    if (_state == APlayer::Play || _state == APlayer::Loop) {
        //qDebug() << "assertionID: " << assertionID;
        if (state != 0) {
            return;
        }
        IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                            kIOPMAssertionLevelOn, reasonForActivity, &assertionID);
        if (success == kIOReturnSuccess)
        {
            qDebug() << "Poi: tetoku, stay awake!";
            //qDebug() << "assertionID: " << assertionID;
            state = 1;
            //  Add the work you need to do without
            //  the system sleeping here.
        }
    } else {
        state = 0;
        IOReturn success = IOPMAssertionRelease(assertionID);
        if (success == kIOReturnSuccess) {
            qDebug() << "Poi: oyasumi";
            //qDebug() << "assertionID: " << assertionID;
            //assertionID = 0;
        }
        //  The system will be able to sleep again.
    }
}

Poi::~Poi()
{
    if (state != 0) {
        state = 0;
        IOReturn success = IOPMAssertionRelease(assertionID);
        if (success == kIOReturnSuccess) {
            qDebug() << "Poi: oyasumi";
            //qDebug() << "assertionID: " << assertionID;
        }
    }
}
