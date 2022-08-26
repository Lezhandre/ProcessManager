#ifndef GREETING_H
#define GREETING_H

#include <QWidget>
#include <QErrorMessage>
#include <QMessageBox>
#include <QBoxLayout>

#include "mainwindow.h"

namespace Ui {
    class Greeting;
}

class Greeting : public QWidget
{
    Q_OBJECT

public slots:
    void SetValue();

public:
    explicit Greeting(uint32_t*, QList<QPair<QString, uint32_t>>, QSqlQuery*, QWidget *parent = nullptr);
    ~Greeting();

private:
    void SetSession(QString, uint32_t);
    void DelSession(QString, QPushButton*);

public:
    QString sel_session;
    bool ok;

private:
    Ui::Greeting *ui;
    uint32_t* m;
    QSqlQuery* query;
};

#endif // GREETING_H
