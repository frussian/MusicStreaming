#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>
#include "opus.h"
#include "opusfile.h"
#include <QDebug>
#include <QFile>
#include <QAudioOutput>

#include "oggdecoder.h"
#include "audioplayer.h"
#include "networkthread.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

	NetworkThread th(&w);

	OggDecoder *dec = new OggDecoder("C:/Users/zoomo/Desktop/Guitar/GNR/Welcome to the jungle/welcome.opus");
	AudioPlayer *player = new AudioPlayer(&w);
	QObject::connect(dec, SIGNAL(decoded(QByteArray)),
			player, SLOT(writeToBuf(QByteArray)));
	dec->moveToThread(&th);
	qDebug() << QThread::currentThread() << "main id";
	th.start();/*
	QObject::connect(&w, SIGNAL(startDecoding()),
					 dec, SLOT(startDecoding()));*/

	auto btn = new QPushButton("decode", &w);
	btn->show();
	QObject::connect(btn, SIGNAL(clicked()),
					 dec, SLOT(startDecoding()));

    return a.exec();
}
