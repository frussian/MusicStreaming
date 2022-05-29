#include <QDebug>
#include <QAudio>
#include <QAudioOutput>
#include <QGridLayout>
#include <QSlider>
#include <QPushButton>
#include <QGroupBox>
#include <QThread>

#include "audioplayer.h"
#include "oggdecoder.h"

AudioPlayer::AudioPlayer(QString stylesheet, QWidget *parent)
	: QWidget(parent)
{
	initUI(stylesheet);

	qDebug() << QThread::currentThread();

	decoder = new OggDecoder(this);
	QThread *th = new QThread;
	decoder->moveToThread(th);
	th->start();

	format.setSampleRate(48000);
	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(format)) {
		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
	}

	audio = new QAudioOutput(format);
	audio->setVolume(0.001);
	audio->setBufferSize(16000);
	dev = audio->start();

	audio->setNotifyInterval(30);
	audio->resume();

	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChange(QAudio::State)));
	connect(audio, SIGNAL(notify()), this, SLOT(notify()));
	connect(decoder, SIGNAL(decoded(QByteArray)), this, SLOT(writeToBuf(QByteArray)));

	connect(this, SIGNAL(decode()), decoder, SLOT(decode()));
	connect(this, SIGNAL(writeOpus(QByteArray)), decoder, SLOT(writeOpus(QByteArray)));
}

void AudioPlayer::initUI(QString stylesheet)
{
	QGridLayout *playLay = new QGridLayout;

//	QProgressBar *progress = new QProgressBar;
//	progress->setMinimum(0);
//	progress->setMaximum(60);
//	progress->setValue(43);
//	progress->setMaximumHeight(20);
//	progress->setTextVisible(false);
//	progress->setStyleSheet(customStyleSheet);
//	playLay->addWidget(progress);
	QSlider *playSlider = new QSlider(Qt::Horizontal);
	playSlider->setMinimum(0);
	playSlider->setMaximum(100);
	playSlider->setValue(63);
	playSlider->setStyleSheet(stylesheet);

	QPushButton *playBtn = new QPushButton;
//	playBtn->setFixedSize(70, 70);
//	QRect rect(QPoint(), playBtn->size());
//	rect.adjust(10, 10, -10, -10);
//	QRegion region(rect,QRegion::Ellipse);
	playBtn->setStyleSheet(stylesheet);
//	playBtn->setMask(region);
//	playBtn->setFixedSize(50, 50);

	QVBoxLayout *songInfoLay = new QVBoxLayout;
	QPushButton *songNameBtn = new QPushButton("SongName");
	songNameBtn->setProperty("isFlat", true);
	songNameBtn->setStyleSheet("QPushButton { border: none; }");
	QPushButton *bandNameBtn = new QPushButton("BandName");
	bandNameBtn->setFlat(true);
	songInfoLay->addWidget(songNameBtn);
	songInfoLay->addWidget(bandNameBtn);
	songInfoLay->addStretch();
//	bandNameBtn->setStyleSheet("QPushButton { border: none; }");


	playLay->addWidget(playBtn, 0, 1);
	playLay->setAlignment(playBtn, Qt::AlignCenter);
	playLay->addWidget(playSlider, 1, 1);
	playLay->addLayout(songInfoLay, 0, 0, 2, 1);

	setLayout(playLay);
}

void AudioPlayer::handleStateChange(QAudio::State state)
{
	switch (state) {
		case QAudio::IdleState:
				// Finished playing (no more data)
//				audio->stop();
//				delete audio;
//		qDebug() << "Idle";
//		decode();
		break;
		case QAudio::StoppedState:
			// Stopped for other reasons
			qDebug() << "Stopped";
			if (audio->error() != QAudio::NoError) {
				// Error handling
			}
			break;
		default:
//			qDebug() << state;
		// ... other cases as appropriate
			break;
		}
}

void AudioPlayer::notify()
{
//	qDebug() << audio->bytesFree();
//	decode();
//	if (decoder->availableForDec() > 0) decoder->decode();
	emit decode();
	tryWriting();
}

void AudioPlayer::writeToBuf(QByteArray pcm)
{
//	qDebug() << pcm.length();
	buf.append(pcm);
	tryWriting();
}

void AudioPlayer::writeOpusData(QByteArray opus)
{
	emit writeOpus(opus);
//	decoder->writeOpus(opus);
}

int AudioPlayer::tryWriting()
{
	int bytesToWrite = std::min(audio->bytesFree(), buf.length());
	if (!bytesToWrite) return 0;
	dev->write(buf.data(), bytesToWrite);
	buf.remove(0, bytesToWrite);
	return bytesToWrite;
}
