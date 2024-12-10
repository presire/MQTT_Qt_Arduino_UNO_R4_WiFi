#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QTimer>
#include <QDebug>

class Subscriber : public QObject
{
    Q_OBJECT

private:
    QMqttClient *m_client;
    QString     m_host;
    quint16     m_port;
    QMap<QString, QMqttSubscription*> m_subscriptions;

public:
    explicit Subscriber(QObject *parent = nullptr) : QObject(parent), m_client(new QMqttClient(this))
    {
        // MQTT接続状態変更時のシグナルをハンドラに接続
        connect(m_client, &QMqttClient::stateChanged, this, &Subscriber::handleConnectionStateChange);

        // エラー発生時のシグナルをハンドラに接続
        connect(m_client, &QMqttClient::errorChanged, this, &Subscriber::handleError);
    }

    ~Subscriber()
    {
        // 全ての購読を解除
        for (auto it = m_subscriptions.constBegin(); it != m_subscriptions.constEnd(); it++) {
            it.value()->unsubscribe();
            delete it.value();
        }

        m_subscriptions.clear();

        if (m_client->state() == QMqttClient::Connected) {
            m_client->disconnect();
        }
    }

    // MQTT接続を開始する
    void connectToHost(const QString &host, quint16 port, const QString &username = QString(), const QString &password = QString())
    {
        m_host = host;
        m_port = port;

        // 接続パラメータを設定
        m_client->setHostname(host);
        m_client->setPort(port);

        // ユーザ名とパスワードが指定されている場合は設定
        if (!username.isEmpty()) {
            m_client->setUsername(username);
        }

        if (!password.isEmpty()) {
            m_client->setPassword(password);
        }

        // 非同期で接続を開始
        m_client->connectToHost();
    }

    // トピックを購読する
    void subscribe(const QString &topic)
    {
        if (m_client->state() != QMqttClient::Connected) {
            qWarning() << "MQTTクライアントが接続されていません";
            return;
        }

        // 既に購読済みのトピックは無視
        if (m_subscriptions.contains(topic)) {
            qInfo() << "トピックは既に購読されています : " << topic;
            return;
        }

        // QoS 1でトピックを購読 (メッセージ到達保証あり)
        auto subscription = m_client->subscribe(topic, 1);
        if (!subscription) {
            qWarning() << "トピックの購読に失敗しました : " << topic;
            return;
        }

        // メッセージ受信時のハンドラを設定
        connect(subscription, &QMqttSubscription::messageReceived, this, &Subscriber::handleMessage);

        // 購読リストに追加
        m_subscriptions.insert(topic, subscription);
        qInfo() << "トピックを購読開始しました : " << topic;
    }

    // 接続を切断する
    void disconnect()
    {
        if (m_client->state() == QMqttClient::Connected) {
            m_client->disconnectFromHost();
        }
    }

signals:
    // メッセージ受信時のシグナル
    void connected();
    void messageReceived(const QString &topic, const QByteArray &message);

private slots:
    // MQTT接続状態が変化した時のハンドラ
    void handleConnectionStateChange()
    {
        switch (m_client->state()) {
        case QMqttClient::Connected:
            qInfo() << "MQTTブローカーに接続しました";
            emit connected();  // 接続成功時にシグナルを発行
            break;
        case QMqttClient::Disconnected:
            qInfo() << "MQTTブローカーから切断されました";

            // 切断時に再接続を試みる
            QTimer::singleShot(5000, this, [this]() {
                qInfo() << "再接続を試みます...";
                this->connectToHost(m_host, m_port, m_client->username(), m_client->password());
            });

            break;
        case QMqttClient::Connecting:
            qInfo() << "MQTTブローカーに接続中...";
            break;
        default:
            qWarning() << "不明な接続状態です: " << m_client->state();
            break;
        }
    }

    // エラー発生時のハンドラ
    void handleError(QMqttClient::ClientError error)
    {
        QString errorMsg;
        switch (error) {
        case QMqttClient::NoError:
            return;
        case QMqttClient::InvalidProtocolVersion:
            errorMsg = "無効なプロトコルバージョンです";
            break;
        case QMqttClient::IdRejected:
            errorMsg = "クライアントIDが拒否されました";
            break;
        case QMqttClient::ServerUnavailable:
            errorMsg = "サーバが利用できません";
            break;
        case QMqttClient::BadUsernameOrPassword:
            errorMsg = "ユーザ名またはパスワードが無効です";
            break;
        case QMqttClient::NotAuthorized:
            errorMsg = "認証されていません";
            break;
        case QMqttClient::TransportInvalid:
            errorMsg = "トランスポートが無効です";
            break;
        case QMqttClient::ProtocolViolation:
            errorMsg = "プロトコル違反が発生しました";
            break;
        case QMqttClient::UnknownError:
            errorMsg = "不明なエラーが発生しました";
            break;
        default:
            errorMsg = "予期せぬエラーが発生しました";
            break;
        }

        qCritical() << "MQTTエラー : " << errorMsg;

        // エラー発生時は再接続を試みる
        if (m_client->state() != QMqttClient::Connected) {
            QTimer::singleShot(5000, this, [this]() {
                qInfo() << "再接続を試みます...";
                this->connectToHost(m_host, m_port, m_client->username(), m_client->password());
            });
        }
    }

    // メッセージ受信時のハンドラ
    void handleMessage(const QMqttMessage &message)
    {
        // シグナルを発行してアプリケーションに通知
        emit messageReceived(message.topic().name(), message.payload());
    }
};

#endif // SUBSCRIBER_H
