#include "devicelistmodel.h"

#include <QIcon>

#include <algorithm>

#include "androiddrive.h"

QModelIndex DeviceListModel::index(int row, int column, const QModelIndex &parent) const{
    if(!this->hasIndex(row, column, parent)){
        return QModelIndex();
    }

    //If the parent is the root item (represented by a null pointer), then the child is a device
    if(this->indexIsRoot(parent)){
        if(row >= 0 && row < this->_devices.size()){
            return this->createIndex(row, column, static_cast<QObject*>(this->_devices[row]));    //This needs to be manually cast to a QObject* before being cast to a void*, see https://stackoverflow.com/a/5445220/4284627
        }
    }

    //If the parent is a device, then the child is a drive
    AndroidDevice *parentDevice = this->indexToDevice(parent);
    if(parentDevice != nullptr){
        if(row >= 0 && row < parentDevice->numberOfDrives()){
            AndroidDrive *childDrive = parentDevice->drives().at(row);    //Use .at(row) instead of [row] for performance reasons: https://stackoverflow.com/a/48881232/4284627
            return this->createIndex(row, column, static_cast<QObject*>(childDrive));
        }
    }

    //This will be run either if the parent is neither a device or a root item, meaning it's a drive, or if the child is out of bounds.
    //In both cases, return an invalid index, since drives don't have any children.
    return QModelIndex();
}

QModelIndex DeviceListModel::parent(const QModelIndex &index) const{
    QObject *item = nullptr;
    if(index.isValid()){
        item = static_cast<QObject*>(index.internalPointer());
    }

    AndroidDevice *device = dynamic_cast<AndroidDevice*>(item);
    AndroidDrive *drive = dynamic_cast<AndroidDrive*>(item);

    //If the item is a drive, the parent is a device
    if(drive != nullptr){
        return this->deviceToIndex(drive->device());
    }

    //If the item is a device, the parent is the root item
    else if(device != nullptr){
        return this->rootIndex();
    }

    //If the item is neither a drive nor a device, it's the root item which doesn't have any parent
    return QModelIndex();
}

int DeviceListModel::rowCount(const QModelIndex &parent) const{
    //If the parent is the root, it has a number of children equal to the number of devices
    if(this->indexIsRoot(parent)){
        return this->_devices.size();
    }

    //If the parent is a device, it has a number of children equal to the number of drives for that device
    AndroidDevice *parentDevice = this->indexToDevice(parent);
    if(parentDevice != nullptr){
        return parentDevice->numberOfDrives();
    }

    //Otherwise the parent is a drive, in which case it has no children
    return 0;
}

int DeviceListModel::columnCount(const QModelIndex&) const{
    return 2;
}

QVariant DeviceListModel::data(const QModelIndex &index, int role) const{
    if(!index.isValid()){
        return QVariant();
    }

    AndroidDevice *device = this->indexToDevice(index);
    AndroidDrive *drive = this->indexToDrive(index);
    switch(index.column()){
    case 0:    //Column with the name of the device/drive
        switch(role){
        case Qt::DisplayRole:    //Text
            if(this->indexIsRoot(index)){
                return QObject::tr("Device");
            }
            else if(device != nullptr){
                return device->model();
            }
            else if(drive != nullptr){
                return drive->name();
            }
            break;

        case Qt::DecorationRole:    //Icon
            if(device != nullptr){
                return QIcon(":/android.svg");
            }
            else if(drive != nullptr){
                return QIcon(":/drive.svg");
            }
            break;
        }
        break;

    case 1:    //Status column
        if(role == Qt::DisplayRole){
            if(this->indexIsRoot(index)){
                return QObject::tr("Status");
            }
            else if(drive != nullptr){
                if(drive->isConnected()){
                    return QObject::tr("Mounted as %1").arg(drive->mountPoint());
                }
                else{
                    return QObject::tr("Not mounted");
                }
            }
        }
        break;
    }

    return QVariant();
}

QVariant DeviceListModel::headerData(int section, Qt::Orientation orientation, int role) const{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole){
        return this->data(this->rootIndex(section), role);
    }
    return QVariant();
}

Qt::ItemFlags DeviceListModel::flags(const QModelIndex &index) const{
    if(!index.isValid()){
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags result = QAbstractItemModel::flags(index);
    if(this->indexToDrive(index) != nullptr){
        result |= Qt::ItemNeverHasChildren;
    }
    return result;
}

void DeviceListModel::updateDevices(const QStringList &newSerialNumbers, const QStringList &newOfflineSerialNumbers){
    //Add new devices
    for(const QString &newSerialNumber: newSerialNumbers){
        if(std::find_if(this->_devices.begin(), this->_devices.end(), [&newSerialNumber](const AndroidDevice *device){return device->serialNumber() == newSerialNumber;}) == this->_devices.end()){
            const int index = this->_devices.size();
            this->beginInsertRows(this->rootIndex(), index, index);
            AndroidDevice *device = new AndroidDevice(newSerialNumber);
            this->_devices.append(device);
            this->endInsertRows();
        }
    }

    //Add new offline devices
    for(const QString &newOfflineSerialNumber: newOfflineSerialNumbers){
        if(!this->_offlineSerialNumbers.contains(newOfflineSerialNumber)){
            this->_offlineSerialNumbers.append(newOfflineSerialNumber);
        }
    }

    //Remove devices that no longer exist
    for(int i = 0; i < this->_devices.size(); i++){
        AndroidDevice *oldDevice = this->_devices[i];
        if(!newSerialNumbers.contains(oldDevice->serialNumber())){
            this->beginRemoveRows(this->rootIndex(), i, i);
            this->_devices.removeAll(oldDevice);
            oldDevice->shutdown();
            this->endRemoveRows();
            i--;
        }
    }

    //Remove offline devices that no longer exist
    const QStringList oldOfflineSerialNumbers = this->_offlineSerialNumbers;
    for(const QString &oldOfflineSerialNumber: oldOfflineSerialNumbers){
        if(!newOfflineSerialNumbers.contains(oldOfflineSerialNumber)){
            this->_offlineSerialNumbers.removeAll(oldOfflineSerialNumber);
        }
    }
}

QList<AndroidDevice*> DeviceListModel::devices() const{
    return this->_devices;
}

QStringList DeviceListModel::offlineSerialNumbers() const{
    return this->_offlineSerialNumbers;
}

QModelIndex DeviceListModel::rootIndex(int column) const{
    return this->createIndex(0, column, nullptr);
}

QModelIndex DeviceListModel::deviceToIndex(AndroidDevice *device, int column) const{
    const int row = this->_devices.indexOf(device);
    return this->createIndex(row, column, static_cast<QObject*>(device));
}

bool DeviceListModel::indexIsRoot(const QModelIndex &index) const{
    return this->indexToDevice(index) == nullptr && this->indexToDrive(index) == nullptr;
}

AndroidDevice *DeviceListModel::indexToDevice(const QModelIndex &index) const{
    if(!index.isValid()){
        return nullptr;
    }
    QObject *object = static_cast<QObject*>(index.internalPointer());
    return dynamic_cast<AndroidDevice*>(object);
}

AndroidDrive *DeviceListModel::indexToDrive(const QModelIndex &index) const{
    if(!index.isValid()){
        return nullptr;
    }
    QObject *object = static_cast<QObject*>(index.internalPointer());
    return dynamic_cast<AndroidDrive*>(object);
}
