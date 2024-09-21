#ifndef DEVICELISTMODEL_H
#define DEVICELISTMODEL_H

#include <QAbstractItemModel>

#include "androiddevice.h"

/**
 * This model is a tree model with three levels: the root level (which contains titles for the columns), devices and drives.
 * The QModelIndex::internalPointer() returns a pointer to the object at that index.
 * For the root level, this pointer is always null.
 * For the device level, this is a pointer to the corresponding AndroidDevice object.
 * For the drive level, this is a pointer to the corresponding AndroidDrive object.
 */
class DeviceListModel : public QAbstractItemModel {
    Q_OBJECT

public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;    //Returns the text that will be displayed
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
     * Update the list of devices.
     *
     * @param newSerialNumbers - The serial numbers of the devices that are connected and work correctly (including those that already were connected).
     * @param newOfflineSerialNumbers - The serial numbers of offline devices (including those that already were offline).
     */
    void updateDevices(const QStringList &newSerialNumbers, const QStringList &newOfflineSerialNumbers);

    /**
     * Gets a list of all connected devices.
     *
     * @return All devices that are connected (not including offline devices).
     */
    QList<std::shared_ptr<AndroidDevice>> devices() const;

    /**
     * Gets the parent device of a given drive.
     *
     * @param drive - The drive to get the parent of.
     *
     * @return The parent device.
     */
    std::shared_ptr<AndroidDevice> parentDevice(const AndroidDrive *drive) const;

    /**
     * Gets the number of times ADB has run since the given device has been offline.
     *
     * @param offlineSerialNumber - The serial number of the device to check.
     *
     * @return The number of times ADB has run since the device has been offline, or 0 if it's not offline or if it's disconnected.
     */
    int timeSinceOffline(const QString &offlineSerialNumber) const;

    /**
     * Gets the root QModelIndex in the tree structure, i.e. the one containing all devices.
     *
     * @param column - The column to get the index at.
     *
     * @return The root index.
     */
    QModelIndex rootIndex(int column = 0) const;

    /**
     * @brief Gets the QModelIndex corresponding to a specific device.
     *
     * @param device - The device to get the index of.
     * @param column - The column of the index.
     *
     * @return The QModelIndex corresponding to the given device.
     */
    QModelIndex deviceToIndex(const std::shared_ptr<AndroidDevice> &device, int column = 0) const;

    /**
     * Checks if a given index is the root index, i.e. the one containing all the devices.
     *
     * @param index - The index to check if it's the root index.
     *
     * @return True if it's the root index, false if it isn't.
     */
    bool indexIsRoot(const QModelIndex &index) const;

    /**
     * Gets the device corresponding to the given QModelIndex.
     *
     * @param index - The index to convert to a device.
     *
     * @return A non-owning pointer to the corresponding device, or nullptr if this index doesn't correspond to a device.
     */
    AndroidDevice *indexToDevice(const QModelIndex &index) const;

    /**
     * Gets the drive corresponding to the given QModelIndex.
     *
     * @param index - The index to convert to a drive.
     *
     * @return A non-owning pointer to the corresponding drive, or nullptr if this index doesn't correspond to a drive.
     */
    AndroidDrive *indexToDrive(const QModelIndex &index) const;

private:
    QList<std::shared_ptr<AndroidDevice>> _devices;
    QMap<QString, int> _offlineSerialNumbers;
};

#endif // DEVICELISTMODEL_H
