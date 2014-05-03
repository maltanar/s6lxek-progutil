#include <QDebug>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "datareceivethread.h"

DataReceiveThread::DataReceiveThread(int serialPortDescriptor, QObject *parent) :
    QThread(parent)
{
    m_serialPortDescriptor = serialPortDescriptor;
    m_stopRequest = false;
}

void DataReceiveThread::run()
{
    qDebug() << "receiver thread started";
    while(!m_stopRequest)
    {
        char buf[1024];
        int n = ::read(m_serialPortDescriptor, buf, sizeof(buf));

        if(n > 0)
        {
            qDebug() << "receiver thread got " << n << " bytes";
            QByteArray dataBuffer = QByteArray::fromRawData(buf, n);
            //qDebug() << QString::fromLocal8Bit(dataBuffer);
            emit dataReceived(dataBuffer);
        }
    }
    qDebug() << "receiver thread exiting";
    // set stop request to false in case the thread gets re-started
    m_stopRequest = false;
}


void DataReceiveThread::stopRequest()
{
    m_stopRequest = true;
}
