#ifndef DEVICELISTMODEL_H
#define DEVICELISTMODEL_H

#include <QAbstractItemModel>

#include "androiddevice.h"

//This model is a tree model with three levels: the root level (which contains titles for the columns), devices and drives.
//The QModelIndex::internalPointer() returns a pointer to the object at that index.
//For the root level, this pointer is always null.
//For the device level, this is a pointer to the corresponding AndroidDevice object.
//For the drive level, this is a pointer to the corresponding AndroidDrive object.
class DeviceListModel : public QAbstractItemModel{
    Q_OBJECT

public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;    //Returns the text that will be displayed
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void updateDevices(const QStringList &newSerialNumbers, const QStringList &newOfflineSerialNumbers);
    QList<AndroidDevice*> devices() const;
    QStringList offlineSerialNumbers() const;

    QModelIndex rootIndex(int column = 0) const;
    QModelIndex deviceToIndex(AndroidDevice *device, int column = 0) const;

    bool indexIsRoot(const QModelIndex &index) const;
    AndroidDevice *indexToDevice(const QModelIndex &index) const;
    AndroidDrive *indexToDrive(const QModelIndex &index) const;

private:
    QList<AndroidDevice*> _devices;
    QStringList _offlineSerialNumbers;
};

#endif // DEVICELISTMODEL_H
