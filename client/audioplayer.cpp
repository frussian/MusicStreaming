#include <QDebug>
#include <QAudio>
#include <QAudioOutput>
#include <QGridLayout>
#include <QSlider>
#include <QPushButton>
#include <QGroupBox>
#include <QThread>
#include <QTimer>

#include "audioplayer.h"
#include "oggdecoder.h"

#define AUDIO_BUFFER_SIZE 32000

AudioPlayer::AudioPlayer(QObject *parent)
	: QObject(parent)
{
	qDebug() << QThread::currentThread();

	decoder = new OggDecoder(this);

	state = NOT_ACTIVE;

	QThread *th = new QThread;
	moveToThread(th);
	th->start();

	format.setSampleRate(48000);
	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(format)) {
		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
	}

	audio = new QAudioOutput(format);
	audio->setBufferSize(AUDIO_BUFFER_SIZE);
	dev = audio->start();
	qDebug() << "buffer size" << audio->bufferSize();

	audio->setNotifyInterval(30);

	checkBytesTimer = new QTimer;
	checkBytesTimer->moveToThread(th);
	checkBytesTimer->setInterval(100);
	connect(checkBytesTimer, &QTimer::timeout, this, &AudioPlayer::checkBytes);

	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChange(QAudio::State)));
	connect(audio, SIGNAL(notify()), this, SLOT(notify()));
	connect(decoder, SIGNAL(decoded(QByteArray)), this, SLOT(writeToBuf(QByteArray)));

	connect(this, SIGNAL(decode()), decoder, SLOT(decode()));
	connect(this, SIGNAL(writeOpus(QByteArray)), decoder, SLOT(writeOpus(QByteArray)));
	connect(this, SIGNAL(decReset()), decoder, SLOT(reset()));
}

void AudioPlayer::handleStateChange(QAudio::State state)
{
	qDebug() << state << "state";
	switch (state) {
		case QAudio::IdleState:
				// Finished playing (no more data)
//				audio->stop();
//				delete audio;
		qDebug() << "Idle";
//		decode();
		break;
		case QAudio::StoppedState:
			// Stopped for other reasons
			qDebug() << "Stopped";
//			emit decode();
			if (audio->error() != QAudio::NoError) {
				// Error handling
				qDebug() << audio->error();
			}
			break;
		default:
//			qDebug() << state;
		// ... other cases as appropriate
			break;
		}
}

#define MSEC_100_LEFT 20000

void AudioPlayer::notify()
{
	tryWriting();
}

void AudioPlayer::checkBytes()
{
	if (state == ACTIVE && buf.length() < MSEC_100_LEFT) {
//		qDebug() << "decoding";
		emit decode();
		QTimer::singleShot(0, this, SLOT(checkBytes()));
	} else {
//		qDebug() << "enough bytes";
		emit processedUSecs(audio->processedUSecs());
	}
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

void AudioPlayer::start(bool clear)
{
	qDebug() << "start player";
	state = ACTIVE;
	if (clear) {
		audio->reset();
		audio->setBufferSize(AUDIO_BUFFER_SIZE);
		dev = audio->start();
		buf.clear();
		emit decReset();
	}
	audio->resume();
	checkBytesTimer->start();
}

void AudioPlayer::stop(bool /*clear*/)
{
	state = STOPPED;
	audio->suspend();
	checkBytesTimer->stop();
}

int AudioPlayer::tryWriting()
{
	int bytesToWrite = std::min(audio->bytesFree(), buf.length());
	if (!bytesToWrite) return 0;
//	qDebug() << "writing to dev" << bytesToWrite << audio->bytesFree() << buf.length();
	dev->write(buf.data(), bytesToWrite);
	buf.remove(0, bytesToWrite);
	return bytesToWrite;
}
