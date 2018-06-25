/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <QtTest>
#include "syncenginetestutils.h"
#include <syncengine.h>

using namespace OCC;

class FakeAsyncReply : public QNetworkReply
{
    Q_OBJECT
    QByteArray _pollLocation;

public:
    FakeAsyncReply(const QByteArray &pollLocation, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent)
        : QNetworkReply{ parent }
        , _pollLocation(pollLocation)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        open(QIODevice::ReadOnly);

        QMetaObject::invokeMethod(this, "respond", Qt::QueuedConnection);
    }

    Q_INVOKABLE void respond()
    {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 202);
        setRawHeader("OC-JobStatus-Location", _pollLocation);
        emit metaDataChanged();
        emit finished();
    }

    void abort() override {}
    qint64 readData(char *, qint64) override { return 0; }
};


class TestAsyncOp : public QObject
{
    Q_OBJECT

private slots:

    void asyncOperations()
    {
        FakeFolder fakeFolder{ FileInfo::A12_B12_C12_S12() };
        fakeFolder.syncEngine().account()->setCapabilities({ { "dav", QVariantMap{ { "chunking", "1.0" } } } });
        // Reduce max chunk size a bit so we get more chunks
        SyncOptions options;
        options._maxChunkSize = 20 * 1000;
        fakeFolder.syncEngine().setSyncOptions(options);
        int nGET = 0;

        struct TestCase
        {
            std::function<QNetworkReply *(TestCase *, const QNetworkRequest &request)> pollRequest;
            std::function<FileInfo *()> perform = nullptr;
        };
        QHash<QString, TestCase> testCases;

        fakeFolder.setServerOverride([&](QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData) -> QNetworkReply * {
            auto path = request.url().path();

            if (op == QNetworkAccessManager::GetOperation && path.startsWith("/async-poll/")) {
                auto file = path.mid(sizeof("/async-poll/") - 1);
                Q_ASSERT(testCases.contains(file));
                auto &testCase = testCases[file];
                return testCase.pollRequest(&testCase, request);
            }

            if (op == QNetworkAccessManager::PutOperation && !path.contains("/uploads/")) {
                // Not chunking
                auto file = getFilePathFromUrl(request.url());
                Q_ASSERT(testCases.contains(file));
                auto &testCase = testCases[file];
                Q_ASSERT(!testCase.perform);
                auto putPayload = outgoingData->readAll();
                testCase.perform = [putPayload, request, &fakeFolder] {
                    return FakePutReply::perform(fakeFolder.remoteModifier(), request, putPayload);
                };
                return new FakeAsyncReply("/async-poll/" + file.toUtf8(), op, request, &fakeFolder.syncEngine());
            } else if (request.attribute(QNetworkRequest::CustomVerbAttribute) == "MOVE") {
                QString file = getFilePathFromUrl(QUrl::fromEncoded(request.rawHeader("Destination")));
                Q_ASSERT(testCases.contains(file));
                auto &testCase = testCases[file];
                Q_ASSERT(!testCase.perform);
                testCase.perform = [request, &fakeFolder] {
                    return FakeChunkMoveReply::perform(fakeFolder.uploadState(), fakeFolder.remoteModifier(), request);
                };
                return new FakeAsyncReply("/async-poll/" + file.toUtf8(), op, request, &fakeFolder.syncEngine());
            } else if (op == QNetworkAccessManager::GetOperation) {
                nGET++;
            }
            return nullptr;
        });


        auto successCallbak = [](TestCase *tc, const QNetworkRequest &request) {
            tc->pollRequest = [](auto...) -> QNetworkReply * { std::abort(); }; // shall no longer be called
            FileInfo *info = tc->perform();
            QByteArray body = "{ \"status\":\"finished\", \"ETag\":\"\\\"" + info->etag.toUtf8() + "\\\"\", \"fileId\":\"" + info->fileId + "\"}\n";
            return new FakePayloadReply(QNetworkAccessManager::GetOperation, request, body, nullptr);
        };
        auto waitForeverCallBack = [](TestCase *, const QNetworkRequest &request) {
            QByteArray body = "{\"status\":\"started\"}\n";
            return new FakePayloadReply(QNetworkAccessManager::GetOperation, request, body, nullptr);
        };
        auto errorCallBack = [](TestCase *tc, const QNetworkRequest &request) {
            tc->pollRequest = [](auto...) -> QNetworkReply * { std::abort(); }; // shall no longer be called;
            QByteArray body = "{\"status\":\"error\",\"errorCode\":500,\"errorMessage\":\"TestingErrors\"}\n";
            return new FakePayloadReply(QNetworkAccessManager::GetOperation, request, body, nullptr);
        };
        auto waitAndChain = [](const auto &chain) {
            return [chain](TestCase *tc, const QNetworkRequest &request) {
                tc->pollRequest = chain;
                QByteArray body = "{\"status\":\"started\"}\n";
                return new FakePayloadReply(QNetworkAccessManager::GetOperation, request, body, nullptr);
            };
        };

        auto insertFile = [&](const QString &file, int size, auto cb) {
            fakeFolder.localModifier().insert(file, size);
            testCases[file] = { cb };
        };
        fakeFolder.localModifier().mkdir("success");
        insertFile("success/chunked_success", options._maxChunkSize * 3, successCallbak);
        insertFile("success/single_success", 300, successCallbak);
        insertFile("success/chunked_patience", options._maxChunkSize * 3,
            waitAndChain(waitAndChain(successCallbak)));
        insertFile("success/single_patience", 300,
            waitAndChain(waitAndChain(successCallbak)));
        fakeFolder.localModifier().mkdir("err");
        insertFile("err/chunked_error", options._maxChunkSize * 3, errorCallBack);
        insertFile("err/single_error", 300, errorCallBack);
        insertFile("err/chunked_error2", options._maxChunkSize * 3, waitAndChain(errorCallBack));
        insertFile("err/single_error2", 300, waitAndChain(errorCallBack));

        // First sync should finish by itself.
        // All the things in "success/" should be transfered, the things in "err/" ,not
        QVERIFY(!fakeFolder.syncOnce());
        QCOMPARE(nGET, 0);
        QCOMPARE(*fakeFolder.currentLocalState().find("success"),
            *fakeFolder.currentRemoteState().find("success"));
        testCases.clear();
        testCases["err/chunked_error"] = { successCallbak };
        testCases["err/chunked_error2"] = { successCallbak };
        testCases["err/single_error"] = { successCallbak };
        testCases["err/single_error2"] = { successCallbak };

        fakeFolder.localModifier().mkdir("waiting");
        insertFile("waiting/small", 300, waitForeverCallBack);
        insertFile("waiting/willNotConflict", 300, waitForeverCallBack);
        insertFile("waiting/big", options._maxChunkSize * 3,
            waitAndChain(waitAndChain([&](TestCase *tc, const QNetworkRequest &request) {
                QTimer::singleShot(0, &fakeFolder.syncEngine(), &SyncEngine::abort);
                return waitAndChain(waitForeverCallBack)(tc, request);
            })));

        fakeFolder.syncJournal().wipeErrorBlacklist();

        // This second sync will redo the files that had errors
        // But the waiting folder will not complete before it is aborted.
        QVERIFY(!fakeFolder.syncOnce());
        QCOMPARE(nGET, 0);
        QCOMPARE(*fakeFolder.currentLocalState().find("err"),
            *fakeFolder.currentRemoteState().find("err"));

        testCases["waiting/small"].pollRequest = waitAndChain(waitAndChain(successCallbak));
        testCases["waiting/big"].pollRequest = waitAndChain(successCallbak);
        testCases["waiting/willNotConflict"].pollRequest =
            [&fakeFolder, &successCallbak](TestCase *tc, const QNetworkRequest &request) {
                auto &remoteModifier = fakeFolder.remoteModifier(); // successCallbak destroyes the capture
                auto reply = successCallbak(tc, request);
                // This is going to succeed, and after we just change the file.
                // This should not be a conflict, but this should be downloaded in the
                // next sync
                remoteModifier.appendByte("waiting/willNotConflict");
                return reply;
            };


        int nPUT = 0;
        int nMOVE = 0;
        int nDELETE = 0;
        fakeFolder.setServerOverride([&](QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *) -> QNetworkReply * {
            auto path = request.url().path();
            if (op == QNetworkAccessManager::GetOperation && path.startsWith("/async-poll/")) {
                auto file = path.mid(sizeof("/async-poll/") - 1);
                Q_ASSERT(testCases.contains(file));
                auto &testCase = testCases[file];
                return testCase.pollRequest(&testCase, request);
            } else if (op == QNetworkAccessManager::PutOperation) {
                nPUT++;
            } else if (op == QNetworkAccessManager::GetOperation) {
                nGET++;
            } else if (op == QNetworkAccessManager::DeleteOperation) {
                nDELETE++;
            } else if (request.attribute(QNetworkRequest::CustomVerbAttribute) == "MOVE") {
                nMOVE++;
            }
            return nullptr;
        });


        // This last sync will do the waiting stuff
        QVERIFY(fakeFolder.syncOnce());
        QCOMPARE(nGET, 1); // "waiting/willNotConflict"
        QCOMPARE(nPUT, 0);
        QCOMPARE(nMOVE, 0);
        QCOMPARE(nDELETE, 0);
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
    }
};

QTEST_GUILESS_MAIN(TestAsyncOp)
#include "testasyncop.moc"
