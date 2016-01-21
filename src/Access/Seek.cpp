/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Seek.cpp
*   Time:        2015/06/30
*   Author:      Lysine
*
*   Lysine is a student majoring in Software Engineering
*   from the School of Software, SUN YAT-SEN UNIVERSITY.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
=========================================================================*/

#include "Seek.h"
#include "AccessPrivate.h"
#include "../Utils.h"

Seek *Seek::ins = nullptr;

Seek *Seek::instance()
{
	return ins ? ins : new Seek(qApp);
}

namespace
{
	QStandardItem *praseItem(QNetworkRequest requset)
	{
		auto cell = requset.attribute(QNetworkRequest::User).value<quintptr>();
		return reinterpret_cast<QStandardItem *>(cell);
	}
}

class SeekPrivate : public AccessPrivate<Seek ,Seek::Proc, Seek::Task>
{
public:
	explicit SeekPrivate(Seek *seek):
		AccessPrivate<Seek, Seek::Proc, Seek::Task>(seek)
	{
	}

	virtual bool onError(QNetworkReply *reply) override
	{
		Q_Q(Seek);
		const Seek::Task &task = *q->getHead();
		switch(task.state){
		case Seek::More:
		case Seek::Data:
			return true;
		default:
			return AccessPrivate<Seek, Seek::Proc, Seek::Task>::onError(reply);
		}
	}

	void onThumbArrived(QNetworkReply *reply)
	{
		Q_Q(Seek);
		auto task = q->getHead();
		QPixmap icon;
		if (icon.loadFromData(reply->readAll())){
			icon = icon.scaled(task->cover, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            praseItem(reply->request())->setIcon(icon);
		}
	}
};

namespace
{
	QStringList translate(const QList<const char *> &list)
	{
		QStringList t;
		for (const char *iter : list){
			t.append(Seek::tr(iter));
		}
		return t;
	}
	
	QHash<int, QString> getChannel(QString name)
	{
		static QHash<QString, QHash<int, QString>> m;
		if (!m.contains(name)){
			QFile file(":/Text/DATA");
			file.open(QIODevice::ReadOnly | QIODevice::Text);
			QJsonObject data = QJsonDocument::fromJson(file.readAll()).object()[name + "Channel"].toObject();
			for (auto iter = data.begin(); iter != data.end(); ++iter){
				m[name][iter.key().toInt()] = iter.value().toString();
			}
		}
		return m[name];
	}

	void disableEditing(const QList<QStandardItem *> &list)
	{
		for (QStandardItem *iter : list){
			iter->setEditable(false);
		}
	}

