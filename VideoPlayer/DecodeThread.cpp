#include "DecodeThread.h"
#include "Decoder.h"

DecodeThread::DecodeThread(QObject *parent):QThread(parent)
{

}

DecodeThread::~DecodeThread()
{

}

void DecodeThread::setDecoder(Decoder * d)
{
	decoder = d;
}

void DecodeThread::run()
{
	decoder->decode();
}
