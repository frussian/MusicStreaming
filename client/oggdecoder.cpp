#include <QDebug>
#include <QFile>
#include <QVector>
#include <QThread>
#include <QTimer>

#include "oggdecoder.h"
#include "opus.h"
#include "opusfile.h"
#include "audioplayer.h"


#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define APPLICATION OPUS_APPLICATION_AUDIO
#define BITRATE 64000
#define MAX_FRAME_SIZE 6*960
#define MAX_PACKET_SIZE (3*1276)

OggDecoder::OggDecoder(AudioPlayer *player, QObject *parent):
	QObject(parent)
{
	this->player = player;
	auto th = new QThread;
	moveToThread(th);
	th->start();
}

int OggDecoder::init(QByteArray data)
{
	OpusFileCallbacks cbs;
	int err;

	cbs.seek = NULL;
	cbs.tell = NULL;
	cbs.read = oggRead;
	cbs.close = NULL;  //free fileptr

	offset = 0;
	opus.clear();

	opus.append(data);
	qDebug() << "init decoder";

	if (dec) {
		//TODO: free opus or reinit
	}
	err = 0;
	dec = op_open_callbacks(this, &cbs, nullptr, 0, &err);
	if (err) qDebug() << "init error" << err;
	else qDebug() << "success init";
	op_set_dither_enabled(dec, false);
	qDebug() << op_channel_count(dec, -1) << op_bitrate_instant(dec);

	QTimer::singleShot(0, this, SLOT(decode()));

	return err;
}

int OggDecoder::availableForDec()
{
	return opus.size()-offset;
}

int OggDecoder::decode()
{
	int r;
	if (!dec) return 0;
	int n = 960 * 6;
	opus_int16 pcm[n];  //TODO: выделять один раз

	if (availableForDec() == 0) {
		qDebug() << "not available for decoding";
		return 0;
	}

	QTimer::singleShot(0, this, &OggDecoder::decode);

	r = op_read_stereo(dec, pcm, n);
	if (r < 0) {
		qDebug() << "error" << r;
		return -1;
	} else if (r == 0) {
		qDebug() << "end";
		return 0;
	}

	int bytes = r * 2 * sizeof(opus_int16);

	qDebug() << bytes << "decoded";
	QByteArray pcmArr((char*)pcm, bytes);
	emit decoded(pcmArr);  //TODO: maybe make it a direct call

	return r;
}

int OggDecoder::writeOpus(QByteArray newOpus)
{
//	qDebug() << "opus size" << opus.size();

	if (opus.size() == 0) {
		init(newOpus);
		return 0;
	}

	opus.append(newOpus);
	return decode();
}

int OggDecoder::oggRead(void *ptr, unsigned char *data, int len)
{
	OggDecoder *decoder = (OggDecoder*)ptr;
	qDebug() << "reading" << len;
	int offset = decoder->offset;
	int size = decoder->opus.size();
	unsigned char *opus = (unsigned char*)decoder->opus.data();

	if (offset == size) return 0;

	if (offset + len > size) {
		len = size - offset;
	}

	memcpy(data, opus + offset, len);
	decoder->offset += len;

	return len;
}



