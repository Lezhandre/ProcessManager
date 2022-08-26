#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QPushButton>
#include <QQueue>
#include <QString>
#include <QLayout>
#include <QRect>
#include <QPalette>
#include <QWidget>
#include <QString>
#include <QMutex>
#include <QListWidgetItem>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlRecord>
#include <QtSql>

#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
    class Process;
    static QMutex mut1, mut2;
}
QT_END_NAMESPACE

class MainWindow;
class Process;
class DBDriver;

class MainWindow : public QMainWindow {
    Q_OBJECT

//signals:

public slots:
    void resetPalette(const QPalette&, const QString&, int, QString);
    void resetAct(QMenu*, bool, bool);

public:
    MainWindow(uint32_t, QString, QSqlDatabase&, QWidget* = nullptr);
    ~MainWindow();
    bool eventFilter(QObject *, QEvent *);

private:
    Ui::MainWindow *ui;
    uint32_t quan_res;
    QPushButton* arr;
    //QThread* thr;
};

class Process : public QWidget {
    Q_OBJECT
    static Process ** wdg;
    static uint32_t quan_res;
    static QPushButton* arr;
    static DBDriver* dr;
    static QTimer* updView;

signals:
    void SendPalette(const QPalette&, const QString&, int, QString);
    void SendAct(QMenu*, bool, bool);

public slots:
    bool StartProcess();
    void StopProcess();
    void InfoAboutProcess();
    void SetWindowActive(bool);
    void DeleteListItem(QListWidgetItem*);
    static void handleClick(Ui::MainWindow*, int);
    static void handleProcesses();
    static void EndOfProgram();

public:
    static void Init(MainWindow*, QSqlDatabase&, QString, uint32_t, QPushButton*);

    explicit Process(QWidget *parent = nullptr);
    ~Process();

    bool eventFilter(QObject *, QEvent *);

private:
    Ui::Process *ui;
    PROCESS_INFORMATION pi;
    QQueue<QString> q;
    QString cur;
    bool opened_proc;
};

class DBDriver : public QSqlTableModel {
    Q_OBJECT

public:
    DBDriver(QSqlDatabase&, QString, uint32_t);
    ~DBDriver();

    void FirstTime(QVector<QPair<QQueue<QString>*, QString*>>&);
    void LastTime();

private slots:
    void Handling();

private:
    QVector<QPair<QQueue<QString>*, QString*>> queues;
    QTimer updTimer;
};

#endif // MAINWINDOW_H
