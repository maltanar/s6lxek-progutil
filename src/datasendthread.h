#ifndef DATASENDTHREAD_H
#define DATASENDTHREAD_H

#include <QThread>
#include <QByteArray>

class DataSendThread : public QThread
{
    Q_OBJECT
public:
    explicit DataSendThread(int serialPortDescriptor, QByteArray dataToSend, QObject *parent = 0);
    
protected:
    int m_serialPortDescriptor;
    bool m_stopRequest;
    QByteArray m_dataToSend;
    
    void run();
    
signals:
    void progress(unsigned int completed, unsigned int total);
    void sendThreadFinished();
    
public slots:
    void stopRequest();
    
};

#endif // DATASENDTHREAD_H

