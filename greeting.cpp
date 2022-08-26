#include "greeting.h"
#include "ui_greeting.h"

Greeting::Greeting(uint32_t* m, QList<QPair<QString, uint32_t>> sessionList, QSqlQuery * query, QWidget *parent) :
    QWidget(parent),
    ok(false),
    ui(new Ui::Greeting),
    m(m),
    query(query) {
    ui->setupUi(this);
    ui->spinBox->setMaximum(QThread::idealThreadCount());
    auto layout = new QVBoxLayout();
    layout->setSpacing(0);
    for (auto session : sessionList) {
        auto sesbtn = new QPushButton(session.first, this);
        auto mn = new QMenu();
        mn->addAction(new QAction("Запустить сессию"));
        mn->addAction(new QAction("Удалить сессию"));
        sesbtn->setMenu(mn);
        connect(mn->actions()[0], &QAction::triggered, this, [=](){ SetSession(sesbtn->text(), session.second); });
        connect(mn->actions()[1], &QAction::triggered, this, [=](){ DelSession(sesbtn->text(), sesbtn); });
        layout->addWidget(sesbtn);
        //connect(sesbtn, &QPushButton::clicked, this, [=](){ SetSession(sesbtn->text()); });
    }
    ui->listView->setLayout(layout);
    connect(ui->pushButton, &QPushButton::clicked, this, &Greeting::SetValue);
}

void Greeting::SetValue() {
    *m = ui->spinBox->value();
    ok = true;
    this->close();
}

void Greeting::SetSession(QString session, uint32_t num) {
    sel_session = session;
    *m = num;
    ok = true;
    this->close();
}

void Greeting::DelSession(QString session, QPushButton* btn) {
    query->exec(QString("DROP TABLE %1 IF EXISTS").arg(session));
    query->exec(QString("DELETE FROM Sessions WHERE name_of_session = '%1'").arg(session));
    ui->listView->layout()->removeWidget(btn);
    delete btn;
}

Greeting::~Greeting() {
    delete ui->listView->layout();
    delete ui;
}
