// .h include
#include "jqsentry.h"

// Qt lib import
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUuid>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QCoreApplication>
#include <QThread>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QSysInfo>

// JQSentryTransit
void JQSentryTransit::transit(const std::function<void()> &callback)
{
    if ( QThread::currentThread() == this->thread() )
    {
        callback();
    }
    else
    {
        QMetaObject::invokeMethod(
            this,
            "transit",
            Qt::QueuedConnection,
            Q_ARG( std::function<void()>, callback ) );
    }
}

// JQSentry
QSharedPointer< QNetworkAccessManager > JQSentry::networkAccessManager_;
QPointer< JQSentryTransit > JQSentry::transit_;
bool JQSentry::continueFlag_ = true;

QString JQSentry::clientName_ = "JQSentry";
QString JQSentry::clientVersion_ = "1.3";

QString JQSentry::dsn_;
QString JQSentry::protocol_;
QString JQSentry::publicKey_;
QString JQSentry::host_;
quint16 JQSentry::port_;
QString JQSentry::path_;
QString JQSentry::projectId_;

QString JQSentry::serverName_;
QString JQSentry::userId_;
QString JQSentry::userName_;
QString JQSentry::userIpAddress_;
QString JQSentry::release_;

bool JQSentry::initialize(const QString &dsn)
{
    if ( networkAccessManager_ )
    {
        qDebug() << "Already initialized: Already initialized";
        return false;
    }

    if ( !qApp )
    {
        qDebug() << "Cannot be used without QApplication";
        return false;
    }

    QUrl url( dsn );
    if ( dsn.isEmpty() || !url.isValid() )
    {
        qDebug() << "JQSentry::initialize: dsn is not valid";
        return false;
    }

    if ( ( url.scheme().toLower() == "https" ) && ( !QSslSocket::supportsSsl() ) )
    {
        qDebug() << "JQSentry::initialize: protocol is https but ssl is not support";
        return false;
    }

    qRegisterMetaType< std::function<void()> >( "std::function<void()>" );

    networkAccessManager_.reset( new QNetworkAccessManager );
    transit_ = new JQSentryTransit;

    dsn_       = dsn;
    protocol_  = url.scheme();
    publicKey_ = url.userName();
    host_      = url.host();
    port_      = static_cast< quint16 >( url.port( 80 ) );
    path_      = url.path();
    projectId_ = url.fileName();

    int i = path_.lastIndexOf( '/' );
    if ( i >= 0 )
    {
        path_ = path_.left( i );
    }

    userName_ = qgetenv( "USER" );
    if ( userName_.isEmpty() )
    {
        userName_ = qgetenv( "USERNAME" );
    }

    userIpAddress_ = []()->QString
    {
        for ( const auto &address: QNetworkInterface::allAddresses() )
        {
            if ( !address.isLoopback() && ( address.protocol() == QAbstractSocket::IPv4Protocol ) )
            {
                return address.toString();
            }
        }

        return { };
    }();

#ifndef QT_NO_DEBUG
    qDebug().noquote() << "JQSentry: initialize succeed, project id:" << projectId_;
#endif

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, []() {
        continueFlag_ = false;
        networkAccessManager_.clear();
        transit_->deleteLater();
    } );

    return true;
}

void JQSentry::installMessageHandler(const int &acceptedType_)
{
    static const auto defaultHandler = qInstallMessageHandler( nullptr );
    static const auto acceptedType = acceptedType_;

    class HelperClass
    {
    public:
        static void messageHandler(
            QtMsgType                 type,
            const QMessageLogContext &context,
            const QString &           log )
        {
            defaultHandler( type, context, log );

            if ( acceptedType & type )
            {
                JQSentry::postLog( log, type, context );
            }
        }
    };

    qInstallMessageHandler( HelperClass::messageHandler );
}

bool JQSentry::serverReachable()
{
    QTcpSocket socket;

    socket.connectToHost( host_, port_ );
    socket.waitForConnected( 5000 );

    return socket.state() == QAbstractSocket::ConnectedState;
}

bool JQSentry::postLog(const QString &log, const QtMsgType &type, const QMessageLogContext &context)
{
    if ( !networkAccessManager_ ) { return false; }

    if ( !continueFlag_ ) { return false; }

    if ( log.contains( "QDisabledNetworkReply" ) ) { return false; }

    if ( QThread::currentThread() != transit_->thread() )
    {
        const auto fileName = context.file;
        const auto lineNumber = context.line;
        const auto functionName = context.function;
        const auto category = context.category;

        transit_->transit( [ = ]()
        {
            JQSentry::postLog( log, type, { fileName, lineNumber, functionName, category } );
        } );
        return true;
    }

    auto data = sentryData();

    data[ "event_id" ]  = QUuid::createUuid().toString().mid( 1, 36 );
    data[ "level" ]     = getLogLevel( type );
    data[ "message" ]   = log;

    {
        QJsonObject tagData;

        if ( !QString( context.file ).isEmpty() )
        {
            tagData[ "file" ] = QFileInfo( context.file ).fileName();
        }

        data[ "tags" ] = tagData;
    }

    if ( !QString( context.file ).isEmpty() )
    {
        QString function = context.function;
        function.replace( "static ", "" );
        function.replace( "__cdecl ", "" );
        function.replace( "(anonymous class)::", "" );

        data[ "culprit" ] = QString( "file: %1 / function: %2 / line: %3" )
                                .arg(
                                    QFileInfo( context.file ).fileName(),
                                    function,
                                    QString::number( context.line ) );
    }

    const auto url = QString( "%1://%2:%3%4/api/%5/store/" )
                         .arg( protocol_ )
                         .arg( host_ )
                         .arg( port_ )
                         .arg( path_ )
                         .arg( projectId_ );
    const auto auth = xSentryAuth();

    QNetworkRequest request( url );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
    request.setRawHeader( "X-Sentry-Auth", xSentryAuth() );

    handleReply( networkAccessManager_->post( request, QJsonDocument( data ).toJson( QJsonDocument::Compact ) ) );

    return true;
}

