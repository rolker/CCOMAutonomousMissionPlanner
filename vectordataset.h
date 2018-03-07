#ifndef VECTORDATASET_H
#define VECTORDATASET_H

#include "georeferenced.h"
#include "group.h"

class VectorDataset :public Group, public Georeferenced
{
    Q_OBJECT
    
public:
    VectorDataset(QObject *parent = 0);
    
    void write(QJsonObject &json) const;
    void read(const QJsonObject &json);
    
    void open(const QString &fname);
    
public slots:
    void updateProjectedPoints() override;
    
private:
    QString m_filename;
};

#endif // VECTORDATASET_H
