#ifndef NETWORKPARSER_H
#define NETWORKPARSER_H

#include <QObject>
#include <QAbstractSocket>
#include <set>

#include "messages.pb.h"

class QTcpSocket;

class NetworkParser : public QObject
{
	Q_OBJECT
public:
	explicit NetworkParser(QThread *th);
	void connectToHost(QString host, int port);
public slots:
	int requestTable(uint64_t reqId, int first, int last, QString filter,
					 enum EntityType type);
private slots:
	void connected();
	void disconnected();
	void socketError(QAbstractSocket::SocketError);
	void reconnect();
	void readyRead();
	void stateChanged(QAbstractSocket::SocketState);
signals:
	void tableAns(uint64_t reqId, TableAns ans);
	void simpleAns(uint64_t reqId, SimpleAns ans);
	void streamAns(uint64_t reqId, StreamAns);
	void parserConnected();
	void reqFailed(uint64_t reqId);
private:
	int handleSize();
	int handlePkt();
	void prepareReq(uint64_t reqId);

	std::set<uint64_t> requests;
	QString host;
	int port;
	bool isConnected = false;
	QTcpSocket *socket;
	int state;
	QByteArray buf;
	int pktlen;
};

//Q_DECLARE_METATYPE(QAbstractSocket::SocketError);
//Q_DECLARE_METATYPE(QAbstractSocket::SocketState);

#endif // NETWORKPARSER_H
