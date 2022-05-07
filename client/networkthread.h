#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QThread>

class NetworkThread: public QThread
{
	Q_OBJECT
public:
	explicit NetworkThread(QObject *parent = nullptr);
protected:
//	virtual void run() override;
};

#endif // NETWORKTHREAD_H
