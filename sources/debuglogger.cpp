#include "debuglogger.hpp"

#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <source_location>

#include "version.h"

DebugLogger &DebugLogger::getInstance(){
    static DebugLogger instance;
    return instance;
}

bool DebugLogger::start(){
    const QString parentFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(parentFolder).mkpath(".");
    const QString path = parentFolder + "/AndroidDriveDebugLog" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".log";
    this->_file = std::make_unique<QFile>(path);
    if(!this->_file->open(QFile::WriteOnly)){
        this->_file = nullptr;
        return false;
    }
    this->_file->write("Log captured with AndroidDrive version " PROGRAMVERSION "\n");
    return true;
}

void DebugLogger::stop(){
    if(this->_file != nullptr){
        QProcess::startDetached("explorer.exe", {"/select," + this->logFilePath()});
    }
    this->_file = nullptr;
}

void DebugLogger::log(const QString &message, std::source_location location){
    if(this->_file == nullptr){
        return;
    }

    this->_file->write(QString("%1 %2:%3 %4\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
        .arg(location.file_name())
        .arg(location.line())
        .arg(message)
        .toUtf8()
    );
    this->_file->flush();    //Very important so that it's possible to debug crashes
}

bool DebugLogger::isRecording() const{
    return this->_file != nullptr;
}

QString DebugLogger::logFilePath() const{
    if(this->_file == nullptr){
        return "";
    }
    return QDir::toNativeSeparators(this->_file->fileName());
}
