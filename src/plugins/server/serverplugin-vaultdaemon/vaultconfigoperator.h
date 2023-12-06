// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VAULTCONFIGOPERATOR_H
#define VAULTCONFIGOPERATOR_H

#include "serverplugin_vaultdaemon_global.h"

#include <QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

SERVERVAULT_BEGIN_NAMESPACE

class VaultConfigOperator
{
public:
    VaultConfigOperator(const QString &filePath = "");
    ~VaultConfigOperator();
    void set(const QString &nodeName, const QString &keyName, QVariant value);
    QVariant get(const QString &nodeName, const QString &keyName);
    QVariant get(const QString &nodeName, const QString &keyName, const QVariant &defaultValue);

private:
    QString currentFilePath;
    QSettings *setting;
};
SERVERVAULT_END_NAMESPACE

#endif // VAULTCONFIGOPERATOR_H
