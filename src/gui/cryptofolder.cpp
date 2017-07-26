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
#include "config.h"

#include "account.h"
#include "accountstate.h"
#include "folderdefinition.h"
#include "cryptofolder.h"
#include "folderman.h"
#include "logger.h"
#include "configfile.h"
#include "networkjobs.h"
#include "syncjournalfilerecord.h"
#include "syncresult.h"
#include "utility.h"
#include "clientproxy.h"
#include "syncengine.h"
#include "syncrunfilelog.h"
#include "socketapi.h"
#include "theme.h"
#include "filesystem.h"
#include "excludedfiles.h"

#include "creds/abstractcredentials.h"

#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QSettings>

#include <QMessageBox>
#include <QPushButton>

namespace OCC {

Q_LOGGING_CATEGORY(lcCryptoFolder, "gui.cryptofolder", QtInfoMsg)

CryptoFolder::CryptoFolder(const FolderDefinition &definition,
                           AccountState *accountState,
                           QObject *parent)
    : AbstractFolder(definition, accountState, parent)
{
    // FIXME: Engine is initialized in the AbstractFolder constructor.
    // Change that to have a method initializeSyncEngine(SyncEngine) in the AbstractFolder
    // that needs to be called by the Folder class, to let the Folder instanciate its own
    // SyncEngine. For that, we need an Abstract Sync Engine as well.

    connect(_engine.data(), SIGNAL(rootEtag(QString)), this, SLOT(etagRetreivedFromSyncEngine(QString)));
}

CryptoFolder::~CryptoFolder()
{

}

void CryptoFolder::slotRunEtagJob()
{
    qCInfo(lcCryptoFolder) << "Trying to check" << remoteUrl().toString() << "for changes via ETag check.";

    AccountPtr account = _accountState->account();

    if (_requestEtagJob) {
        qCInfo(lcCryptoFolder) << remoteUrl().toString() << "has ETag job queued, not trying to sync";
        return;
    }

    if (!canSync()) {
        qCInfo(lcCryptoFolder) << "Not syncing.  :" << remoteUrl().toString() << _definition.paused << AccountState::stateString(_accountState->state());
        return;
    }

    // Do the ordinary etag check for the root folder and schedule a
    // sync if it's different.

    _requestEtagJob = new RequestEtagJob(account, remotePath(), this);
    _requestEtagJob->setTimeout(60 * 1000);
    // check if the etag is different when retrieved
    QObject::connect(_requestEtagJob, SIGNAL(etagRetreived(QString)), this, SLOT(etagRetreived(QString)));
    FolderMan::instance()->slotScheduleETagJob(alias(), _requestEtagJob);
    // The _requestEtagJob is auto deleting itself on finish. Our guard pointer _requestEtagJob will then be null.
}

void CryptoFolder::etagRetreived(const QString &etag)
{
    // re-enable sync if it was disabled because network was down
    FolderMan::instance()->setSyncEnabled(true);

    if (_lastEtag != etag) {
        qCInfo(lcCryptoFolder) << "Compare etag with previous etag: last:" << _lastEtag << ", received:" << etag << "-> CHANGED";
        _lastEtag = etag;
        slotScheduleThisFolder();
    }

    _accountState->tagLastSuccessfullETagRequest();
}

void CryptoFolder::etagRetreivedFromSyncEngine(const QString &etag)
{
    qCInfo(lcCryptoFolder) << "Root etag from during sync:" << etag;
    accountState()->tagLastSuccessfullETagRequest();
    _lastEtag = etag;
}

void CryptoFolder::startSync(const QStringList &pathList)
{
    AbstractFolder::startSync(pathList);
}

} // namespace OCC
