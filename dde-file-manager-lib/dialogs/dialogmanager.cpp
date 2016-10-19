#include "dialogmanager.h"
#include "closealldialogindicator.h"
#include "openwithotherdialog.h"
#include "trashpropertydialog.h"
#include "usersharepasswordsettingdialog.h"

#include "app/define.h"
#include "app/filesignalmanager.h"
#include "dfmevent.h"

#include "fileoperations/filejob.h"
#include "dfileservices.h"

#include "dfileinfo.h"
#include "models/trashfileinfo.h"

#include "views/windowmanager.h"

#include "shutil/iconprovider.h"

#include "xutil.h"
#include "utils.h"

#include "dialogs/dtaskdialog.h"
#include "dialogs/messagewrongdialog.h"
#include "dialogs/propertydialog.h"
#include "dialogs/openwithdialog.h"

#include "deviceinfo/udisklistener.h"
#include "deviceinfo/udiskdeviceinfo.h"

#include "widgets/singleton.h"

#include <ddialog.h>
#include <DAboutDialog>
#include <dscrollbar.h>

#include <QTimer>
#include <QDesktopWidget>
#include <QApplication>
#include <QScreen>

DWIDGET_USE_NAMESPACE

DialogManager::DialogManager(QObject *parent) : QObject(parent)
{
    initTaskDialog();
    initCloseIndicatorDialog();
    initConnect();
}

DialogManager::~DialogManager()
{

}

void DialogManager::initTaskDialog()
{
    m_taskDialog = new DTaskDialog;
    m_taskDialog->setWindowIcon(QIcon(":/images/images/dde-file-manager.svg"));
    m_taskDialog->setStyleSheet(getQssFromFile(":/qss/dialogs/qss/light.qss"));
    m_updateJobTaskTimer = new QTimer;
    m_updateJobTaskTimer->setInterval(1000);
    connect(m_updateJobTaskTimer, &QTimer::timeout, this, &DialogManager::updateJob);
}

void DialogManager::initCloseIndicatorDialog()
{
    m_closeIndicatorDialog = new CloseAllDialogIndicator;
    m_closeIndicatorDialog->setWindowIcon(QIcon(":/images/images/dde-file-manager.svg"));
    m_closeIndicatorDialog->setStyleSheet(getQssFromFile(":/qss/dialogs/qss/light.qss"));
    m_closeIndicatorTimer = new QTimer;
    m_closeIndicatorTimer->setInterval(100);
    connect(m_closeIndicatorTimer, &QTimer::timeout, this, &DialogManager::updateCloseIndicator);
}

void DialogManager::initConnect()
{
    connect(fileSignalManager, &FileSignalManager::requestStartUpdateJobTimer, this, &DialogManager::startUpdateJobTimer);
    connect(fileSignalManager, &FileSignalManager::requestStopUpdateJobTimer, this, &DialogManager::stopUpdateJobTimer);

    connect(fileSignalManager, &FileSignalManager::requestAbortJob, this, &DialogManager::abortJobByDestinationUrl);

    connect(m_taskDialog, &DTaskDialog::conflictRepsonseConfirmed, this, &DialogManager::handleConflictRepsonseConfirmed);
    connect(m_taskDialog, &DTaskDialog::closed, fileSignalManager, &FileSignalManager::requestQuitApplication);

    connect(m_taskDialog, &DTaskDialog::abortTask, this, &DialogManager::abortJob);
    connect(m_closeIndicatorDialog, &CloseAllDialogIndicator::allClosed, this, &DialogManager::closeAllPropertyDialog);

    connect(fileSignalManager, &FileSignalManager::requestShowUrlWrongDialog, this, &DialogManager::showUrlWrongDialog);
    connect(fileSignalManager, &FileSignalManager::requestShowOpenWithDialog, this, &DialogManager::showOpenWithDialog);
    connect(fileSignalManager, &FileSignalManager::requestShowPropertyDialog, this, &DialogManager::showPropertyDialog);
    connect(fileSignalManager, &FileSignalManager::requestShowTrashPropertyDialog, this, &DialogManager::showTrashPropertyDialog);
    connect(fileSignalManager, &FileSignalManager::requestShowDevicePropertyDialog, this, &DialogManager::showDevicePropertyDialog);
    connect(fileSignalManager, &FileSignalManager::showDiskErrorDialog,
            this, &DialogManager::showDiskErrorDialog);
    connect(fileSignalManager, &FileSignalManager::showAboutDialog,
            this, &DialogManager::showAboutDialog);

//    connect(qApp, &QApplication::focusChanged, this, &DialogManager::handleFocusChanged);

}

