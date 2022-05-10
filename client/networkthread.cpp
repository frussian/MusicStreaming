#include <QEventLoop>
#include <QTcpSocket>
#include "networkthread.h"

NetworkThread::NetworkThread(QObject *parent):
	QThread(parent)
{
}

void NetworkThread::run() {
	QEventLoop ev;
	while (1) {
		ev.processEvents(ev.AllEvents, 100);
	}
}
