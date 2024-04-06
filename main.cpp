#include <string>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QHostAddress>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

namespace {

constexpr char kLogin[] = "login";
constexpr char kPassword[] = "password";
constexpr char kSorry[] = "Sorry, something went wrong.";

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTcpServer server;
    QObject::connect(&server, &QTcpServer::newConnection, [&server] {
        auto* socket = server.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        QObject::connect(socket, &QTcpSocket::readyRead, [socket] {
            QObject::disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);
            QString method, login, password;
            QDataStream stream(socket);
            stream >> method >> login >> password;
            password = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Algorithm::Sha512);

            auto db = QSqlDatabase::addDatabase("QSQLITE");
            db.setDatabaseName("users.db");
            db.open();
            QSqlQuery query;
            if (method == "registration") {
                query.prepare(QString("insert into users(%1, %2) values (?, ?);").arg(kLogin).arg(kPassword));
                query.addBindValue(QString("'%1'").arg(login).toStdString().c_str());
                query.addBindValue(QString("'%1'").arg(password).toStdString().c_str());
                if (query.exec()) {
                    stream << QString("Congratulations, %1! You were registered.").arg(login);
                } else {
                    stream << QString(kSorry);
                }
            } else if (method == "authorization") {
                query.prepare(QString("select %1 from users where %1 = ? and %2 = ?").arg(kLogin).arg(kPassword).toStdString().c_str());
                query.addBindValue(QString("'%1'").arg(login));
                query.addBindValue(QString("'%1'").arg(password));
                if (query.exec() && query.next()) {
                    stream << QString("Congratulations, %1! You were authorized.").arg(login);
                } else {
                    stream << QString(kSorry);
                }
            } else {
                stream << QString(kSorry);
            }
            db.close();
        });
    });
    server.listen(QHostAddress::Any, 12345);

    return app.exec();
}
