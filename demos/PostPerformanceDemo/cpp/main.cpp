// Qt lib import
#include <QtCore>

// JQLibrary import
#include <JQSentry>

void something()
{
    // 通过手动方式生成span，生命周期结束后就会自动上传数据
    // 指定parent的span，生命周期结束后，会写入结果数据到root span中，等待root span生命周期结束后一起上传
    // 结果数据包括创建span是指定的operationName、description、span创建时间和span销毁时间

    // 可以附带数据，方便调试
    QJsonObject data;

    data[ "key" ]   = "mykey1";
    data[ "value" ] = "myvalue1";

    auto rootSpan = JQSentrySpan::create( "WorkResult", "saveToFile", data );

    QThread::msleep( 20 ); // 模拟耗时操作

    for ( auto index = 0; index < 3; ++index )
    {
        auto readyDataSpan = JQSentrySpan::create( rootSpan, "DataProvider", "readyData" );

        QThread::msleep( 20 );

        if ( index == 1 )
        {
            // 不需要上传时可以cancel
            readyDataSpan->cancel();
        }
    }

    {
        auto saveStep1Span = JQSentrySpan::create( rootSpan, "DataSaver", "saveStep1" );

        QThread::msleep( 5 );

        {
            // 可以不指定description
            auto saveStep2Span = JQSentrySpan::create( saveStep1Span, "DataSaver" );

            QThread::msleep( 10 );

            {
                // 也可以提前释放span
                saveStep1Span.clear();

                auto saveStep3Span = JQSentrySpan::create( saveStep2Span, "DataSaver", "saveStep3" );

                // 指定status，具体可以填写哪些值，请参考HTTP的status，不可以自定义
                saveStep3Span->setStatus( "internal_error" );

                QThread::msleep( 50 );
            }
        }
    }

    {
        auto cleanSpan = JQSentrySpan::create( rootSpan, "WorkResult", "cleanBuffer", data );

        QThread::msleep( 10 );
    }

    QThread::msleep( 5 );
}

int main(int argc, char *argv[])
{
    qSetMessagePattern( "%{time hh:mm:ss.zzz}: %{message}" );

    QCoreApplication app( argc, argv );

    // 这里初始化填写DSN，请换成实际DSN，这里写的是我测试用的
    JQSentry::initialize( "https://e9b577341ff2463e95d4944ffd3b9a39@o495303.ingest.sentry.io/5567822" );

    something();

    // Sentry的数据会在后台上传，主线程必须开启事件循环
    QTimer::singleShot( 5000, &app, &QCoreApplication::quit );
    return app.exec();
}
