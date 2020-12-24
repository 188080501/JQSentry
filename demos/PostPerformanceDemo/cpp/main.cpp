// Qt lib import
#include <QtCore>

// JQLibrary import
#include <JQSentry>

void something()
{
    // 模拟一些流程，通过msleep模拟耗时操作
    // 这里是通过手动方式生成span，没有指定parent的为root span，生命周期结束后就会上传数据
    // 指定parent的span，生命周期结束后，会写入结果数据到root span中，等待root span一起上传
    // 结果数据包括创建span是指定的operationName、description、span创建时间和span销毁时间

    auto rootSpan = JQSentrySpan::create( "WorkResult", "saveToFile" );

    QThread::msleep( 20 );

    for ( auto index = 0; index < 3; ++index )
    {
        auto readyDataSpan = JQSentrySpan::create( "DataProvider", "readyData", rootSpan );

        QThread::msleep( 20 );
    }

    {
        auto saveStep1Span = JQSentrySpan::create( "DataSaver", "saveStep1", rootSpan );

        QThread::msleep( 5 );

        {
            auto saveStep2Span = JQSentrySpan::create( "DataSaver", "saveStep2", saveStep1Span );

            QThread::msleep( 10 );

            {
                // 也可以提前释放span
                saveStep1Span.clear();

                auto saveStep3Span = JQSentrySpan::create( "DataSaver", "saveStep3", saveStep2Span );

                QThread::msleep( 50 );
            }
        }
    }

    {
        auto cleanSpan = JQSentrySpan::create( "WorkResult", "cleanBuffer", rootSpan );

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
    QTimer::singleShot( 10000, &app, &QCoreApplication::quit );
    return app.exec();
}