QPoint DialogManager::getPerportyPos(int dialogWidth, int dialogHeight, int count, int index)
{
    const QScreen *cursor_screen = Q_NULLPTR;
    const QPoint &cursor_pos = QCursor::pos();

    for (const QScreen *screen : qApp->screens()) {
        if (screen->geometry().contains(cursor_pos)) {
            cursor_screen = screen;
            break;
        }
    }

    if (!cursor_screen)
        cursor_screen = qApp->primaryScreen();

    int desktopWidth = cursor_screen->size().width();
    int desktopHeight = cursor_screen->size().height();
    int SpaceWidth = 20;
    int SpaceHeight = 100;
    int row, x , y;

    int numberPerRow =  desktopWidth / (dialogWidth + SpaceWidth);


    if (count % numberPerRow == 0){
        row = count / numberPerRow;

    }else{
        row = count / numberPerRow + 1;

    }

    int dialogsWidth;
    if (count / numberPerRow > 0){
        dialogsWidth = dialogWidth * numberPerRow + SpaceWidth * (numberPerRow - 1);
    }else{
        dialogsWidth = dialogWidth * (count % numberPerRow)  + SpaceWidth * (count % numberPerRow - 1);
    }

    int dialogsHeight = dialogHeight + SpaceHeight * (row - 1);

    x =  (desktopWidth - dialogsWidth) / 2 + (dialogWidth + SpaceWidth) * (index % numberPerRow);

    y = (desktopHeight - dialogsHeight) / 2 + (index / numberPerRow) * SpaceHeight;

    return QPoint(x, y) + cursor_screen->geometry().topLeft();
}

void DialogManager::addJob(FileJob *job)
{
    m_jobs.insert(job->getJobId(), job);
    emit fileSignalManager->requestStartUpdateJobTimer();
    connect(job, &FileJob::requestJobAdded, m_taskDialog, &DTaskDialog::addTask);
    connect(job, &FileJob::requestJobRemoved, m_taskDialog, &DTaskDialog::delayRemoveTask);
    connect(job, &FileJob::requestJobDataUpdated, m_taskDialog, &DTaskDialog::handleUpdateTaskWidget);
    connect(job, &FileJob::requestAbortTask, m_taskDialog, &DTaskDialog::abortTask);
    connect(job, &FileJob::requestConflictDialogShowed, m_taskDialog, &DTaskDialog::showConflictDiloagByJob);
}


void DialogManager::removeJob(const QString &jobId)
{
    m_jobs.remove(jobId);
    if (m_jobs.count() == 0){
        emit fileSignalManager->requestStopUpdateJobTimer();
    }
}

void DialogManager::updateJob()
{
    foreach (QString jobId, m_jobs.keys()) {
        FileJob* job = m_jobs.value(jobId);
        if (job->currentMsec() - job->lastMsec() > FileJob::Msec_For_Display){
            if (!job->isJobAdded()){
                job->jobAdded();
                job->jobUpdated();
            }else{
                job->jobUpdated();
            }
        }
    }
}

void DialogManager::startUpdateJobTimer()
{
    m_updateJobTaskTimer->start();
}

void DialogManager::stopUpdateJobTimer()
{
    m_updateJobTaskTimer->stop();
}

void DialogManager::abortJob(const QMap<QString, QString> &jobDetail)
{
    QString jobId = jobDetail.value("jobId");
    FileJob * job = m_jobs.value(jobId);
    if (job){
        job->setApplyToAll(true);
        job->setStatus(FileJob::Cancelled);
    }
}

