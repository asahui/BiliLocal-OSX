/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Load.h
*   Time:        2014/04/22
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

#pragma once

#include <QtCore>
#include <QtNetwork>
#include <QStandardItemModel>
#include <functional>

class Load : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None = 0,
		Page = 381,
		Part = 407,
		Code = 379,
		File = 384,
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

signals:
	void stateChanged(int state);

public slots:
	void addProc(const Load::Proc *proc);

    void dumpDanmaku(const QByteArray &data, int site, bool full);

	QStandardItemModel *getModel();
	void forward(QNetworkRequest, int);
	void dequeue();
	Load::Task *getHead();
};
