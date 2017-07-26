/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef CRYPTO_FOLDER_H
#define CRYPTO_FOLDER_H

#include "syncresult.h"
#include "progressdispatcher.h"
#include "syncjournaldb.h"
#include "clientproxy.h"
#include "networkjobs.h"
#include "abstractfolder.h"

#include <csync.h>

#include <QObject>
#include <QStringList>

class QThread;
class QSettings;

namespace OCC {

class SyncEngine;
class AccountState;
class FolderDefinition;

/**
 * @brief The Folder class
 * @ingroup gui
 */
class CryptoFolder : public AbstractFolder
{
    Q_OBJECT

public:
    CryptoFolder(const FolderDefinition &definition, AccountState *accountState, QObject *parent = 0L);

    ~CryptoFolder();

    RequestEtagJob *etagJob() { return _requestEtagJob; }

public slots:

    void startSync(const QStringList &pathList = QStringList());

private slots:
    void slotRunEtagJob();
    void etagRetreived(const QString &);
    void etagRetreivedFromSyncEngine(const QString &);


private:

    QPointer<RequestEtagJob> _requestEtagJob;
    QString _lastEtag;
};
}

#endif
