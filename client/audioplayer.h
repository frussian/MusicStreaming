#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QAudio>
#include <QAudioOutput>
#include <QWidget>

class AudioPlayer : public QWidget
{
	Q_OBJECT
public:
	explicit AudioPlayer(QString stylesheet, QWidget *parent = nullptr);

public slots:
	void handleStateChange(QAudio::State state);
	void notify();
	void writeToBuf(QByteArray pcm);
private:
	void initUI(QString stylesheet);
	int tryWriting();
	QAudioFormat format;
	QAudioOutput *audio;
	QIODevice *dev;
	QByteArray *buf;
};

#endif // AUDIOPLAYER_H
