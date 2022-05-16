/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BURNJOB_H
#define BURNJOB_H

#include "dfmplugin_burn_global.h"
#include "dfm-base/interfaces/abstractjobhandler.h"

#include <dfm-burn/dopticaldiscmanager.h>

#include <QThread>

DPBURN_BEGIN_NAMESPACE

class AbstractBurnJob : public QThread
{
    Q_OBJECT

public:
    enum JobType {
        kOpticalBurn,
        kOpticalBlank,
        kOpticalImageBurn,
        kOpticalCheck
    };

    enum PropertyType {
        KStagingUrl,
        kImageUrl,
        kVolumeName,
        kSpeeds,
        kBurnOpts
    };

    enum JobPhase {
        kReady,
        kWriteData,
        kCheckData
    };

    using ProperyMap = QMap<PropertyType, QVariant>;

public:
    explicit AbstractBurnJob(const QString &dev, const JobHandlePointer handler);
    virtual ~AbstractBurnJob() override {}

    void setProperty(PropertyType type, const QVariant &val);

protected:
    virtual void updateMessage(JobInfoPointer ptr);
    virtual void readFunc(int progressFd, int checkFd);
    virtual void writeFunc(int progressFd, int checkFd);
    virtual void work() = 0;

    void run() override;
    bool readyToWork();
    void workingInSubProcess();
    DFMBURN::DOpticalDiscManager *createManager(int fd);
    QByteArray updatedInSubProcess(DFMBURN::JobStatus status, int progress, const QString &speed, const QStringList &message);
    void comfort();
    void deleteStagingFiles();

signals:
    void requestErrorMessageDialog(const QString &title, const QString &message);
    void requestFailureDialog(int type, const QString &err, const QStringList &details);
    void requestCompletionDialog(const QString &msg, const QString &icon);

public slots:
    void onJobUpdated(DFMBURN::JobStatus status, int progress, const QString &speed, const QStringList &message);

protected:
    QString curDev;
    QString curDevId;
    JobHandlePointer jobHandlePtr {};
    ProperyMap curProperty;
    JobType curJobType;
    int lastProgress {};
    int curPhase {};
    QString lastError;
    QStringList lastSrcMessages;
    DFMBURN::JobStatus lastStatus;
    bool jobSuccess {};   // delete staging files if sucess
};

class EraseJob : public AbstractBurnJob
{
    Q_OBJECT

public:
    explicit EraseJob(const QString &dev, const JobHandlePointer handler);
    virtual ~EraseJob() override {}

protected:
    virtual void updateMessage(JobInfoPointer ptr) override;
    virtual void work() override;
};

class BurnISOFilesJob : public AbstractBurnJob
{
    Q_OBJECT

public:
    explicit BurnISOFilesJob(const QString &dev, const JobHandlePointer handler);
    virtual ~BurnISOFilesJob() override {}

protected:
    virtual void writeFunc(int progressFd, int checkFd) override;
    virtual void work() override;
};

class BurnISOImageJob : public AbstractBurnJob
{
    Q_OBJECT

public:
    explicit BurnISOImageJob(const QString &dev, const JobHandlePointer handler);
    virtual ~BurnISOImageJob() override {}

protected:
    virtual void writeFunc(int progressFd, int checkFd) override;
    virtual void work() override;
};

class BurnUDFFilesJob : public AbstractBurnJob
{
    Q_OBJECT

public:
    explicit BurnUDFFilesJob(const QString &dev, const JobHandlePointer handler);
    virtual ~BurnUDFFilesJob() override {}

protected:
    virtual void writeFunc(int progressFd, int checkFd) override;
    virtual void work() override;
};

DPBURN_END_NAMESPACE
Q_DECLARE_METATYPE(DFMBURN::BurnOptions)

#endif   // BURNJOB_H
