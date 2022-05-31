#ifndef OGGDECODER_H
#define OGGDECODER_H

#include <QWidget>
#include <QFile>

struct OggOpusFile;
class AudioPlayer;

class OggDecoder: public QObject {
	Q_OBJECT

public:
	OggDecoder(AudioPlayer *player, QObject *parent = nullptr);
public slots:
	int writeOpus(QByteArray encoded);
	int init(QByteArray encoded); //first portion
	int availableForDec();
	int decode();
	void reset();
//	int startDecoding();
signals:
	void decoded(QByteArray pcm);
private:
	static int oggRead(void *stream, unsigned char *ptr, int bytes);
	OggOpusFile *dec = nullptr;
	int offset = 0;
	QByteArray opus;
	AudioPlayer *player;
};

#endif // OGGDECODER_H
