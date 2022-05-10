#include "networkparser.h"
#include <QTcpSocket>
#include <QDebug>
#include <QThread>

NetworkParser::NetworkParser(QThread *th):
	QObject(NULL)
{
	moveToThread(th);
	socket = new QTcpSocket(th);
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
//	connect(socket, SIGNAL(connected), )
}

bool NetworkParser::connectToHost(QString host, int port)
{
	this->host = host;
	this->port = port;
	socket->connectToHost(host, port);
	return socket->waitForConnected();  //TODO: maybe remove
}

void NetworkParser::connected()
{
	qDebug() << "connected to" << host << "on port" << port;
	qDebug() << thread();
}

void NetworkParser::readyRead()
{
	qDebug() << "ready to read" << socket->bytesAvailable();
}

int NetworkParser::requestTable(uint64_t reqId, int first, int last,
								QString filter, EntityType type)
{
	Request reqWrapper;
	reqWrapper.set_cancel(false);
	reqWrapper.set_reqid(reqId);

	TableReq *req = new TableReq();
	req->set_first(first);
	req->set_last(last);
	req->set_filter(filter.toStdString());
	req->set_type(type);
	reqWrapper.set_allocated_tablereq(req);
	uint32_t total = 4;
	total += reqWrapper.ByteSizeLong();
	char tmp[total];
	*(uint32_t*)tmp = total-4;
	reqWrapper.SerializeToArray(tmp + 4, total-4);
	qDebug() << total << "total sent";
	socket->write(tmp, total);
	return 0;
}
