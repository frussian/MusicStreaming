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

	cbs.seek = oggSeek;
	cbs.tell = oggTell;
	cbs.read = oggRead;
	cbs.close = NULL;  //free

	offset = 0;
	opus.clear();

	opus.append(data);
	qDebug() << "init decoder" << data.size();
	offset = data.size();
	err = 0;
	dec = op_open_callbacks(this, &cbs, (const unsigned char*)data.data(), data.size(), &err);
	inited = true;
	if (err) qDebug() << "init error" << err;
	else qDebug() << "success init";
	op_set_dither_enabled(dec, false);

//	QTimer::singleShot(0, this, SLOT(decode()));

	return err;
}

int OggDecoder::availableForDec()
{
	return opus.size()-offset;
}

int OggDecoder::decode()
{
	int r;
	int n = 960 * 6;
	opus_int16 pcm[n];  //TODO: выделять один раз

	if (!dec) return 0;

	if (availableForDec() <= 0) {
//		qDebug() << "not available for decoding";
		return 0;
	}
	qDebug() << opus.size() << offset << "avail";
//	QTimer::singleShot(0, this, &OggDecoder::decode);  //for continuous decoding

	r = op_read_stereo(dec, pcm, n);
	qDebug() << "read" << r;
	if (r < 0) {
		qDebug() << "error" << r;
		return -1;
	} else if (r == 0) {
//		qDebug() << "end";
		return 0;
	}

	int bytes = r * 2 * sizeof(opus_int16);

//	qDebug() << bytes << "decoded";
	QByteArray pcmArr((char*)pcm, bytes);
	emit decoded(pcmArr);  //TODO: maybe make it a direct call

	return r;
}

int OggDecoder::writeOpus(QByteArray newOpus)
{
	qDebug() << "opus size" << opus.size() << opusTmp.size();
	if (isReset) {
		opusTmp.append(newOpus);
		return 0;
	}

	if (opusTmp.size() != 0) {
		opusTmp.append(newOpus);
		init(opusTmp);
		opusTmp.clear();
		return 0;
	}

	if (opus.size() == 0) {
		init(newOpus);
		return 0;
	}

	opus.append(newOpus);
//	return decode();
	return 0;
}

void OggDecoder::reset()
{
	qDebug() << "reset";
	opus.clear();
	if (dec) op_free(dec);
	dec = nullptr;
	offset = 0;
	isReset = false;
}

void OggDecoder::seek(int sample_offset)
{
	op_pcm_seek(dec, sample_offset);
}

int OggDecoder::oggRead(void *ptr, unsigned char *data, int len)
{
	OggDecoder *decoder = (OggDecoder*)ptr;
	qDebug() << "reading" << decoder->offset <<  len;
	int offset = decoder->offset;
	int size = decoder->opus.size();
	unsigned char *opus = (unsigned char*)decoder->opus.data();

	if (offset == size) {
		qDebug() << "offset == size";
		return 0;
	}

	if (offset + len > size) {
		len = size - offset;
	}
	if (len == 0) {
		qDebug() << "len 0";
	}
	memcpy(data, opus + offset, len);
	decoder->offset += len;

	return len;
}

int OggDecoder::oggSeek(void *stream, int64_t offset, int whence)
{
	OggDecoder *decoder = (OggDecoder*)stream;
	qDebug() << "offset" << offset << "whence" << whence;
	if (whence == SEEK_SET) {
		decoder->offset = offset;
	} else if (whence == SEEK_END) {
		decoder->offset = decoder->opus.size() + offset;
	} else {
		decoder->offset += offset;
	}

	return 0;
}

int64_t OggDecoder::oggTell(void *stream)
{
	OggDecoder *decoder = (OggDecoder*)stream;
	qDebug() << "tell" << decoder->offset;
	return decoder->offset;
}
