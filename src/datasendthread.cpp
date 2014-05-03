#include <QDebug>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "datasendthread.h"


DataSendThread::DataSendThread(int serialPortDescriptor, QByteArray dataToSend, QObject *parent) : QThread(parent)
{
    m_serialPortDescriptor = serialPortDescriptor;
    m_dataToSend = dataToSend;
}

void DataSendThread::run()
{
    qDebug() << "sender thread started";
    const unsigned int segmentSize = 64;

    if(m_dataToSend.size() > segmentSize)
    {
        unsigned int segments = m_dataToSend.size() / segmentSize;
        unsigned int remainder = m_dataToSend.size() - segments*segmentSize;

        for(unsigned int i = 0; i < segments; i++)
        {
            QByteArray buffer = m_dataToSend.mid(segmentSize*i, segmentSize);

            char * buf = buffer.data();
            unsigned int n = buffer.length();
            qDebug() << "sending data of length " << n << ", segment " << i << " of " << segments;

            ::write(m_serialPortDescriptor, buf, n);
            emit progress(i+1,segments);
        }

        if(remainder > 0)
        {
            QByteArray buffer = m_dataToSend.mid(segmentSize*(segments),remainder);

            char * buf = buffer.data();
            unsigned int n = buffer.length();
            qDebug() << "sending data of length " << n << ", remainder";

            ::write(m_serialPortDescriptor, buf, n);
            emit progress(segments,segments);
        }
    }
    else
    {
        char * buf = m_dataToSend.data();
        unsigned int n = m_dataToSend.length();
        qDebug() << "sending data of length " << n << ", one-go";

        ::write(m_serialPortDescriptor, buf, n);
    }


    qDebug() << "sender thread exiting";
    emit sendThreadFinished();
}


void DataSendThread::stopRequest()
{
    m_stopRequest = true;
}