void DialogManager::abortJobByDestinationUrl(const DUrl &url)
{
    qDebug() << url;
    foreach (QString jobId, m_jobs.keys()) {
        FileJob* job = m_jobs.value(jobId);
        qDebug() << jobId << job->getTargetDir();
        if (job->getTargetDir().startsWith(url.path())){
            job->jobAborted();
        }
    }
}

void DialogManager::showUrlWrongDialog(const DUrl &url)
{
    MessageWrongDialog d(url.toString());
    qDebug() << url;
    d.exec();
}

int DialogManager::showRunExcutableDialog(const DUrl &url)
{
    QString fileDisplayName = QFileInfo(url.path()).fileName();
    QString message = tr("Do you wan to run %1 or display its content?").arg(fileDisplayName);
    QString tipMessage = tr("It is an executable text file.");
    QStringList buttonKeys, buttonTexts;
    buttonKeys << "OptionCancel" << "OptionRun" << "OptionRunInTerminal" << "OptionDisplay";
    buttonTexts << tr("Cancel") << tr("Run") << tr("Run in terminal") << tr("Display");
    DDialog d;
#ifdef ARCH_MIPSEL
    d.setIcon(QIcon(svgToPixmap(":/images/images/application-x-shellscript.svg", 64, 64)));
#else
    d.setIcon(fileIconProvider->getDesktopIcon("application-x-shellscript", 256));
#endif
    d.setTitle(message);
    d.setMessage(tipMessage);
    d.addButtons(buttonTexts);
    d.setDefaultButton(2);
    d.setFixedWidth(480);
    int code = d.exec();
    return code;
}


int DialogManager::showRenameNameSameErrorDialog(const QString &name, const DFMEvent &event)
{
    DDialog d(WindowManager::getWindowById(event.windowId()));
    d.setTitle(tr("\"%1\" already exists, please use another name.").arg(name));
    QStringList buttonTexts;
    buttonTexts << tr("Confirm");
    d.addButtons(buttonTexts);
    d.setDefaultButton(0);
    d.setIcon(QIcon(":/images/dialogs/images/dialog_warning_64.png"));
    int code = d.exec();
    return code;
}


int DialogManager::showDeleteFilesClearTrashDialog(const DFMEvent &event)
{
    QString ClearTrash = tr("Are you sure to empty %1 item?");
    QString ClearTrashMutliple = tr("Are you sure to empty %1 items?");
    QString DeleteFileName = tr("Permanently delete %1?");
    QString DeleteFileItems = tr("Permanently delete %1 items?");

    DUrlList urlList = event.fileUrlList();
    QStringList buttonTexts;
    buttonTexts << tr("Cancel") << tr("Delete");

    qDebug() << event;
    DDialog d(WindowManager::getWindowById(event.windowId()));
    d.setIcon(QIcon(":/images/dialogs/images/user-trash-full-opened.png"));
    if (urlList.first() == DUrl::fromTrashFile("/") && event.source() == DFMEvent::Menu && urlList.size() == 1){
        buttonTexts[1]= tr("Empty");
        const DAbstractFileInfoPointer &fileInfo = DFileService::instance()->createFileInfo(urlList.first());
        if(fileInfo->filesCount() == 1)
            d.setTitle(ClearTrash.arg(fileInfo->filesCount()));
        else
            d.setTitle(ClearTrashMutliple.arg(fileInfo->filesCount()));
    }else if (urlList.first().isLocalFile() && event.source() == DFMEvent::FileView && urlList.size() == 1){
        DFileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.fileDisplayName()));
    }else if (urlList.first().isLocalFile() && event.source() == DFMEvent::FileView && urlList.size() > 1){
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }else if (urlList.first().isTrashFile() && event.source() == DFMEvent::Menu && urlList.size() == 1){
        TrashFileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.fileDisplayName()));
    }else if (urlList.first().isTrashFile() && event.source() == DFMEvent::Menu && urlList.size() > 1){
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }else if (urlList.first().isTrashFile() && event.source() == DFMEvent::FileView && urlList.size() == 1 ){
        TrashFileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.fileDisplayName()));
    }else if (urlList.first().isTrashFile() && event.source() == DFMEvent::FileView && urlList.size() > 1 ){
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }else{
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }
    d.setMaximumWidth(480);
    d.setMessage(tr("This action cannot be restored"));
    d.addButtons(buttonTexts);
    d.setDefaultButton(1);
    int code = d.exec();
    return code;
}

