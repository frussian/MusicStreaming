#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QAudio>
#include <QAudioOutput>
#include <QWidget>

class QTimer;
class OggDecoder;

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayer(QObject *parent = nullptr);

signals:
	void writeOpus(QByteArray opus);
	void decode();
	void decReset();
	void processedUSecs(quint64 usecs);
	void seekDec(int pcm_offset);
public slots:
	void writeOpusData(QByteArray opus);
	void start(bool clear);
	void stop(bool clear);
	void seek(int secs);
	void reset();
private slots:
	void handleStateChange(QAudio::State state);
	void notify();
	void writeToBuf(QByteArray pcm);
	void checkBytes();
private:
	enum State {
		NOT_ACTIVE,
		STOPPED,
		ACTIVE,
		NO_MORE_DATA
	};

	int tryWriting();
	QAudioFormat format;
	QAudioOutput *audio;
	QIODevice *dev;
	QByteArray buf;
	OggDecoder *decoder;
	QTimer *checkBytesTimer;
	State state;
};

#endif // AUDIOPLAYER_H
