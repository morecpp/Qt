#include "HttpClient.h"

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QHttpPart>
#include <QHttpMultiPart>

class HttpClientPrivate {
public:
    HttpClientPrivate(const QString &url);

    QString   url;    // 请求的 URL
    QUrlQuery params; // 请求的参数使用 Form 格式
    QString   json;   // 请求的参数使用 Json 格式
    QHash<QString, QString> headers; // 请求头
    QNetworkAccessManager  *manager;

    bool useJson; // 为 true 时请求使用 Json 格式传递参数，否则使用 Form 格式传递参数
    bool debug;   // 为 true 时输出请求的 URL 和参数

    // HTTP 请求的类型
    enum HttpMethod {
        GET, POST, PUT, DELETE
    };

    /**
     * @brief 执行请求的辅助函数
     * @param method         请求的类型
     * @param d              HttpClient 的辅助对象
     * @param successHandler 请求成功的回调 lambda 函数
     * @param errorHandler   请求失败的回调 lambda 函数
     * @param encoding       请求响应的编码
     */
    static void executeQuery(HttpMethod method, HttpClientPrivate *d,
                             std::function<void (const QString &)> successHandler,
                             std::function<void (const QString &)> errorHandler,
                             const char *encoding);

    /**
     * @brief 读取服务器响应的数据
     * @param reply 请求的 QNetworkReply 对象
     * @param encoding 请求响应的编码，默认使用 UTF-8
     * @return 服务器端响应的字符串
     */
    static QString readReply(QNetworkReply *reply, const char *encoding = "UTF-8");

    /**
     * @brief 使用用户设定的 URL、请求头等创建 Request
     * @param method 请求的类型
     * @param d      HttpClientPrivate 的对象
     * @return 返回可用于执行请求的 QNetworkRequest
     */
    static QNetworkRequest createRequest(HttpMethod method, HttpClientPrivate *d);
};

HttpClientPrivate::HttpClientPrivate(const QString &url) : url(url), manager(NULL), useJson(false), debug(false) {
}

// 注意: 不要在回调函数中使用 d，因为回调函数被调用时 HttpClient 对象很可能已经被释放掉了。
HttpClient::HttpClient(const QString &url) : d(new HttpClientPrivate(url)) {
    // qDebug().noquote() << "HttpClient";
}

HttpClient::~HttpClient() {
    // qDebug().noquote() << "~HttpClient";
    delete d;
}

HttpClient &HttpClient::manager(QNetworkAccessManager *manager) {
    d->manager = manager;
    return *this;
}

// 传入 debug 为 true 则使用 debug 模式，请求执行时输出请求的 URL 和参数等
HttpClient &HttpClient::debug(bool debug) {
    d->debug = debug;

    return *this;
}

// 添加 Form 格式参数
HttpClient &HttpClient::param(const QString &name, const QString &value) {
    d->params.addQueryItem(name, value);

    return *this;
}

// 添加 Json 格式参数
HttpClient &HttpClient::json(const QString &json) {
    d->useJson  = true;
    d->json = json;

    return *this;
}

// 添加访问头
HttpClient &HttpClient::header(const QString &header, const QString &value) {
    d->headers[header] = value;

    return *this;
}

// 执行 GET 请求
void HttpClient::get(std::function<void (const QString &)> successHandler,
                     std::function<void (const QString &)> errorHandler,
                     const char *encoding) {
    HttpClientPrivate::executeQuery(HttpClientPrivate::GET, d, successHandler, errorHandler, encoding);
}

// 执行 POST 请求
void HttpClient::post(std::function<void (const QString &)> successHandler,
                      std::function<void (const QString &)> errorHandler,
                      const char *encoding) {
    HttpClientPrivate::executeQuery(HttpClientPrivate::POST, d, successHandler, errorHandler, encoding);
}

// 执行 PUT 请求
void HttpClient::put(std::function<void (const QString &)> successHandler,
                     std::function<void (const QString &)> errorHandler,
                     const char *encoding) {
    HttpClientPrivate::executeQuery(HttpClientPrivate::PUT, d, successHandler, errorHandler, encoding);
}

// 执行 DELETE 请求
void HttpClient::remove(std::function<void (const QString &)> successHandler,
                        std::function<void (const QString &)> errorHandler,
                        const char *encoding) {
    HttpClientPrivate::executeQuery(HttpClientPrivate::DELETE, d, successHandler, errorHandler, encoding);
}

void HttpClient::download(const QString &destinationPath,
                          std::function<void ()> finishHandler,
                          std::function<void (const QString &)> errorHandler) {
    bool debug = d->debug;
    QFile *file = new QFile(destinationPath);

    if (file->open(QIODevice::WriteOnly)) {
        download([=](const QByteArray &data) {
            file->write(data);
        }, [=] {
            // 请求结束后释放文件对象.
            file->flush();
            file->close();
            file->deleteLater();

            if (debug) {
                qDebug().noquote() << QString("下载完成，保存到: %1").arg(destinationPath);
            }

            if (NULL != finishHandler) {
                finishHandler();
            }
        }, errorHandler);
    } else {
        // 打开文件出错
        if (debug) {
            qDebug().noquote() << QString("打开文件出错: %1").arg(destinationPath);
        }

        if (NULL != errorHandler) {
            errorHandler(QString("打开文件出错: %1").arg(destinationPath));
        }
    }
}