int DialogManager::showRemoveBookMarkDialog(const DFMEvent &event)
{
    DDialog d(WindowManager::getWindowById(event.windowId()));
    d.setTitle(tr("Sorry, unable to locate your bookmark directory, remove it?"));
    QStringList buttonTexts;
    buttonTexts << tr("Cancel") << tr("Remove");
    d.addButtons(buttonTexts);
    d.setDefaultButton(1);
    d.setIcon(fileIconProvider->getDesktopIcon("folder", 64));
    int code = d.exec();
    return code;
}

void DialogManager::showOpenWithDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        OpenWithOtherDialog* d = new OpenWithOtherDialog(event.fileUrl(), w);
        d->setDisplayPostion(OpenWithOtherDialog::DisplayCenter);
        d->exec();
    }
}

void DialogManager::showPropertyDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        const DUrlList& urlList  = event.fileUrlList();
        int count = urlList.count();
        foreach (const DUrl& url, urlList) {
            int index = urlList.indexOf(url);
            PropertyDialog *dialog;
            if (m_propertyDialogs.contains(url)){
                dialog = m_propertyDialogs.value(url);
                dialog->raise();
            }else{
                dialog = new PropertyDialog(url);
                m_propertyDialogs.insert(url, dialog);
                QPoint pos = getPerportyPos(dialog->size().width(), dialog->size().height(), count, index);

                dialog->show();
                dialog->move(pos);

                connect(dialog, &PropertyDialog::closed, this, &DialogManager::removePropertyDialog);
//                connect(dialog, &PropertyDialog::raised, this, &DialogManager::raiseAllPropertyDialog);
                QTimer::singleShot(100, dialog, &PropertyDialog::raise);
            }
        }

        if (urlList.count() >= 2){
            m_closeIndicatorDialog->show();
            m_closeIndicatorTimer->start();
        }
    }
}

void DialogManager::showTrashPropertyDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());

    if (w) {
        if (m_trashDialog){
            m_trashDialog->close();
        }
        m_trashDialog = new TrashPropertyDialog(event.fileUrl());
        connect(m_trashDialog, &TrashPropertyDialog::closed, [=](){
               m_trashDialog = NULL;
        });
        QPoint pos = getPerportyPos(m_trashDialog->size().width(), m_trashDialog->size().height(), 1, 0);
        m_trashDialog->show();
        m_trashDialog->move(pos);

        TIMER_SINGLESHOT(100, {
                             m_trashDialog->raise();
                         }, this)
    }
}

void DialogManager::showDevicePropertyDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        PropertyDialog* dialog = new PropertyDialog(event.fileUrl());
        dialog->show();
    }
}

void DialogManager::showDiskErrorDialog(const QString & id, const QString & errorText)
{
    Q_UNUSED(errorText)

    UDiskDeviceInfo* info = deviceListener->getDevice(id);
    if (info){
        DDialog d;
        d.setTitle(tr("Disk file is being used, can not unmount now"));
        QStringList buttonTexts;
        buttonTexts << tr("Cancel") << tr("Force unmount");
        d.addButtons(buttonTexts);
        d.setDefaultButton(0);
        d.setIcon(info->fileIcon(64, 64));
        int code = d.exec();
        if (code == 1){
            deviceListener->forceUnmount(id);
        }
    }
}

void DialogManager::showBreakSymlinkDialog(const QString &targetName, const DUrl &linkfile)
{
    const DAbstractFileInfoPointer &fileInfo = DFileService::instance()->createFileInfo(linkfile);

    DDialog d;
    d.setTitle(tr("%1 that this shortcut refers to has been changed or moved").arg(targetName));
    d.setMessage(tr("Do you want to delete this shortcut？"));
    QStringList buttonTexts;
    buttonTexts << tr("Cancel") << tr("Confirm");
    d.addButtons(buttonTexts);
    d.setDefaultButton(1);
    d.setIcon(fileInfo->fileIcon().pixmap(64, 64));
    int code = d.exec();
    if (code == 1){
        DUrlList urls;
        urls << linkfile;
        fileService->moveToTrash(urls);
    }
}

