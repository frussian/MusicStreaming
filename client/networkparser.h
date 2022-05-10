#ifndef NETWORKPARSER_H
#define NETWORKPARSER_H

#include <QObject>
#include "messages.pb.h"

class QTcpSocket;

class NetworkParser : public QObject
{
	Q_OBJECT
public:
	explicit NetworkParser(QThread *th);
	bool connectToHost(QString host, int port);
public slots:
	int requestTable(uint64_t reqId, int first, int last, QString filter,
					 enum EntityType type);
private slots:
	void connected();
	void readyRead();

signals:

private:
	QString host;
	int port;
	QTcpSocket *socket;
};

#endif // NETWORKPARSER_H
