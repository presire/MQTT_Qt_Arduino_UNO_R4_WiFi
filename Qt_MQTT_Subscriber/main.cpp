#include <QCoreApplication>
#include "Subscriber.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Subscriber subscriber;

    // 接続成功時の処理
    QObject::connect(&subscriber, &Subscriber::connected, [&subscriber]() {
        subscriber.subscribe("arduino/sensor");
    });

    // メッセージ受信時の処理
    QObject::connect(&subscriber, &Subscriber::messageReceived, [](const QString &topic, const QByteArray &message) {
        qDebug() << "受信 - トピック : " << topic;
        qDebug() << "メッセージ : "  << message;
    });

    subscriber.connectToHost("<MQTTブローカーのホスト名またはIPアドレス  例: localhost>",
                             1883,  // このサンプルコードでは、SSL/TLSを使用しないためデフォルトポート番号1883を使用
                             "<MQTTユーザ名 (匿名ユーザを許可する場合は空にする)>",
                             "<MQTTユーザのパスワード (匿名ユーザを許可する場合は空にする)>");

    return a.exec();
}
