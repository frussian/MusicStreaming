#ifndef OGGDECODER_H
#define OGGDECODER_H
#include <QWidget>

struct OggOpusFile;

class OggDecoder: public QObject {
	Q_OBJECT

public:
	OggDecoder(QString file);
public slots:
	int decode(char *encoded);
	int startDecoding();
signals:
	void decoded(QByteArray pcm);
private:
	static int oggRead(void *stream, unsigned char *ptr, int bytes);
	OggOpusFile *dec;
};

#endif // OGGDECODER_H
