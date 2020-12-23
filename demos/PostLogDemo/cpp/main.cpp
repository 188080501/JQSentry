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

    // 手动上传一条log，指定类型为 QtDebugMsg
    JQSentry::postLog( "This is a debug", QtDebugMsg );

    // 注册全局消息捕获，注册后会自动捕获 qWarning 和 qCritical 中包含的信息，也包括Qt内部报错
    JQSentry::installMessageHandler();

    // 注册全局消息捕获后，使用QDebug系列接口就可以完成对应的上传
    qWarning() << "This is a warning";
    qCritical() << "This is a critical";

    // 注册全局消息捕获后，也会捕获Qt内部的报错，这里模拟一次Qt发出的报错
    QFile file( "qrc:/this_file_not_exists" );
    file.write( "balabala" );

    return app.exec();
}
