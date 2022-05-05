/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
 *             liqiang<liqianga@uniontech.com>
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

#include "dcustomactiondata.h"

DPMENU_USE_NAMESPACE

DCustomActionData::DCustomActionData()
    : actionPosition(0), actionNameArg(DCustomActionDefines::kNoneArg), actionCmdArg(DCustomActionDefines::kNoneArg), actionSeparator(DCustomActionDefines::kNone)
{
}

DCustomActionData::DCustomActionData(const DCustomActionData &other)
    : comboPos(other.comboPos), actionPosition(other.actionPosition), actionNameArg(other.actionNameArg), actionCmdArg(other.actionCmdArg), actionName(other.actionName), actionIcon(other.actionIcon), actionCommand(other.actionCommand), actionSeparator(other.actionSeparator), childrenActions(other.childrenActions)
{
}

DCustomActionData &DCustomActionData::operator=(const DCustomActionData &other)
{
    if (this == &other)
        return *this;
    actionNameArg = other.actionNameArg;
    actionCmdArg = other.actionCmdArg;
    actionName = other.actionName;
    comboPos = other.comboPos;
    actionPosition = other.actionPosition;
    actionSeparator = other.actionSeparator;
    actionIcon = other.actionIcon;
    actionCommand = other.actionCommand;
    childrenActions = other.childrenActions;
    return *this;
}

bool DCustomActionData::isMenu() const
{
    return !childrenActions.isEmpty();
}

bool DCustomActionData::isAction() const
{
    return childrenActions.isEmpty();
}

QString DCustomActionData::name() const
{
    return actionName;
}

int DCustomActionData::position(DCustomActionDefines::ComboType combo) const
{
    auto it = comboPos.find(combo);
    if (it != comboPos.end())
        return it.value();
    else
        return actionPosition;
}

int DCustomActionData::position() const
{
    return actionPosition;
}

QString DCustomActionData::icon() const
{
    return actionIcon;
}

QString DCustomActionData::command() const
{
    return actionCommand;
}

DCustomActionDefines::Separator DCustomActionData::separator() const
{
    return actionSeparator;
}

QList<DCustomActionData> DCustomActionData::acitons() const
{
    return childrenActions;
}

DCustomActionDefines::ActionArg DCustomActionData::nameArg() const
{
    return actionNameArg;
}

DCustomActionDefines::ActionArg DCustomActionData::commandArg() const
{
    return actionCmdArg;
}

DCustomActionEntry::DCustomActionEntry()
{
}

DCustomActionEntry::DCustomActionEntry(const DCustomActionEntry &other)
    : packageName(other.packageName), packageVersion(other.packageVersion), packageComment(other.packageComment), packageSign(other.packageSign), actionFileCombo(other.actionFileCombo), actionMimeTypes(other.actionMimeTypes), actionExcludeMimeTypes(other.actionExcludeMimeTypes), actionSupportSchemes(other.actionSupportSchemes), actionNotShowIn(other.actionNotShowIn), actionSupportSuffix(other.actionSupportSuffix), actionData(other.actionData)
{
}

DCustomActionEntry &DCustomActionEntry::operator=(const DCustomActionEntry &other)
{
    if (this == &other)
        return *this;
    packageName = other.packageName;
    packageVersion = other.packageVersion;
    packageComment = other.packageComment;
    actionFileCombo = other.actionFileCombo;
    actionMimeTypes = other.actionMimeTypes;
    actionExcludeMimeTypes = other.actionExcludeMimeTypes;
    actionSupportSchemes = other.actionSupportSchemes;
    actionNotShowIn = other.actionNotShowIn;
    actionSupportSuffix = other.actionSupportSuffix;
    packageSign = other.packageSign;
    actionData = other.actionData;
    return *this;
}

QString DCustomActionEntry::package() const
{
    return packageName;
}

QString DCustomActionEntry::version() const
{
    return packageVersion;
}

QString DCustomActionEntry::comment() const
{
    return packageComment;
}

DCustomActionDefines::ComboTypes DCustomActionEntry::fileCombo() const
{
    return actionFileCombo;
}

QStringList DCustomActionEntry::mimeTypes() const
{
    return actionMimeTypes;
}

QStringList DCustomActionEntry::excludeMimeTypes() const
{
    return actionExcludeMimeTypes;
}

QStringList DCustomActionEntry::surpportSchemes() const
{
    return actionSupportSchemes;
}

QStringList DCustomActionEntry::notShowIn() const
{
    return actionNotShowIn;
}

QStringList DCustomActionEntry::supportStuffix() const
{
    return actionSupportSuffix;
}

DCustomActionData DCustomActionEntry::data() const
{
    return actionData;
}
