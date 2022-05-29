#include "networkparser.h"
#include <QTcpSocket>
#include <QDebug>
#include <QThread>
#include <QTimer>

#define TIMEOUT_MS 3000

NetworkParser::NetworkParser(QThread *th):
	QObject(th), state(0)
{
//	moveToThread(th);
	socket = new QTcpSocket(this);
	qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(connected()), this, SIGNAL(parserConnected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SLOT(stateChanged(QAbstractSocket::SocketState)));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(socketError(QAbstractSocket::SocketError)));
//	connect(socket, SIGNAL(connected), )
}

void NetworkParser::connectToHost(QString host, int port)
{
	this->host = host;
	this->port = port;
	socket->connectToHost(host, port);
}

void NetworkParser::connected()
{
	qDebug() << "connected to" << host << "on port" << port;
	isConnected = true;
}

void NetworkParser::reconnect() {
//	if (socket->state() == QAbstractSocket::UnconnectedState) {
//		socket->connectToHost(host, port);
//	}

//	QTimer::singleShot(4*1000, this,
//					   SLOT(reconnect()));
	socket->connectToHost(host, port);
}

void NetworkParser::disconnected() {
	qDebug() << "disconnected from " << host << ":" << port;
	isConnected = false;
}

void NetworkParser::socketError(QAbstractSocket::SocketError err)
{
	(void)err;
	qDebug() << err;
	QTimer::singleShot(0, this, &NetworkParser::reconnect);
}

void NetworkParser::stateChanged(QAbstractSocket::SocketState st)
{
	(void)st;
//	qDebug() << st;
//	if (st == QAbstractSocket::)
}

int NetworkParser::handleSize()
{
	if (buf.size() < 4) {
		return 0;
	}

	pktlen = *((int*)buf.data());
	state = 1;
	return 4;
}

int NetworkParser::handlePkt()
{
	if (buf.size() < pktlen) {
		return 0;
	}

	Answer ans;
	ans.ParseFromArray(buf.data(), pktlen);
	Answer::MsgCase msgType = ans.msg_case();

	requests.erase(ans.reqid());

	switch (msgType) {
	case Answer::kTableAns: {
		TableAns t = ans.tableans();
		emit tableAns(ans.reqid(), t);
		break;
	}
	case Answer::kSimpleAns: {
		SimpleAns s = ans.simpleans();
		emit simpleAns(ans.reqid(), s);
		break;
	}
	case Answer::kStreamAns: {
		StreamAns s = ans.streamans();
		emit streamAns(ans.reqid(), s);
		break;
	}
	case Answer::MSG_NOT_SET: {
		qDebug() << "Parser error: msg was not set";
		break;
	}
	}

	state = 0;
	return pktlen;
}

void NetworkParser::readyRead()
{
	qDebug() << "ready to read" << socket->bytesAvailable();
	int read = -1;
	buf.append(socket->readAll());
	while (buf.size() != 0 && read != 0) {
		if (state == 0) {
			read = handleSize();
			buf.remove(0, read);
		}
		if (state == 1) {
			read = handlePkt();
			buf.remove(0, read);
		}
	}
}

void NetworkParser::prepareReq(uint64_t reqId)
{
	requests.insert(reqId);
	QTimer::singleShot(TIMEOUT_MS, this, [this, reqId](){
		if (requests.find(reqId) != requests.end()) {
			emit reqFailed(reqId);
		}
	});
}

int NetworkParser::requestTable(uint64_t reqId, int first, int last,
								QString filter, EntityType type)
{
	prepareReq(reqId);

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
	socket->write(tmp, total);
	return 0;
}
