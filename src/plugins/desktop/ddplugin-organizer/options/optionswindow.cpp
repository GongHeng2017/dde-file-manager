/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
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

#include "optionswindow.h"
#include "optionswindow_p.h"
#include "config/configpresenter.h"

#include <dpf.h>

#include <DTitlebar>

#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QSize>

using namespace ddplugin_organizer;
DWIDGET_USE_NAMESPACE

OptionsWindowPrivate::OptionsWindowPrivate(OptionsWindow *qq)
    : QObject(qq)
    , q(qq)
{
    dpfSignalDispatcher->subscribe("ddplugin_canvas", "signal_CanvasManager_AutoArrangeChanged", this, &OptionsWindowPrivate::autoArrangeChanged);
}

OptionsWindowPrivate::~OptionsWindowPrivate()
{
    dpfSignalDispatcher->unsubscribe("ddplugin_canvas", "signal_CanvasManager_AutoArrangeChanged", this, &OptionsWindowPrivate::autoArrangeChanged);
}

bool OptionsWindowPrivate::isAutoArrange()
{
    return dpfSlotChannel->push("ddplugin_canvas", "slot_CanvasManager_AutoArrange").toBool();
}

void OptionsWindowPrivate::setAutoArrange(bool on)
{
    dpfSlotChannel->push("ddplugin_canvas", "slot_CanvasManager_SetAutoArrange", on);
}

void OptionsWindowPrivate::autoArrangeChanged(bool on)
{
    // sync sate that changed on canvas.
    if (autoArrange && autoArrange->checked() != on) {
        autoArrange->setChecked(on);
    }
}

void OptionsWindowPrivate::enableChanged(bool enable)
{
    if (organization) {
        autoArrange->setVisible(!CfgPresenter->isEnable());
        organization->reset();
        sizeSlider->switchMode(enable ? SizeSlider::View : SizeSlider::Icon);
        q->adjustSize();
    }
}

OptionsWindow::OptionsWindow(QWidget *parent)
    : DAbstractDialog(parent)
    , d(new OptionsWindowPrivate(this))
{

}

OptionsWindow::~OptionsWindow()
{

}

bool OptionsWindow::initialize()
{
    Q_ASSERT(!layout());
    Q_ASSERT(!d->mainLayout);

    // main layout
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize); // update size when subwidget is removed.
    setLayout(mainLayout);
    d->mainLayout = mainLayout;

    // title
    auto titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    titleBar->setTitle(tr("Desktop options"));
    mainLayout->addWidget(titleBar, 0, Qt::AlignTop);

    // content
    d->contentWidget = new QWidget(this);
    mainLayout->addWidget(d->contentWidget);

    auto contentLayout = new QVBoxLayout(d->contentWidget);
    contentLayout->setContentsMargins(10, 0, 10, 10);
    contentLayout->setSpacing(0);
    contentLayout->setSizeConstraint(QLayout::SetFixedSize);
    d->contentLayout = contentLayout;
    d->contentWidget->setLayout(contentLayout);

    // organization
    d->organization = new OrganizationGroup(d->contentWidget);
    d->organization->reset();
    contentLayout->addWidget(d->organization);

    contentLayout->addSpacing(10);

    // auto arrange
    d->autoArrange = new SwitchWidget(tr("Auto arrange icons"), this);
    d->autoArrange->setChecked(d->isAutoArrange());
    d->autoArrange->setFixedSize(400, 48);
    d->autoArrange->setRoundEdge(SwitchWidget::kBoth);
    contentLayout->addWidget(d->autoArrange);
    connect(d->autoArrange, &SwitchWidget::checkedChanged, this, [this](bool check){
        d->setAutoArrange(check);
    });
    // hide auto arrange if organizer is on.
    d->autoArrange->setVisible(!CfgPresenter->isEnable());

    contentLayout->addSpacing(10);

    // size slider
    d->sizeSlider = new SizeSlider(this);
    d->sizeSlider->setRoundEdge(SwitchWidget::kBoth);
    d->sizeSlider->setFixedSize(400, 94);
    d->sizeSlider->switchMode(CfgPresenter->isEnable() ? SizeSlider::View : SizeSlider::Icon);
    contentLayout->addWidget(d->sizeSlider);

    adjustSize();

    // must be queued
    connect(CfgPresenter, &ConfigPresenter::changeEnableState, d, &OptionsWindowPrivate::enableChanged, Qt::QueuedConnection);
    return true;
}

void OptionsWindow::moveToCenter(const QPoint &cursorPos)
{
    if (auto screen = QGuiApplication::screenAt(cursorPos)) {
        auto pos = (screen->size() - size()) / 2.0;
        if (!pos.isValid()) {
            move(screen->geometry().topLeft());
        } else {
            auto topleft = screen->geometry().topLeft();
            move(topleft.x() + pos.width(), topleft.y() + pos.height());
        }
    }
}