// 使用 GET 进行下载，当有数据可读取时回调 readyRead(), 大多数情况下应该在 readyRead() 里把数据保存到文件
void HttpClient::download(std::function<void (const QByteArray &)> readyRead,
                          std::function<void ()> finishHandler,
                          std::function<void (const QString &)> errorHandler) {
    bool internal = d->manager == NULL;
    QNetworkRequest request = HttpClientPrivate::createRequest(HttpClientPrivate::GET, d);
    QNetworkAccessManager *manager = internal ? new QNetworkAccessManager() : d->manager;
    QNetworkReply *reply = manager->get(request);

    // 有数据可读取时回调 readyRead()
    QObject::connect(reply, &QNetworkReply::readyRead, [=] {
        readyRead(reply->readAll());
    });

    // 请求结束
    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NoError && NULL != finishHandler) {
            finishHandler();
        }

        // 释放资源
        reply->deleteLater();
        if (internal) {
            manager->deleteLater();
        }
    });

    // 请求错误处理
    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=] {
        if (NULL != errorHandler) {
            errorHandler(reply->errorString());
        }
    });
}

void HttpClient::upload(const QString &path,
                        std::function<void (const QString &)> successHandler,
                        std::function<void (const QString &)> errorHandler,
                        const char *encoding) {
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QFile *file = new QFile(path);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    // 如果文件打开失败，则释放资源返回
    if(!file->open(QIODevice::ReadOnly)) {
        if (NULL != errorHandler) {
            errorHandler(QString("文件打开失败: %1").arg(file->errorString()));
            multiPart->deleteLater();
            return;
        }
    }

    // 表明是文件上传
    QString disposition = QString("form-data; name=\"file\"; filename=\"%1\"").arg(file->fileName());
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(disposition));
    part.setBodyDevice(file);
    multiPart->append(part);

    bool internal = d->manager == NULL;
    QNetworkRequest request = HttpClientPrivate::createRequest(HttpClientPrivate::GET, d);
    QNetworkAccessManager *manager = internal ? new QNetworkAccessManager() : d->manager;
    QNetworkReply *reply = manager->post(request, multiPart);

    // 请求结束时一次性读取所有响应数据
    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NoError && NULL != successHandler) {
            successHandler(HttpClientPrivate::readReply(reply, encoding)); // 成功执行
        }

        // 释放资源
        multiPart->deleteLater();
        reply->deleteLater();
        if (internal) {
            manager->deleteLater();
        }
    });

    // 请求错误处理
    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=] {
        if (NULL != errorHandler) {
            errorHandler(reply->errorString());
        }
    });
}

// 执行请求的辅助函数
void HttpClientPrivate::executeQuery(HttpMethod method, HttpClientPrivate *d,
                                     std::function<void (const QString &)> successHandler,
                                     std::function<void (const QString &)> errorHandler,
                                     const char *encoding) {
    // 如果不使用外部的 manager 则创建一个新的，在访问完成后会自动删除掉
    bool internal = d->manager == NULL;
    QNetworkRequest request = createRequest(method, d);
    QNetworkAccessManager *manager = internal ? new QNetworkAccessManager() : d->manager;
    QNetworkReply *reply = NULL;

    switch (method) {
    case HttpClientPrivate::GET:
        reply = manager->get(request);
        break;
    case HttpClientPrivate::POST:
        reply = manager->post(request, d->useJson ? d->json.toUtf8() : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpClientPrivate::PUT:
        reply = manager->put(request, d->useJson ? d->json.toUtf8() : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpClientPrivate::DELETE:
        reply = manager->deleteResource(request);
        break;
    }

    // 请求结束时一次性读取所有响应数据
    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NoError && NULL != successHandler) {
            successHandler(HttpClientPrivate::readReply(reply, encoding)); // 成功执行
        }

        // 释放资源
        reply->deleteLater();
        if (internal) {
            manager->deleteLater();
        }
    });

    // 请求错误处理
    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=] {
        if (NULL != errorHandler) {
            errorHandler(reply->errorString());
        }
    });
}

QString HttpClientPrivate::readReply(QNetworkReply *reply, const char *encoding) {
    QTextStream in(reply);
    QString result;
    in.setCodec(encoding);

    while (!in.atEnd()) {
        result += in.readLine();
    }

    return result;
}

QNetworkRequest HttpClientPrivate::createRequest(HttpMethod method, HttpClientPrivate *d) {
    bool get = method == HttpMethod::GET;

    // 如果是 GET 请求，并且参数不为空，则编码请求的参数，放到 URL 后面
    if (get && !d->params.isEmpty()) {
        d->url += "?" + d->params.toString(QUrl::FullyEncoded);
    }

    // 输出调试信息
    if (d->debug) {
        qDebug().noquote() << "网址:" << d->url;

        if (!get && d->useJson) {
            qDebug().noquote() << "参数:" << d->json;
        } else if (!get && !d->useJson) {
            qDebug().noquote() << "参数:" << d->params.toString();
        }
    }

    // 如果是 POST 请求，useJson 为 true 时添加 Json 的请求头，useJson 为 false 时添加 Form 的请求头
    if (!get && !d->useJson) {
        d->headers["Content-Type"] = "application/x-www-form-urlencoded";
    } else if (!get && d->useJson) {
        d->headers["Accept"] = "application/json; charset=utf-8";
        d->headers["Content-Type"] = "application/json";
    }

    // 把请求的头添加到 request 中
    QNetworkRequest request(QUrl(d->url));
    QHashIterator<QString, QString> iter(d->headers);
    while (iter.hasNext()) {
        iter.next();
        request.setRawHeader(iter.key().toUtf8(), iter.value().toUtf8());
    }

    return request;
}
