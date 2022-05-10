#include <QDebug>
#include <QFile>
#include <QVector>
#include <QThread>

#include "oggdecoder.h"
#include "opus.h"
#include "opusfile.h"

#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define APPLICATION OPUS_APPLICATION_AUDIO
#define BITRATE 64000
#define MAX_FRAME_SIZE 6*960
#define MAX_PACKET_SIZE (3*1276)

OggDecoder::OggDecoder(QString file):
	QObject(nullptr)
{
	QFile *filePtr = new QFile(file);
	filePtr->open(QIODevice::ReadOnly);
	OpusFileCallbacks *cbs = new OpusFileCallbacks;

	cbs->seek = NULL;
	cbs->tell = NULL;
	cbs->read = oggRead;
	cbs->close = NULL;  //free fileptr
	int err;
	int n = 57;
	unsigned char initData[n];
	n = filePtr->read((char*)initData, n);

	dec = op_open_callbacks(filePtr, cbs, initData, n, &err);
	if (err) qDebug() << err;

//	dec = op_open_file(file.toStdString().data(), nullptr);
}

int OggDecoder::decode(char *encoded)
{
	int n = 960 * 6;
	opus_int16 pcm[n];
	int r;

//	if (len) {
//		dev->write(buf->data() + appendPos, )
//	}

	r = op_read_stereo(dec, pcm, n);
	if (r < 0) {
		qDebug() << "error" << r;
		return -1;
	} else if (r == 0) {
		qDebug() << "end";
		return 0;
	}

	int bytes = r * 2 * sizeof(opus_int16);
//	qDebug() << r << "r";
//	qDebug() << bytes << "can write";
//	qDebug() << f << "free bytes";

//	qDebug() << bytes << "wrote";
	QByteArray pcmArr((char*)pcm, bytes);
	emit decoded(pcmArr);

	return r;
}

int OggDecoder::startDecoding()
{
	unsigned long sum = 0;
	qDebug() << QThread::currentThreadId() << "dec id";
	while(1) {
		int r = decode(nullptr);
		if (r < 0) return r;
		if (r == 0) return sum;
		sum += r;
//		if (sum > 50000) return sum;
	}
	return sum;
}

int count = 1;

int OggDecoder::oggRead(void *ptr, unsigned char *data, int len)
{
	QFile *file = (QFile*)ptr;
	count++;
	if (count % 225 == 0) {
		qDebug() << count;
		return 0;
	}
	return file->read((char*)data, len);
}
