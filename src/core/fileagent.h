#ifndef FILEAGENT_H
#define FILEAGENT_H

#include "utils.h"

class FileAgent : public QObject
{
    Q_OBJECT
public:
    explicit FileAgent(QObject *parent = nullptr);

    void File_Open(QFile &f, int mode = 0);
    QJsonObject File_GetDataObject(QFile &File);
    QJsonDocument File_GetDataDocument(QFile &File);

signals:


};

#endif // FILEAGENT_H
