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
	void seek(int pcm_offset);
//	int startDecoding();
signals:
	void decoded(QByteArray pcm);
public:
	bool isReset = false;
private:
	static int oggRead(void *stream, unsigned char *ptr, int bytes);
	static int oggSeek(void *stream, int64_t offset, int whence);
	static int64_t oggTell(void *stream);
	OggOpusFile *dec = nullptr;
	int offset = 0;
	bool inited = false;
	QByteArray opus;
	QByteArray opusTmp;
	AudioPlayer *player;
};

#endif // OGGDECODER_H
