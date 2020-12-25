#ifndef JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_H_
#define JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_H_

// C++ lib import
#include <functional>

// Qt lib import
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QVector>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QMutex>

// JQLibrary lib import
#include <../JQDeclare>

class JQSentrySpan;
class JQSentrySpan;

struct JQSentrySpanData
{
    QString    operationName;
    QString    description;
    QJsonValue data;
    QString    status;

    QString parentSpanId;
    QString spanId;

    QDateTime startTime;
    QDateTime endTime;
};

class JQSentryTransit: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( JQSentryTransit )

public:
    JQSentryTransit() = default;

    ~JQSentryTransit() = default;

public slots:
    void transit(const std::function<void()> &callback);
};

class JQLIBRARY_EXPORT JQSentry: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( JQSentry )

    JQSentry() = default;

public:
    ~JQSentry() = default;

    static bool initialize(const QString &dsn);

    static void installMessageHandler(const int &acceptedType = QtWarningMsg | QtCriticalMsg);

    static bool serverReachable();


    static bool postLog(const QString &log, const QtMsgType &type = QtDebugMsg, const QMessageLogContext &context = { });

    static bool postMinidump(const QString &log, const QString &dumpFileName, const QByteArray &dumpFileData);

    static bool postPerformance(const QVector< JQSentrySpanData > &spanDataList);


    inline static void setServerName(const QString &serverName);

    inline static void setUserId(const QString &userId);

    inline static void setUserName(const QString &userName);

    inline static void setRelease(const QString &release);

    inline static void setLoggerName(const QString &loggerName);

private:
    static void handleReply(QNetworkReply *reply);

    static QString getLogLevel(const QtMsgType &type);

    static QJsonObject sentryData();

    static QByteArray xSentryAuth();

    static QJsonValue dateTimeToSentryTime(const QDateTime &time);

private:
    static QSharedPointer< QNetworkAccessManager > networkAccessManager_;
    static QPointer< JQSentryTransit > transit_;
    static bool continueFlag_;

    static QString clientName_;
    static QString clientVersion_;

    static QString dsn_;
    static QString protocol_;
    static QString publicKey_;
    static QString host_;
    static quint16 port_;
    static QString path_;
    static QString projectId_;

    static QString serverName_;
    static QString userId_;
    static QString userName_;
    static QString userIpAddress_;
    static QString release_;
};

class JQLIBRARY_EXPORT JQSentrySpan
{
private:
    JQSentrySpan(
        const QString &   operationName,
        const QString &   description,
        const QJsonValue &data );

public:
    ~JQSentrySpan();

    static QSharedPointer< JQSentrySpan > create(
        const QSharedPointer< JQSentrySpan > &parent,
        const QString &                       operationName,
        const QString &                       description = QString(),
        const QJsonValue &                    data = QJsonValue() );

    inline static QSharedPointer< JQSentrySpan > create(
        const QString &                       operationName,
        const QString &                       description = QString(),
        const QJsonValue &                    data = QJsonValue() );

    inline void cancel();

    inline void setStatus(const QString &status);

private:
    static QMutex mutex_;

    JQSentrySpanData spanData_;
    bool isCancel_ = false;

    QWeakPointer< JQSentrySpan > rootSpan_;
    QVector< JQSentrySpanData >  spanDataList_;
};

// .inc include
#include "jqsentry.inc"

#endif//JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_H_