	QString toRichText(QString text)
	{
		return text.isEmpty() ? text : Qt::convertFromPlainText(text);
	}
}

Seek::Seek(QObject *parent) : QObject(parent), d_ptr(new SeekPrivate(this))
{
	Q_D(Seek);
	ins = this;
	setObjectName("Seek");

	QList<const char *> biOrder;
#define tr
	biOrder <<
		tr("default") <<
		tr("pubdate") <<
		tr("senddate") <<
		tr("ranklevel") <<
		tr("click") <<
		tr("scores") <<
		tr("dm") <<
		tr("stow");
#undef tr

	auto biProcess = [=](QNetworkReply *reply){
		Q_D(Seek);
		Task &task = d->queue.head();
		static QHash<const Task *, QList<QNetworkRequest>> images;
		switch (task.state){
		case None:
		{

            // Search changes to fetch pages by ajax_api
            // Request all api:     http://search.bilibili.com/all?keyword=foo
            // All search consists of three results from bangumi and the first page of 20 results from video
            // Ajax all(video) api: http://search.bilibili.com/ajax_api/video?keyword=foo&page=2
            // Request video api:   http://search.bilibili.com/video?keyword=foo
            // Ajax video api:      http://search.bilibili.com/ajax_api/video?keyword=foo&page=2
            // Request bangumi api: http://search.bilibili.com/bangumi?keyword=foo
            // Ajax bangumi api:    http://search.bilibili.com/ajax_api/bangumi?keyword=foo&page=2

            if (task.text.isEmpty()){
				task.model->clear();
				dequeue();
				break;
			}
            QUrl url("http://search." + Utils::customUrl(Utils::Bilibili) + "/video");
			QUrlQuery query;
			query.addQueryItem("keyword", task.text);
			query.addQueryItem("type", "comprehensive");
			query.addQueryItem("page", QString::number(task.page.first));
			query.addQueryItem("pagesize", "20");
			query.addQueryItem("orderby", biOrder[task.sort]);
			url.setQuery(query);
			task.request.setUrl(url);
			task.state = List;
			forward();
			break;
		}
		case List:
		{
			task.model->clear();
			task.model->setHorizontalHeaderLabels({ tr("Cover"), tr("Play"), tr("Danmaku"), tr("Title"), tr("Typename"), tr("Author") });
			task.model->horizontalHeaderItem(0)->setData(-1, Qt::UserRole);
			task.model->horizontalHeaderItem(1)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(2)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(4)->setData(75, Qt::UserRole);
			task.model->horizontalHeaderItem(5)->setData(75, Qt::UserRole);
			const QByteArray &data = reply->readAll();
			auto page = QTextCodec::codecForHtml(data, QTextCodec::codecForName("UTF-8"))->toUnicode(data);

            auto list = page.split(QRegularExpression("<li class=\"list\\W"), QString::SkipEmptyParts);
			if (2 > list.size()) break;
			QString &last = list.last();
			last.truncate(last.indexOf("</li>") + 5);
			list.removeFirst();
			for (const QString &item : list){
				QList<QStandardItem *> line;
				for (int i = 0; i < 6; ++i){
					line.append(new QStandardItem);
				}
				QRegularExpression r;
				QRegularExpressionMatch m;
				r.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
				r.setPattern("av\\d+");
				m = r.match(item);
				bool isBangumi = !m.hasMatch();
				line[0]->setData(m.captured(), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));
				r.setPattern("(?<=src=\")[^\"']+");
				m = r.match(item, isBangumi ? 0 : m.capturedEnd());
                qDebug() << m.captured();

				QNetworkRequest request(m.captured());
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				images[&task].append(request);
				if (isBangumi){
					r.setPattern("(?<=<div class=\"t\">).*?(?=</div>)");
					m = r.match(item, m.capturedEnd());
					line[3]->setText(Utils::decodeXml(m.captured()));
				}
				else{
                    r.setPattern("class=\"tag[^>]+>");
                    m = r.match(item, m.capturedEnd());
                    r.setPattern(".*?(?=</a>)");
                    m = r.match(item, m.capturedEnd());
                    line[4]->setText(m.captured().trimmed());
                    r.setPattern("(?<=title=\")[^\"']+");
                    //r.setPattern("(?=<a .*?class=\"title\"[^>]+).?*(?=</a>)");
                    m = r.match(item, m.capturedEnd());
                    line[3]->setText(Utils::decodeXml(m.captured().trimmed()));

                    r.setPattern("class=\"icon-playtime");
                    m = r.match(item, m.capturedEnd());
                    r.setPattern("(?<=</i>).*?(?=</span>)");
                    m = r.match(item, m.capturedEnd());
                    line[1]->setText(m.captured().trimmed());

                    r.setPattern("class=\"icon-subtitle");
                    m = r.match(item, m.capturedEnd());
                    r.setPattern("(?<=</i>).*?(?=</span>)");
                    m = r.match(item, m.capturedEnd());
                    line[2]->setText(m.captured().trimmed());

                    r.setPattern("class=\"icon-uper");
                    m = r.match(item, m.capturedEnd());
                    r.setPattern("(?<=</i>).*?(?=</span>)");
                    m = r.match(item, m.capturedEnd());
                    line[5]->setText(Utils::decodeXml(m.captured().trimmed()));

				}
                r.setPattern("(?<=intro\">).*?(?=</div>)");
                m = r.match(item);
                line[3]->setToolTip(toRichText(Utils::decodeXml(m.captured())));
                // Bangumi html has changed
                /*if (isBangumi) {
                    r.setPattern("(?<=createSeasonList\\().*?(?=\\);)");
                    m = r.match(item, m.capturedEnd());
                    QStringList args = m.captured().split(',');
                    if (args.size() == 3){
                        for (QString &iter : args){
                            iter = iter.trimmed();
                        }
                        QString url("http://app." + Utils::customUrl(Utils::Bilibili) + "/bangumi/seasoninfo/");
                        url += args[1] + ".ver";
                        QNetworkRequest request(url);
                        request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
                        d->remain += d->manager.get(request);
                    }
                }*/
				disableEditing(line);
				task.model->appendRow(line);
			}
            auto m = QRegularExpression("\\d+").match(page, page.indexOf("data-num_pages"));
			task.page.second = qMax(list.isEmpty() ? 0 : task.page.first, m.captured().toInt());
			if (d->remain.isEmpty()){
				emit stateChanged(task.state = Data);
			}
			else{
				emit stateChanged(task.state = More);
				break;
			}
		}
		case More:
		{
			auto cell = praseItem(reply->request());
            QByteArray r = reply->readAll();
            if (r.startsWith("episodeJsonCallback")) {
                r = r.mid(r.indexOf("code") - 2,  r.length() - r.indexOf("code") + 1);
            }
            QJsonObject seasonlist = QJsonDocument::fromJson(r).object();
            QJsonArray list = seasonlist["result"].toObject()["episodes"].toArray();
			for (const QJsonValue &iter : list){
				QJsonObject item = iter.toObject();
				QList<QStandardItem *> line;
				for (int i = 0; i < 6; ++i){
					line.append(new QStandardItem);
				}
                line[0]->setData(QString("av%1").arg(item["av_id"].toString()), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));
				QNetworkRequest request(item["cover"].toString());
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				images[&task].append(request);
                //line[1]->setText(QString::number(item["click"].toInt()));
                line[3]->setText(Utils::decodeXml(item["index_title"].toString()));
				disableEditing(line);
				cell->insertRow(0, line);
			}
			if (d->remain.isEmpty()){
				if (reply->error() != QNetworkReply::OperationCanceledError){
					for (auto r : images.take(&task)){
						d->remain += d->manager.get(r);
					}
					if (d->remain.isEmpty()){
						emit stateChanged(task.state = None);
						dequeue();
					}
					else{
						emit stateChanged(task.state = Data);
					}
				}
				else{
					images.remove(&task);
				}
			}
			break;
		}
		case Data:
		{
			if (reply->error() == QNetworkReply::NoError){
				d->onThumbArrived(reply);
			}
			if (d->remain.isEmpty() && reply->error() != QNetworkReply::OperationCanceledError){
				emit stateChanged(task.state = None);
				dequeue();
			}
			break;
		}
		}
	};
	d->pool.append({ "Bilibili", translate(biOrder), 0, biProcess });

	QList<const char *> acOrder;