void DialogManager::showAboutDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    QString icon(":/images/images/dde-file-manager_96.png");
    DAboutDialog *dialog = new DAboutDialog(icon,
                        icon,
                        qApp->applicationDisplayName(),
                        tr("Version:") + qApp->applicationVersion(),
                        tr("File Manager is a file management tool independently "
                           "developed by Deepin Technology, featured with searching, "
                           "copying, trash, compression/decompression, file property "
                           "and other file management functions. "),w);
    dialog->setTitle("");
    const QPoint global = w->mapToGlobal(w->rect().center());
    dialog->move(global.x() - dialog->width() / 2, global.y() - dialog->height() / 2);
    dialog->show();
}

void DialogManager::showUserSharePasswordSettingDialog(const DFMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    UserSharePasswordSettingDialog *dialog = new UserSharePasswordSettingDialog(w);
    int code = dialog->exec();
    dialog->onButtonClicked(code);
}

void DialogManager::removePropertyDialog(const DUrl &url)
{
    if (m_propertyDialogs.contains(url)){
        m_propertyDialogs.remove(url);
    }
    if (m_propertyDialogs.count() == 0){
        m_closeIndicatorDialog->hide();
    }
}

void DialogManager::closeAllPropertyDialog()
{
    foreach (const DUrl& url, m_propertyDialogs.keys()) {
        m_propertyDialogs.value(url)->close();
    }
    if (m_closeIndicatorDialog){
        m_closeIndicatorTimer->stop();
        m_closeIndicatorDialog->close();
    }
    if (m_trashDialog){
        m_trashDialog->close();
    }
}

void DialogManager::updateCloseIndicator()
{
    qint64 size = 0;
    int fileCount = 0;
    foreach (PropertyDialog* d, m_propertyDialogs.values()) {
        size += d->getFileSize();
        fileCount += d->getFileCount();
    }
    m_closeIndicatorDialog->setTotalMessage(size, fileCount);
}

void DialogManager::raiseAllPropertyDialog()
{
    foreach (PropertyDialog* d, m_propertyDialogs.values()) {
        qDebug() << d->getUrl() << d->isVisible() << d->windowState();
//        d->showMinimized();
        d->showNormal();
        QtX11::utils::ShowNormalWindow(d);
        qobject_cast<QWidget*>(d)->showNormal();
        d->show();
        d->raise();
        qDebug() << d->getUrl() << d->isVisible() << d->windowState();
    }
    m_closeIndicatorDialog->raise();
}

void DialogManager::handleFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)
    Q_UNUSED(now)

    if (m_propertyDialogs.values().contains(qobject_cast<PropertyDialog*>(qApp->activeWindow()))){
        raiseAllPropertyDialog();
    }else if(m_closeIndicatorDialog == qobject_cast<CloseAllDialogIndicator*>(qApp->activeWindow())){
        raiseAllPropertyDialog();
    }
}

void DialogManager::refreshPropertyDialogs(const DUrl &oldUrl, const DUrl &newUrl)
{
    PropertyDialog* d = m_propertyDialogs.value(oldUrl);
    if (d){
        m_propertyDialogs.remove(oldUrl);
        m_propertyDialogs.insert(newUrl, d);
    }
}

void DialogManager::handleConflictRepsonseConfirmed(const QMap<QString, QString> &jobDetail, const QMap<QString, QVariant> &response)
{
    QString jobId = jobDetail.value("jobId");
    FileJob * job = m_jobs.value(jobId);
    if(job != NULL)
    {
        bool applyToAll = response.value("applyToAll").toBool();
        int code = response.value("code").toInt();
        job->setApplyToAll(applyToAll);
        //0 = coexist, 1 = replace, 2 = skip
        switch(code)
        {
        case 0:job->started();break;
        case 1:
            job->started();
            job->setReplace(true);
            break;
        case 2:job->cancelled();break;
        default:
            qDebug() << "unknown code"<<code;
        }
    }
}
