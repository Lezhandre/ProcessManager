#include "greeting.h"
#include "mainwindow.h"

#include <QApplication>
#include <QInputDialog>

int main(int argc, char *argv[]) {
    uint32_t m = 0;
    QApplication a(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("proc_stat.db");
    if (!db.open()) {
        QMessageBox::critical(0,
        "Ошибка",
        "База данных не открылась",
        QMessageBox::Ok
        );
        return 0;
    }
    QSqlQuery query(db);
    if (!query.exec(
                "CREATE TABLE IF NOT EXISTS Sessions("
                "    lrdate DATE,"
                "    lrtime TIME,"
                "    name_of_session TEXT,"
                "    quantity_of_processes INTEGER"
                ")"
        )) {
        QMessageBox::critical(0,
            "Ошибка",
            "В базе данных нет раздела о сессиях",
            QMessageBox::Ok
            );
        qDebug() << query.lastError();
        return 0;
    }
    if (!query.exec(
                "SELECT * FROM Sessions"
                "   ORDER BY lrdate, lrtime DESC"
        )) {
        QMessageBox::critical(0,
        "Ошибка",
        "Нельзя получить доступ к сессиям",
        QMessageBox::Ok
        );
        qDebug() << query.lastError();
        return 0;
    }
    QList<QPair<QString, uint32_t>> sessionList;
    while (query.next()) {
        auto pair = QPair<QString, uint32_t>(query.value(2).toString(), query.value(3).toUInt());
        sessionList.append(pair);
    }
    Greeting g(&m, sessionList, &query);
    g.show();
    a.exec();
    if (!g.ok) return 0;
    QString session;
    if (g.sel_session != "") {
        session = g.sel_session.remove(' ').remove(':');
    } else {
        auto helpStr = QDate::currentDate().toString() + QTime::currentTime().toString();
        helpStr.remove(' ').remove(':');
        bool ok;
        session = QInputDialog::getText(nullptr, "Ввод",
                                                 "Введите имя сессии, не содержащее пробелов", QLineEdit::Normal,
                                                 helpStr, &ok);
        while (query.exec(QString("SELECT * FROM Sessions WHERE name_of_session = '%1'").arg(session)), query.next()) {
            session = QInputDialog::getText(nullptr, "Ввод",
                                                     "Введите имя сессии, которого нет в таблице", QLineEdit::Normal,
                                                     helpStr, &ok);
        }
        if (!ok) return 0;
        query.prepare(
            "INSERT INTO Sessions (lrdate, lrtime, name_of_session, quantity_of_processes)"
            "   VALUES (:curdate, :curtime, :session, :quantity)"
        );
        query.bindValue(":curdate", QDate::currentDate().toString());
        query.bindValue(":curtime", QTime::currentTime().toString());
        query.bindValue(":session", session);
        query.bindValue(":quantity", m);
        if (!query.exec()) {
            QMessageBox::critical(0,
            "Ошибка",
            "В базу данных нельзя записать новую сессию",
            QMessageBox::Ok
            );
            qDebug() << query.lastError();
            return 0;
        }
        QString bigTableCreate = "CREATE TABLE " + session + " (";
        for (uint32_t i = 0; i < m; ++i) {
            bigTableCreate += QString("proc%1 TEXT,").arg(i);
        }
        bigTableCreate[bigTableCreate.size() - 1] = ')';
        if (!query.exec(bigTableCreate)) {
            QMessageBox::critical(0,
            "Ошибка",
            "В базу данных нельзя добавить таблицу",
            QMessageBox::Ok
            );
            qDebug() << query.lastError();
            return 0;
        }
    }
    MainWindow w(m, session, db);
    w.show();
    return a.exec();
}

//