#define tr
	acOrder <<
		tr("rankLevel") <<
		tr("releaseDate") <<
		tr("views") <<
		tr("comments") <<
		tr("stows");
#undef tr

	auto acProcess = [=](QNetworkReply *reply){
		Q_D(Seek);
		Task &task = d->queue.head();
		switch (task.state){
		case None:
		{
			if (task.text.isEmpty()){
				task.model->clear();
				dequeue();
				break;
			}
			QUrl url("http://search." + Utils::customUrl(Utils::AcFun) + "/search");
			QUrlQuery query;
			query.addQueryItem("q", task.text);
			query.addQueryItem("sortType", "-1");
			query.addQueryItem("field", "title");
			query.addQueryItem("sortField", acOrder[task.sort]);
			query.addQueryItem("pageNo", QString::number(task.page.first));
			query.addQueryItem("pageSize", "20");
			url.setQuery(query);
			task.request.setUrl(url);
			task.state = List;
			forward();
			break;
		}
		case List:
		{
			task.model->clear();
			task.model->setHorizontalHeaderLabels({ tr("Cover"), tr("Play"), tr("Comment"), tr("Title"), tr("Typename"), tr("Author") });
			task.model->horizontalHeaderItem(0)->setData(-1, Qt::UserRole);
			task.model->horizontalHeaderItem(1)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(2)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(4)->setData(75, Qt::UserRole);
			task.model->horizontalHeaderItem(5)->setData(75, Qt::UserRole);
			QJsonObject page = QJsonDocument::fromJson(reply->readAll()).object()["data"].toObject()["page"].toObject();
			QJsonArray list = page["list"].toArray();
			for (int i = 0; i < list.count(); ++i){
				QJsonObject item = list[i].toObject();
				int channelId = item["channelId"].toDouble();
				if (channelId == 110 || channelId == 63 || (channelId > 72 && channelId < 77)) {
					continue;
				}
				QList<QStandardItem *> line;
				line << new QStandardItem();
				line << new QStandardItem(QString::number((int)item["views"].toDouble()));
				line << new QStandardItem(QString::number((int)item["comments"].toDouble()));
				line << new QStandardItem(Utils::decodeXml(item["title"].toString()));
				line << new QStandardItem(getChannel("AcFun")[channelId]);
				line << new QStandardItem(Utils::decodeXml(item["username"].toString()));
				line[0]->setData(item["contentId"].toString(), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));
				line[3]->setToolTip(toRichText(item["description"].toString()));
				QNetworkRequest request(QUrl(item["titleImg"].toString()));
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				d->remain += d->manager.get(request);
				disableEditing(line);
				task.model->appendRow(line);
			}
			task.page.second = qCeil(page["totalCount"].toDouble() / page["pageSize"].toDouble());
			if (d->remain.isEmpty()){
				emit stateChanged(task.state = None);
				dequeue();
			}
			else{
				emit stateChanged(task.state = Data);
			}
			break;
		}
		case Data:
		{
			if (reply->error() == QNetworkReply::NoError){
				d->onThumbArrived(reply);
			}
			if (d->remain.isEmpty() && reply->error() != QNetworkReply::OperationCanceledError){
				emit stateChanged(task.state = None);
				dequeue();
			}
			break;
		}
		}
	};
	d->pool.append({ "AcFun", translate(acOrder), 0, acProcess });

	connect(this, &Seek::stateChanged, [this](int code){
		switch (code){
		case None:
		case List:
		case More:
		case Data:
			break;
		default:
		{
			Q_D(Seek);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Seek::~Seek()
{
	delete d_ptr;
}

void Seek::addProc(const Seek::Proc *proc)
{
	Q_D(Seek);
	d->addProc(proc);
}

const Seek::Proc *Seek::getProc(QString name)
{
	Q_D(Seek);
	return d->getProc(name);
}

QStringList Seek::modules()
{
	Q_D(Seek);
	QStringList list;
	for (const Proc &iter : d->pool){
		if (list.contains(iter.name)){
			continue;
		}
		list << iter.name;
	}
	return list;
}

void Seek::dequeue()
{
	Q_D(Seek);
    Task* t = d->getHead();
	d->dequeue();
}

bool Seek::enqueue(const Seek::Task &task)
{
	Q_D(Seek);
	if (d->queue.size()){
		d->dequeue();
	}
	return d->enqueue(task);
}

Seek::Task *Seek::getHead()
{
	Q_D(Seek);
	return d->getHead();
}

void Seek::forward()
{
	Q_D(Seek);
	d->forward();
}
