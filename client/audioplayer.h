#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QAudio>
#include <QAudioOutput>

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayer(QObject *parent = nullptr);

public slots:
	void handleStateChange(QAudio::State state);
	void notify();
	void writeToBuf(QByteArray pcm);
private:
	int tryWriting();
	QAudioFormat format;
	QAudioOutput *audio;
	QIODevice *dev;
	QByteArray *buf;
};

#endif // AUDIOPLAYER_H
