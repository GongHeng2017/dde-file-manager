#include "dialogmanager.h"
#include <QTimer>
#include "dialogs/dtaskdialog.h"
#include "../app/global.h"
#include "../app/filesignalmanager.h"
#include "../controllers/filejob.h"
#include "dialogs/messagewrongdialog.h"
#include "../dialogs/propertydialog.h"
#include "../dialogs/openwithdialog.h"
#include "../models/fileinfo.h"
#include "../app/fmevent.h"
#include "../views/windowmanager.h"
#include "../models/trashfileinfo.h"
#include "closealldialogindicator.h"
#include "utils/xutil.h"
#include "openwithotherdialog.h"
#include "trashpropertydialog.h"
#include <ddialog.h>
#include <DAboutDialog>
#include <dscrollbar.h>
#include <QDesktopWidget>

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
    m_taskDialog->setWindowIcon(QIcon(":/images/images/system-file-manager.png"));
    m_taskDialog->setStyleSheet(getQssFromFile(":/qss/dialogs/qss/light.qss"));
}

void DialogManager::initCloseIndicatorDialog()
{
    m_closeIndicatorDialog = new CloseAllDialogIndicator;
    m_closeIndicatorDialog->setWindowIcon(QIcon(":/images/images/system-file-manager.png"));
    m_closeIndicatorDialog->setStyleSheet(getQssFromFile(":/qss/dialogs/qss/light.qss"));
    m_closeIndicatorTimer = new QTimer;
    m_closeIndicatorTimer->setInterval(100);
    connect(m_closeIndicatorTimer, &QTimer::timeout, this, &DialogManager::updateCloseIndicator);
}

void DialogManager::initConnect()
{
    connect(fileSignalManager, &FileSignalManager::jobAdded, m_taskDialog, &DTaskDialog::addTask);
    connect(fileSignalManager, &FileSignalManager::jobRemoved, m_taskDialog, &DTaskDialog::delayRemoveTask);
    connect(fileSignalManager, &FileSignalManager::jobDataUpdated, m_taskDialog, &DTaskDialog::handleUpdateTaskWidget);
    connect(m_taskDialog, &DTaskDialog::abortTask, fileSignalManager, &FileSignalManager::abortTask);

    connect(fileSignalManager, &FileSignalManager::conflictDialogShowed, m_taskDialog, &DTaskDialog::showConflictDiloagByJob);
    connect(m_taskDialog, &DTaskDialog::conflictShowed, fileSignalManager, &FileSignalManager::conflictTimerStoped);
    connect(m_taskDialog, &DTaskDialog::conflictHided, fileSignalManager, &FileSignalManager::conflictTimerReStarted);
    connect(m_taskDialog, &DTaskDialog::conflictRepsonseConfirmed, fileSignalManager, &FileSignalManager::conflictRepsonseConfirmed);
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
    int desktopWidth = qApp->desktop()->geometry().width();
    int desktopHeight = qApp->desktop()->geometry().height();
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

    qDebug() << desktopHeight <<  dialogsHeight << dialogHeight << y;

    return QPoint(x, y);

}

void DialogManager::addJob(FileJob *job)
{
    m_jobs.insert(job->getJobId(), job);
}


void DialogManager::removeJob(const QString &jobId)
{
    m_jobs.remove(jobId);
}

void DialogManager::showTaskDialog()
{
    m_taskDialog->show();
}


void DialogManager::abortJob(const QMap<QString, QString> &jobDetail)
{
    QString jobId = jobDetail.value("jobId");
    FileJob * job = m_jobs.value(jobId);
    job->setApplyToAll(true);
    job->setStatus(FileJob::Cancelled);
}


void DialogManager::showUrlWrongDialog(const DUrl &url)
{
    MessageWrongDialog d(url.toString());
    qDebug() << url;
    d.exec();
}


int DialogManager::showRenameNameSameErrorDialog(const QString &name, const FMEvent &event)
{
    DDialog d(WindowManager::getWindowById(event.windowId()));;
    d.setTitle(tr("\"%1\" already exists, please select a different name.").arg(name));
    QStringList buttonTexts;
    buttonTexts << tr("Confirm");
    d.addButtons(buttonTexts);
    d.setDefaultButton(0);
    d.setIcon(QIcon(":/images/dialogs/images/dialog_warning_64.png"));
    int code = d.exec();
    return code;
}


