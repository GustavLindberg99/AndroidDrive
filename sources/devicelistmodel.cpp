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
            return this->createIndex(row, column, static_cast<QObject*>(this->_devices[row].get()));    //This needs to be manually cast to a QObject* before being cast to a void*, see https://stackoverflow.com/a/5445220/4284627
        }
    }

    //If the parent is a device, then the child is a drive
    AndroidDevice *parentDevice = this->indexToDevice(parent);
    if(parentDevice != nullptr){
        if(row >= 0 && row < parentDevice->numberOfDrives()){
            AndroidDrive *childDrive = parentDevice->driveAt(row);
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
        return this->deviceToIndex(this->parentDevice(drive));
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
                    if(drive->mountingInProgress()){
                        return QObject::tr("Mounting...");
                    }
                    else if(drive->unmountingInProgress()){
                        return QObject::tr("Unmounting...");
                    }
                    else{
                        return QObject::tr("Mounted as %1").arg(drive->mountPoint());
                    }
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
        const bool serialNumberExists = std::find_if(this->_devices.begin(), this->_devices.end(), [&newSerialNumber](const std::shared_ptr<const AndroidDevice> &device){
            return device->serialNumber() == newSerialNumber;
        }) != this->_devices.end();
        if(!serialNumberExists){
            const int index = this->_devices.size();
            this->beginInsertRows(this->rootIndex(), index, index);
            this->_devices.push_back(std::make_shared<AndroidDevice>(newSerialNumber));
            this->endInsertRows();
        }
    }

    //Remove devices that no longer exist
    for(int i = 0; i < this->_devices.size(); i++){
        const std::shared_ptr<AndroidDevice> oldDevice = this->_devices[i];
        if(!newSerialNumbers.contains(oldDevice->serialNumber())){
            this->beginRemoveRows(this->rootIndex(), i, i);
            this->_devices.removeAll(oldDevice);
            oldDevice->disconnectAllDrives();
            this->endRemoveRows();
            i--;
        }
    }

    //Update the list of offline serial numbers
    const QStringList allOfflineSerialNumbers = this->_offlineSerialNumbers.keys() + newOfflineSerialNumbers;
    for(const QString &offlineSerialNumber: allOfflineSerialNumbers){
        if(newOfflineSerialNumbers.contains(offlineSerialNumber)){
            this->_offlineSerialNumbers[offlineSerialNumber]++;
        }
        else{
            this->_offlineSerialNumbers.remove(offlineSerialNumber);
        }
    }
}

QList<std::shared_ptr<AndroidDevice>> DeviceListModel::devices() const{
    return this->_devices;
    /*QList<std::shared_ptr<AndroidDevice>> result;
    for(const auto& i: this->_devices) result.push_back(i.get());
    return result;*/
}

std::shared_ptr<AndroidDevice> DeviceListModel::parentDevice(const AndroidDrive * drive) const{
    const auto parentDevice = std::find_if(this->_devices.begin(), this->_devices.end(), [&drive](const std::shared_ptr<AndroidDevice> &device){
        return device->isParentOfDrive(drive);
    });
    return *parentDevice;
}

int DeviceListModel::timeSinceOffline(const QString &offlineSerialNumber) const{
    return this->_offlineSerialNumbers.value(offlineSerialNumber, 0);
}

QModelIndex DeviceListModel::rootIndex(int column) const{
    return this->createIndex(0, column, nullptr);
}

QModelIndex DeviceListModel::deviceToIndex(const std::shared_ptr<AndroidDevice> &device, int column) const{
    const int row = this->_devices.indexOf(device);
    /*int row = -1;
    for(int i = 0; i < this->_devices.length(); i++) if(this->_devices[i].get() == device) row = i;*/
    return this->createIndex(row, column, static_cast<QObject*>(device.get()));
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
