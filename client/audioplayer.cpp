#include <QDebug>
#include <QAudio>
#include <QAudioOutput>
#include "audioplayer.h"

AudioPlayer::AudioPlayer(QObject *parent)
	: QObject{parent}
{
	buf = new QByteArray();

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
	audio->setBufferSize(16000);
	dev = audio->start();

	audio->setNotifyInterval(30);
	audio->resume();

	qDebug() << "Construct";

	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChange(QAudio::State)));
	connect(audio, SIGNAL(notify()), this, SLOT(notify()));
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
	tryWriting();
}

void AudioPlayer::writeToBuf(QByteArray pcm)
{
//	qDebug() << pcm.length();
	buf->append(pcm);
	tryWriting();
}

int AudioPlayer::tryWriting()
{
	int bytesToWrite = std::min(audio->bytesFree(), buf->length());
	if (!bytesToWrite) return 0;
	dev->write(buf->data(), bytesToWrite);
	buf->remove(0, bytesToWrite);
	return bytesToWrite;
}
