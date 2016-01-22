#include "biliapi.h"
#include <QStandardItemModel>
#include <QtNetwork>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDomDocument>


void regist(const QHash<QString, QObject *> &objects)
{
    BiliAPI* biliApi = new BiliAPI(objects["Interface"], objects["Load"]);
}

enum QueryName
{
    Name,
    Version,
    Description,
    Author
};

static QHash<QString, QueryName> queryNameMap;

QString string(QString query)
{
    queryNameMap["Name"] = Name;
    queryNameMap["Version"] = Version;
    queryNameMap["Description"] = Description;
    queryNameMap["Author"] = Author;
    switch(queryNameMap[query]) {
    case Name:
        return QString("BiliAPI");
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
namespace
{
    std::function<bool(QString &)> getRegular(QRegularExpression ex)
    {
        return [ex](QString &code){
            auto iter = ex.globalMatch(code);
            code.clear();
            while (iter.hasNext()){
                code = iter.next().captured();
            }
            return code.length() > 2;
        };
    }
}

/*
implement functions used by this library from class Load
Load::State
Load::Role
Load::Task
forward
stateChange
dequeue
addProc

used but not capsulate, use invokeMethod directly
getModel
dumpDanmaku
*/
// from Load
namespace Load {
    enum State
    {
        None = 0,
        Page = 381,
        Part = 407,
        Code = 379,  // author use this to get cid
        File = 384
    };

    enum Role
    {
        UrlRole = Qt::UserRole,
        StrRole,
        NxtRole
    };

    struct Proc
    {
        std::function<bool(QString &)> regular;
        int priority;
        std::function<void(QNetworkReply *)> process;
    };

    struct Task
    {
        QString code;
        QNetworkRequest request;
        int state;
        const Proc *processer;
        qint64 delay;
        Task() :state(None), processer(nullptr), delay(0){}
    };


    void addProc(QObject* obj, const Load::Proc *proc)
    {
        QMetaObject::invokeMethod(obj, "addProc", Q_ARG(const Load::Proc *, proc));
    }

    void stateChanged(QObject* obj, int state)
    {
        QMetaObject::invokeMethod(obj, "stateChanged", Q_ARG(int, state));
    }

    void forward(QObject* obj, QNetworkRequest request, int state)
    {
        QMetaObject::invokeMethod(obj, "forward", Q_ARG(QNetworkRequest, request), Q_ARG(int, state));
    }

    void dequeue(QObject* obj)
    {
        QMetaObject::invokeMethod(obj, "dequeue");
    }

}

//from Util
namespace Utils
{
    enum Site
    {
        Unknown,
        Bilibili,
        AcFun,
        Tudou,
        Letv,
        AcfunLocalizer,
        Niconico,
        TuCao,
        ASS
    };

    QString decodeXml(QString string, bool fast)
    {
        if (!fast){
            QTextDocument text;
            text.setHtml(string);
            return text.toPlainText();
        }
        QString fixed;
        fixed.reserve(string.length());
        int i = 0, l = string.length();
        for (i = 0; i < l; ++i){
            QChar c = string[i];
            if (c >= ' ' || c == '\n'){
                bool f = true;
                switch (c.unicode()){
                case '&':
                    if (l - i >= 4){
                        switch (string[i + 1].unicode()){
                        case 'l':
                            if (string[i + 2] == 't'&&string[i + 3] == ';'){
                                fixed += '<';
                                f = false;
                                i += 3;
                            }
                            break;
                        case 'g':
                            if (string[i + 2] == 't'&&string[i + 3] == ';'){
                                fixed += '>';
                                f = false;
                                i += 3;
                            }
                            break;
                        case 'a':
                            if (l - i >= 5 && string[i + 2] == 'm'&&string[i + 3] == 'p'&&string[i + 4] == ';'){
                                fixed += '&';
                                f = false;
                                i += 4;
                            }
                            break;
                        case 'q':
                            if (l - i >= 6 && string[i + 2] == 'u'&&string[i + 3] == 'o'&&string[i + 4] == 't'&&string[i + 5] == ';'){
                                fixed += '\"';
                                f = false;
                                i += 5;
                            }
                            break;
                        }
                    }
                    break;
                case '/':
                case '\\':
                    if (l - i >= 2){
                        switch (string[i + 1].unicode()){
                        case 'n':
                            fixed += '\n';
                            f = false;
                            i += 1;
                            break;
                        case 't':
                            fixed += '\t';
                            f = false;
                            i += 1;
                            break;
                        case '\"':
                            fixed += '\"';
                            f = false;
                            i += 1;
                            break;
                        }
                    }
                    break;
                }
                if (f){
                    fixed += c;
                }
            }
        }
        return fixed;
    }

    QString APPKEY = "85eb6835b0a1034e";
    QString APPSEC = "2ad42749773c441109bdc0191257a664";

    // Add a hash function for API usage
    QString hash(QString aid, QString pid)
    {
        QString url = "appkey=%2&id=%1&page=%3&type=json";
        url = url.arg(aid, Utils::APPKEY, pid) + Utils::APPSEC;
        QString hashCode = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex();
        return hashCode;
    }

    QString hash1(QString cid)
    {
        QString url = "appkey=%1&cid=%2";
        url = url.arg(Utils::APPKEY, cid) + Utils::APPSEC;
        QString hashCode = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex();
        return hashCode;
    }
}


BiliAPI::BiliAPI(QObject* parent, QObject* object):QObject(parent)
{
    QObject* load = object;

    // get index of method and then use method() to get QMetaMethod
    //qDebug() << "BiliAPI" << load->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("dequeue()").data());

    auto biliapiProcess = [load](QNetworkReply *reply) {
        Load::Task * _task;
        QMetaObject::invokeMethod(load, "getHead", Q_RETURN_ARG(Load::Task *, _task));
        Load::Task &task = *_task;
        int sharp = task.code.indexOf(QRegularExpression("[#_]"));
        switch (task.state) {
            case Load::State::None:
            {
                QString aid = task.code.mid(2, sharp - 2);
                QString pid = sharp == -1 ? "1" : task.code.mid(sharp + 1);

                QString params = "appkey=%2&id=%1&page=%3&type=json";
                params = params.arg(aid, Utils::APPKEY, pid);

                QString apiUrl = "http://api.%1/view?%2&sign=%3";
                apiUrl = apiUrl.arg("bilibili.com", params, Utils::hash(aid, pid));
                Load::forward(load, QNetworkRequest(apiUrl), Load::State::Code);
                break;
            }
            case Load::State::Code:
            {
                QJsonObject j = QJsonDocument::fromJson(reply->readAll()).object();
                if (j.contains("error") || !j.contains("cid")) {
                    Load::stateChanged(load, 203); // will connect to slot and try next process
                    Load::dequeue(load);
                    break;
                }
                int cid = j["cid"].toInt();
                QString apiUrl = "http://comment.%1/%2.xml";
                apiUrl = apiUrl.arg("bilibili.com").arg(cid);
                qDebug() << "api request url:" << apiUrl;
                //load->forward(QNetworkRequest(api.arg(id)), Load::State::File);
                Load::forward(load, QNetworkRequest(apiUrl), Load::State::File);
                break;
            }
            case Load::State::File:
            {
                //load->dumpDanmaku(reply->readAll(), Utils::Bilibili, false);
                QMetaObject::invokeMethod(load, "dumpDanmaku", Q_ARG(const QByteArray &, reply->readAll()),
                                          Q_ARG(int, Utils::Bilibili), Q_ARG(bool, false));
                //emit load->stateChanged(task.state = Load::State::None);
                //load->dequeue();
                Load::stateChanged(load, task.state = Load::State::None);
                Load::dequeue(load);
                break;
            }
        }
    };
    auto biliapiRegular = [](QString &code){
        code.remove(QRegularExpression("/index(?=_\\d+\\.html)"));
        QRegularExpression r("a(v(\\d+([#_])?(\\d+)?)?)?");
        r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        return getRegular(r)(code);
    };
    Load::Proc proc = {biliapiRegular, 100, biliapiProcess};
    QMetaObject::invokeMethod(object, "addProc", Q_ARG(const Load::Proc *, &proc));

    auto biliURLRegular = [](QString &code) {
        QRegularExpression r("urla(v(\\d+([#_])?(\\d+)?)?)?");
        r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        return getRegular(r)(code);
    };
    auto biliURLProcess = [=](QNetworkReply *reply) {
        Load::Task * _task;
        QMetaObject::invokeMethod(load, "getHead", Q_RETURN_ARG(Load::Task *, _task));
        Load::Task &task = *_task;
        int sharp = task.code.indexOf(QRegularExpression("[#_]"));
        switch (task.state) {
            case Load::State::None:
            {
                QString aid = task.code.mid(5, sharp - 5);
                QString pid = sharp == -1 ? "1" : task.code.mid(sharp + 1);

                QString params = "appkey=%2&id=%1&page=%3&type=json";
                params = params.arg(aid, Utils::APPKEY, pid);

                QString apiUrl = "http://api.%1/view?%2&sign=%3";
                apiUrl = apiUrl.arg("bilibili.com", params, Utils::hash(aid, pid));
                Load::forward(load, QNetworkRequest(apiUrl), Load::State::Code);
                break;
            }
            case Load::State::Code:
            {
                QJsonObject j = QJsonDocument::fromJson(reply->readAll()).object();
                if (j.contains("error") || !j.contains("cid")) {
                    Load::stateChanged(load, 203); // will connect to slot and try next process
                    Load::dequeue(load);
                    break;
                }
                int cid = j["cid"].toInt();
                QString title;
                if (j.contains("title")) {
                    title = j["title"].toString();
                    QMetaObject::invokeMethod(parent, "setWindowTitle", Q_ARG(const QString &, title));
                }
                /*
                 *  oversea: http://interface.bilibili.com/v_cdn_play?
                 *  None: http://interface.bilibili.com/playurl?
                 */
                QString params = "appkey=%1&cid=%2";
                params = params.arg(Utils::APPKEY).arg(QString::number(cid));
                QString apiUrl = "http://interface.%1/playurl?%2&sign=%3";
                apiUrl = apiUrl.arg("bilibili.com").arg(params).arg(Utils::hash1(QString::number(cid)));
                qDebug() << "api request url:" << apiUrl;
                //load->forward(QNetworkRequest(api.arg(id)), Load::State::File);
                task.code = task.code.mid(3);
                QNetworkRequest request(apiUrl);
                if (!title.isEmpty()) {
                    qDebug() << "setAttribute: " << title;
                    //QString *name = new QString(title.data());
                    request.setAttribute(QNetworkRequest::User, title);
                }
                Load::forward(load, request, Load::State::File);
                break;
            }
            case Load::State::File:
            {
                // TODO: decode xml, get url,Load it
                QDomDocument doc;
                QStringList urlList;
                if (doc.setContent(reply->readAll(), false)) {
                    QDomNodeList durlList = doc.elementsByTagName("durl");
                    for (int i = 0; i < durlList.size(); i++) {
                        QDomNodeList uList = durlList.item(i).toElement().elementsByTagName("url");
                        for (int j = 0; j < qMin(uList.size(), 1); j++) {
                            QDomNodeList cdataList = uList.item(j).toElement().childNodes();
                            for (int k = 0; k < cdataList.size(); k++) {
                                if (cdataList.item(k).isCDATASection()) {
                                    qDebug() << uList.item(j).toElement().tagName() << ":" << cdataList.item(k).toCDATASection().data();
                                    urlList.append(cdataList.item(k).toCDATASection().data());
                                }
                            }
                        }
                    }
                } else {
                    Load::stateChanged(load, 204);
                    Load::dequeue(load);
                    break;
                }

                if (urlList.isEmpty()) {
                    Load::stateChanged(load, 203);
                } else {
                    // invoke APlayer->setMedia
                    QString title = reply->request().attribute(QNetworkRequest::User).value<QString>();
                    //QString *title = reinterpret_cast<QString *>(t);
                    qDebug() << "attribute:" << title;
                    QMetaObject::invokeMethod(load, "setMedia", Q_ARG(const QStringList, urlList), Q_ARG(const QString, title));
                    Load::stateChanged(load, task.state = Load::State::None);
                }
                Load::dequeue(load);
                break;
            }
        }
    };
    Load::Proc biliURLProc = {biliURLRegular, 0, biliURLProcess};
    QMetaObject::invokeMethod(object, "addProc", Q_ARG(const Load::Proc *, &biliURLProc));

}
