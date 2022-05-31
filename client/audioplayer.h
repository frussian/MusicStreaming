#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QAudio>
#include <QAudioOutput>
#include <QWidget>

class OggDecoder;

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayer(QString stylesheet, QObject *parent = nullptr);

signals:
	void writeOpus(QByteArray opus);
	void decode();
public slots:
	void handleStateChange(QAudio::State state);
	void notify();
	void writeToBuf(QByteArray pcm);
	void writeOpusData(QByteArray opus);
private:
	void initUI(QString stylesheet);
	int tryWriting();
	QAudioFormat format;
	QAudioOutput *audio;
	QIODevice *dev;
	QByteArray buf;
	OggDecoder *decoder;
};

#endif // AUDIOPLAYER_H