int DialogManager::showDeleteFilesClearTrashDialog(const FMEvent &event)
{
    QString ClearTrash = tr("Are you sure to empty trash?");
    QString DeleteFileName = tr("Are you sure to delete %1?");
    QString DeleteFileItems = tr("Are you sure to delete %1 items?");

    DUrlList urlList = event.fileUrlList();
    QStringList buttonTexts;
    buttonTexts << tr("Cancel") << tr("Delete");

    qDebug() << event;
    DDialog d(WindowManager::getWindowById(event.windowId()));
    d.setIcon(QIcon(":/images/dialogs/images/user-trash-full-opened.png"));
    if (urlList.first() == DUrl::fromTrashFile("/") && event.source() == FMEvent::Menu && urlList.size() == 1){
        buttonTexts[1]= tr("Empty");
        d.setTitle(ClearTrash);
    }else if (urlList.first().isLocalFile() && event.source() == FMEvent::FileView && urlList.size() == 1){
        FileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.displayName()));
    }else if (urlList.first().isLocalFile() && event.source() == FMEvent::FileView && urlList.size() > 1){
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }else if (urlList.first().isTrashFile() && event.source() == FMEvent::Menu && urlList.size() == 1){
        TrashFileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.displayName()));
    }else if (urlList.first().isTrashFile() && event.source() == FMEvent::Menu && urlList.size() > 1){
        d.setTitle(DeleteFileItems.arg(urlList.size()));
    }else if (urlList.first().isTrashFile() && event.source() == FMEvent::FileView && urlList.size() == 1 ){
        TrashFileInfo f(urlList.first());
        d.setTitle(DeleteFileName.arg(f.displayName()));
    }else if (urlList.first().isTrashFile() && event.source() == FMEvent::FileView && urlList.size() > 1 ){
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

int DialogManager::showRemoveBookMarkDialog(const FMEvent &event)
{
    DDialog d(WindowManager::getWindowById(event.windowId()));
    d.setTitle(tr("Sorry, unable to locate your bookmark directory, remove it?"));
    QStringList buttonTexts;
    buttonTexts << tr("Cancel") << tr("Remove bookmark");
    d.addButtons(buttonTexts);
    d.setDefaultButton(1);
    d.setIcon(fileIconProvider->getDesktopIcon("folder", 64));
    int code = d.exec();
    return code;
}

void DialogManager::showOpenWithDialog(const FMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        OpenWithOtherDialog* d = new OpenWithOtherDialog(event.fileUrl());
        d->exec();
    }
}

void DialogManager::showPropertyDialog(const FMEvent &event)
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
//                QTimer::singleShot(100, dialog, &PropertyDialog::raise);
                dialog->raise();
            }
        }

        if (urlList.count() >= 2){
            m_closeIndicatorDialog->show();
            m_closeIndicatorTimer->start();
        }
    }
}

void DialogManager::showTrashPropertyDialog(const FMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        m_trashDialog = new TrashPropertyDialog(event.fileUrl());
        m_trashDialog->show();
    }
}

void DialogManager::showDevicePropertyDialog(const FMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    if (w){
        PropertyDialog* dialog = new PropertyDialog(event.fileUrl());
        dialog->show();
    }
}

void DialogManager::showDiskErrorDialog(const QString & id, const QString & errorText)
{
    DDialog d;
    d.setTitle(id + ":" + errorText);
    QStringList buttonTexts;
    buttonTexts << tr("Ok");
    d.addButtons(buttonTexts);
    d.setDefaultButton(0);
    d.setIcon(QIcon(":/images/dialogs/images/dialog-warning.svg"));
    d.exec();
}

void DialogManager::showAboutDialog(const FMEvent &event)
{
    QWidget* w = WindowManager::getWindowById(event.windowId());
    QString icon(":/icons/images/system-file-manager.png");
    DAboutDialog *dialog = new DAboutDialog(icon,
                        icon,
                        tr("File Manager"),
                        tr("Version:1.0"),
                        tr("File Manager is a file management tool independently "
                           "developed by Deepin Technology, featured with searching, "
                           "copying, trash, compression/decompression, file property "
                           "and other file management functions. "),w);
    dialog->setTitle("");
    const QPoint global = w->mapToGlobal(w->rect().center());
    dialog->move(global.x() - dialog->width() / 2, global.y() - dialog->height() / 2);
    dialog->show();
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
    if (m_propertyDialogs.values().contains(qobject_cast<PropertyDialog*>(qApp->activeWindow()))){
        raiseAllPropertyDialog();
    }else if(m_closeIndicatorDialog == qobject_cast<CloseAllDialogIndicator*>(qApp->activeWindow())){
        raiseAllPropertyDialog();
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
