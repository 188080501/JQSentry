#ifndef JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_INC_
#define JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_INC_

// .h include
#include "jqsentry.h"

inline void JQSentry::setServerName(const QString &serverName)
{
    serverName_ = serverName;
}

inline void JQSentry::setUserId(const QString &userId)
{
    userId_ = userId;
}

inline void JQSentry::setUserName(const QString &userName)
{
    userName_ = userName;
}

inline void JQSentry::setRelease(const QString &release)
{
    release_ = release;
}

inline void JQSentry::setLoggerName(const QString &loggerName)
{
    clientName_ = loggerName;
}

inline void JQSentrySpan::cancel()
{
    isCancel_ = true;
}

inline void JQSentrySpan::setStatus(const QString &status)
{
    spanData_.status = status;
}

inline QSharedPointer< JQSentrySpan > JQSentrySpan::create(
    const QString &                       operationName,
    const QString &                       description,
    const QJsonValue &                    data )
{
    return create(
        nullptr,
        operationName,
        description,
        data );
}

#endif//JQLIBRARY_INCLUDE_JQSENTRY_JQSENTRY_INC_
