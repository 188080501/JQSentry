// Qt lib import
#include <QtCore>

// JQLibrary import
#include <JQSentry>

int main(int argc, char *argv[])
{
    qSetMessagePattern( "%{time hh:mm:ss.zzz}: %{message}" );

    QCoreApplication app( argc, argv );

    // 这里初始化填写DSN，请换成实际DSN，这里写的是我测试用的
    JQSentry::initialize( "https://e9b577341ff2463e95d4944ffd3b9a39@o495303.ingest.sentry.io/5567822" );

    QFile minidumpFile( ":/test.dmp" );
    minidumpFile.open( QIODevice::ReadOnly );

    // 手动上传minidump
    JQSentry::postMinidump(
        "This is a minidump",
        "test",
        minidumpFile.readAll() );

    // Sentry的数据会在后台上传，主线程必须开启事件循环
    QTimer::singleShot( 5000, &app, &QCoreApplication::quit );
    return app.exec();
}
