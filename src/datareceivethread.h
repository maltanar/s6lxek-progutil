#ifndef DATARECEIVETHREAD_H
#define DATARECEIVETHREAD_H

#include <QThread>

class DataReceiveThread : public QThread
{
    Q_OBJECT
public:
    explicit DataReceiveThread(int serialPortDescriptor, QObject *parent = 0);

protected:
    int m_serialPortDescriptor;
    bool m_stopRequest;

    void run();
    
signals:
    void dataReceived(QByteArray data);
    
public slots:
    void stopRequest();
    
};

#endif // DATARECEIVETHREAD_H