bool JQSentry::postMinidump(const QString &log, const QString &dumpFileName, const QByteArray &dumpFileData)
{
    if ( !networkAccessManager_ ) { return false; }

    if ( !continueFlag_ ) { return false; }

    auto data = sentryData();

    data[ "message" ] = log;

    auto multiPart = new QHttpMultiPart;
    multiPart->setBoundary( QString( "-----%1" ).arg( QUuid::createUuid().toString().mid( 1, 36 ) ).toUtf8() );

    {
        QHttpPart part;

        part.setHeader( QNetworkRequest::ContentDispositionHeader, QVariant( "form-data; name=\"sentry\"" ) );
        part.setBody( QJsonDocument( data ).toJson( QJsonDocument::Compact ) );

        multiPart->append( part );
    }

    {
        QHttpPart part;

        part.setHeader( QNetworkRequest::ContentTypeHeader, QVariant( "file" ) );
        part.setHeader( QNetworkRequest::ContentDispositionHeader, QVariant( "form-data; name=\"upload_file_minidump\"; filename=\"" + dumpFileName + ".dmp\"" ) );
        part.setBody( dumpFileData );

        multiPart->append( part );
    }

    {
        QHttpPart part;

        part.setHeader( QNetworkRequest::ContentDispositionHeader, QVariant( "form-data; name=\"some_file\"; filename=\"" + dumpFileName + ".dmp\"" ) );
        part.setBody( dumpFileData );

        multiPart->append( part );
    }

    const auto url = QString( "%1://%2:%3%4/api/%5/minidump/" )
                         .arg( protocol_ )
                         .arg( host_ )
                         .arg( port_ )
                         .arg( path_ )
                         .arg( projectId_ );

    QNetworkRequest request( url );
    request.setRawHeader( "Content-Type", "multipart/form-data; boundary=" + multiPart->boundary() );
    request.setRawHeader( "X-Sentry-Auth", xSentryAuth() );

    auto reply = networkAccessManager_->post( request, multiPart );
    multiPart->setParent( reply );

    handleReply( reply );

    return true;
}

void JQSentry::handleReply(QNetworkReply *reply)
{
    QSharedPointer< bool > isCalled( new bool( false ) );

    QObject::connect( reply, &QNetworkReply::finished, [ reply, isCalled ]()
    {
        if ( *isCalled ) { return; }
        *isCalled = true;

        const auto &&rawData = reply->readAll();
        if ( rawData.size() != 36 )
        {
            const auto &&data = QJsonDocument::fromJson( rawData ).object();
            if ( data[ "id" ].toString().isEmpty() )
            {
                qDebug() << "JQSentry::handleReply: data error:" << data;
            }
        }

        reply->deleteLater();
    } );

    QObject::connect( reply, static_cast< void( QNetworkReply::* )( QNetworkReply::NetworkError ) >( &QNetworkReply::error ), [ reply, isCalled ](const QNetworkReply::NetworkError &code)
    {
        if ( *isCalled ) { return; }
        *isCalled = true;

        qDebug() << "JQSentry::handleReply: error:" << code << reply->readAll();

        reply->deleteLater();
    } );
}

QString JQSentry::getLogLevel(const QtMsgType &type)
{
    switch ( type )
    {
        case QtDebugMsg: return "debug";
        case QtWarningMsg: return "warning";
        case QtCriticalMsg: return "error";
        case QtFatalMsg: return "fatal";
        case QtInfoMsg: return "info";
    }

    return "debug";
}

QJsonObject JQSentry::sentryData()
{
    QJsonObject data;

    data[ "timestamp" ] = static_cast< int >( QDateTime::currentDateTime().toTime_t() );
    data[ "platform" ]  = "C++/Qt";
    data[ "logger" ] = clientName_;

    {
        QJsonObject sdkData;

        sdkData[ "name" ] = clientName_;
        sdkData[ "version" ] = clientVersion_;

        data[ "sdk" ] = sdkData;
    }

    {
        QJsonObject contextsData;

        {
            QJsonObject osData;

            osData[ "name" ] = QSysInfo::productType();
            osData[ "version" ] = QSysInfo::productVersion();
            osData[ "type" ] = "os";

            contextsData[ "os" ] = osData;
        }

        {
            QJsonObject browserData;

            browserData[ "name" ] = "Qt";
            browserData[ "version" ] = QT_VERSION_STR;

            contextsData[ "browser" ] = browserData;
        }

        data[ "contexts" ] = contextsData;
    }

    {
        QJsonObject userData;

        if ( !userId_.isEmpty() )
        {
            userData[ "id" ] = userId_;
        }
        if ( !userName_.isEmpty() )
        {
            userData[ "username" ] = userName_;
        }
        if ( !userIpAddress_.isEmpty() )
        {
            userData[ "ip_address" ] = userIpAddress_;
        }

        data[ "user" ] = userData;
    }

    if ( !release_.isEmpty() )
    {
        data[ "release" ] = release_;
    }

    if ( !serverName_.isEmpty() )
    {
        data[ "server_name" ] = serverName_;
    }

    return data;
}

QByteArray JQSentry::xSentryAuth()
{
    return QString( "Sentry "
                    "sentry_version=5,sentry_timestamp=%1,sentry_key=%2,sentry_secret=%3" )
               .arg(
                   QString::number( QDateTime::currentDateTime().toTime_t() ),
                   publicKey_,
                   "" ).toUtf8();
}
