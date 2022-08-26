#include "./ui_process.h"
#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include <filesystem>

#include "shlguid.h"
#include "winnls.h"
#include "shobjidl.h"

MainWindow::MainWindow(uint32_t quan_res, QString session, QSqlDatabase& db, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->quan_res = quan_res;
    arr = new QPushButton[quan_res];
    for (uint32_t i = 0; i < quan_res; ++i) {
        arr[i].setMinimumHeight(200);
        arr[i].setMaximumHeight(200);
        arr[i].setMaximumWidth(400);
        arr[i].setAutoFillBackground(true);
        connect(arr + i, &QPushButton::clicked, [=](){ Process::handleClick(ui, i); });
    }
    Process::Init(this, db, session, quan_res, arr);
    auto field = ui->scrollAreaWidgetContents_2;
    field->installEventFilter(this);
    auto layout = new QGridLayout();
    field->setLayout(layout);
    connect(this, &MainWindow::destroyed, &Process::EndOfProgram);
}

void MainWindow::resetPalette(const QPalette& pal, const QString& text, int i, QString toolTip) {
    arr[i].setPalette(pal);
    arr[i].update();
    arr[i].setText(text);
    arr[i].setToolTip(toolTip);
}

void MainWindow::resetAct(QMenu* mn, bool f, bool s) {
    mn->actions()[0]->setEnabled(f);
    mn->actions()[1]->setEnabled(s);
}

bool MainWindow::eventFilter(QObject *target, QEvent *event) {
  if (event->type() == QEvent::Resize) {
      auto field = static_cast<QWidget *>(target);
      auto layout = static_cast<QGridLayout *>(field->layout());
      qDeleteAll(layout->children());
      int c, w = field->width();
      for (c = 1; w > 400 * c; ++c);
      for (int i = 0; i < quan_res; ++i) {
          layout->addWidget(arr + i, i / c, i % c);
      }
  }
  return false;
}

MainWindow::~MainWindow() {
    delete ui;
    Ui::mut1.lock();
    delete [] arr;
    Ui::mut1.unlock();
}

Process ** Process::wdg = nullptr;
uint32_t Process::quan_res = 0;
QPushButton* Process::arr = nullptr;
DBDriver* Process::dr = nullptr;
QTimer* Process::updView = nullptr;

Process::Process(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Process) {
    ui->setupUi(this);
    opened_proc = false;
    installEventFilter(this);
}

Process::~Process() {
    delete ui;
}

bool Process::eventFilter(QObject *target, QEvent *event) {
    if (event->type() == QEvent::Close)
        opened_proc = false;
    return false;
}

std::wstring findProgram(std::wstring path, QString& needed) {
    for (auto& p : std::filesystem::directory_iterator(path)) {
        auto name = p.path().wstring();
        if (p.is_regular_file()) {
            if (name == path + L"\\" + needed.toStdWString() + L".lnk")
                return name;
        } else if (p.is_directory()) {
            auto res = findProgram(name, needed);
            if (res != L"") return res;
        }
    }
    return L"";
}

std::wstring lnktopath(std::wstring lnk) {
    IPersistFile* pPersistFile=NULL;
    IShellLink* pLink=NULL;
    TCHAR szPath[MAX_PATH]={0};
    try {
        if (FAILED(CoInitialize(NULL)))
            throw "CoInitialize";
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_IShellLink, (void **) &pLink)))
            throw "CoCreateInstance";
        if (FAILED(pLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile)))
            throw "QueryInterface";
        #if defined(_UNICODE)
        if (FAILED(pPersistFile->Load(lnk.c_str(), STGM_READ)))
            throw "Load";
        #else
            LPWSTR pwszTemp = new WCHAR[_MAX_PATH];
            mbstowcs(pwszTemp, pszShortcut, _MAX_PATH);
            ppf->Load(pwszTemp, STGM_READ);
            delete[] pwszTemp;
        #endif
        if (FAILED(pLink->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI)))
            throw "Resolve";
        if (FAILED(pLink->GetPath(szPath, MAX_PATH, NULL, SLGP_RAWPATH)))
            throw "GetPath";
    }
    catch (TCHAR* szErr) {
        return L"";
        //std::cout << "error: " << szErr << std::endl;
    }
    if (pPersistFile)
        pPersistFile->Release();
    if (pLink)
        pLink->Release();
    CoFreeUnusedLibraries();
    CoUninitialize();
    return std::wstring(szPath);
}

bool Process::StartProcess() {
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    cur = q.dequeue();
    std::wstring tmp = findProgram(L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs", cur);
    if (tmp != L"") cur = QString::fromStdWString(lnktopath(tmp));
    return CreateProcessW((wchar_t *)cur.data(),
                  NULL,
                  NULL,
                  NULL,
                  FALSE,
                  0,
                  NULL,
                  NULL,
                  &si,
                  &pi);
}

void Process::StopProcess() {
    Ui::mut2.lock();
    if (q.empty() && cur == "") {
        Ui::mut2.unlock();
        return;
    }
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (q.size() && cur == q.last()) q.pop_back();
    cur = "";
    Ui::mut2.unlock();
}

void Process::InfoAboutProcess() {
    if (opened_proc) {
        activateWindow();
    } else {
        setWindowFlags(Qt::Tool);
        setWindowState(Qt::WindowActive);
        setFixedSize(600, 350);
        opened_proc = true;
        show();
    }
}

void Process::handleClick(Ui::MainWindow* b, int i) {
    QString str = b->lineEdit->text();
    if (str.size()) {
        Ui::mut2.lock();
        wdg[i]->q.push_back(str);
        b->lineEdit->clear();
        Ui::mut2.unlock();
    } else {
        wdg[i]->InfoAboutProcess();
    }
}

void SetColor(QPalette &pal, Qt::GlobalColor col, Qt::GlobalColor dark_col) {
    pal.setColor(QPalette::ButtonText, dark_col);
    pal.setColor(QPalette::Button, col);
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd, &dwProcessId);
    if (dwProcessId == (DWORD)lParam) {
        if (IsIconic(hWnd)) ShowWindow(hWnd, SW_NORMAL);
        if (IsWindow(hWnd)) SetForegroundWindow(hWnd);
    }
    return TRUE;
}

void Process::SetWindowActive(bool checked) {
    if (cur == "") return;
    EnumWindows(EnumWindowsProc, (LPARAM)pi.dwProcessId);
}

void Process::DeleteListItem(QListWidgetItem* item) {
    Ui::mut2.lock();
    for (int i = 0; i < q.size(); ++i) {
        if (ui->listWidget->item(i)->isSelected()) {
            q.remove(i);
            ui->listWidget->takeItem(i);
        }
    }
    Ui::mut2.unlock();
    ui->listWidget->update();
}

void Process::handleProcesses() {
    for (int i = 0, j; i < quan_res; ++i) {
        Ui::mut1.lock();
        QPalette pal = arr[i].palette();
        QString starr = arr[i].text();
        Ui::mut1.unlock();
        QString nstr = "";
        DWORD exit_code{};
        Ui::mut2.lock();
        if (wdg[i]->q.size()) {
            if (FALSE != ::GetExitCodeProcess(wdg[i]->pi.hProcess, ::std::addressof(exit_code)) && STILL_ACTIVE == exit_code) {
                SetColor(pal, Qt::green, Qt::darkGreen);
                nstr = wdg[i]->cur;
            } else {
                if (wdg[i]->StartProcess()) {
                    SetColor(pal, Qt::green, Qt::darkGreen);
                    nstr = wdg[i]->cur;
                } else {
                    SetColor(pal, Qt::red, Qt::darkRed);
                    nstr = wdg[i]->cur;
                    if (nstr.size())
                        wdg[i]->q.enqueue(wdg[i]->cur);
                }
            }
        } else {
            if (FALSE != ::GetExitCodeProcess(wdg[i]->pi.hProcess, ::std::addressof(exit_code)) && STILL_ACTIVE == exit_code) {
                nstr = wdg[i]->cur;
            } else {
                wdg[i]->cur = nstr;
                SetColor(pal, Qt::gray, Qt::darkGray);
            }
        }
        Ui::mut2.unlock();
        auto mn = wdg[i]->ui->pushButton->menu();
        bool f = true, s = true;
        if (pal.button().color() == Qt::red) {
            s = false;
        } else if (pal.button().color() == Qt::gray) {
            f = false;
            s = false;
        }
        emit wdg[i]->SendAct(mn, f, s);
        wdg[i]->ui->pushButton->setText(nstr);
        auto list_wdg = wdg[i]->ui->listWidget;
        Ui::mut2.lock();
        for (j = 0; j < wdg[i]->q.size(); ++j) {
            if (list_wdg->count() <= j) {
                list_wdg->addItem(wdg[i]->q[j]);
            }
            else if (list_wdg->item(j)->text() != wdg[i]->q[j]) {
                list_wdg->takeItem(j);
            }
        }
        while (wdg[i]->q.size() < list_wdg->count()) {
            list_wdg->takeItem(j);
        }
        Ui::mut2.unlock();
        if (pal != wdg[i]->palette() || nstr != starr) {
            emit wdg[i]->SendPalette(pal, nstr, i, wdg[i]->cur);
        }
    }
}

void Process::EndOfProgram() {
    dr->LastTime();
    if (dr->isDirty() && !dr->submitAll()) {
        qDebug() << "Some data won't be in database";
    }
    updView->stop();
    delete updView;
    for (uint32_t i = 0; i < quan_res; ++i) {
        if (wdg[i]->cur != "")
            wdg[i]->StopProcess();
        delete wdg[i];
    }
    delete wdg;
    auto query = new QSqlQuery(dr->database());
    query->prepare(QString("UPDATE Sessions"
                        "   SET (lrdate, lrtime) = (:curdate, :curtime)"
                        "   WHERE name_of_session = '%1'").arg(dr->tableName()));
    query->bindValue(":curdate", QDate::currentDate().toString());
    query->bindValue(":curtime", QTime::currentTime().toString());
    if (!query->exec())
        qDebug() << query->lastError().text();
    delete query;
    //delete dr;
}

void Process::Init(MainWindow* mw, QSqlDatabase& db, QString session, uint32_t quan_res, QPushButton* arr) {
    Process::quan_res = quan_res;
    dr = new DBDriver(db, session, quan_res);
    wdg = new Process*[quan_res];
    QVector<QPair<QQueue<QString> *, QString *>> vec;
    for (int i = 0; i < quan_res; ++i) {
        wdg[i] = new Process();
        vec.push_back(QPair<QQueue<QString> *, QString *>(&(wdg[i]->q), &(wdg[i]->cur)));
        wdg[i]->setWindowTitle(QString("Инфо о процессе №%1").arg(i + 1));
        auto mn = new QMenu();
        mn->addAction(new QAction("Завершить процесс"));
        mn->addAction(new QAction("Показать окно"));
        wdg[i]->ui->pushButton->setMenu(mn);
        connect(mn->actions()[0], &QAction::triggered, wdg[i], &Process::StopProcess);
        connect(mn->actions()[1], &QAction::triggered, wdg[i], &Process::SetWindowActive);
        connect(wdg[i]->ui->listWidget, &QListWidget::itemDoubleClicked, wdg[i], &Process::DeleteListItem);
        connect(wdg[i], &Process::SendPalette, mw, &MainWindow::resetPalette);
        connect(wdg[i], &Process::SendAct, mw, &MainWindow::resetAct);
    }
    dr->FirstTime(vec);
    Process::arr = arr;
    updView = new QTimer();
    updView->setInterval(100);
    updView->callOnTimeout(&Process::handleProcesses);
    updView->start();
}

DBDriver::DBDriver(QSqlDatabase& db, QString session, uint32_t num) : QSqlTableModel(nullptr, db) {
    QSqlQuery q(db);
    updTimer.setInterval(2000);
    updTimer.callOnTimeout(this, &DBDriver::Handling);
    setTable(session);
    setEditStrategy(QSqlTableModel::OnRowChange);
    if (select())
        qDebug() << "Select success!!!";
    else
        qDebug() << "Select error:" << lastError().text();
    updTimer.start();
}

void DBDriver::FirstTime(QVector<QPair<QQueue<QString> *, QString *>>& ret_ans) {
    Ui::mut2.lock();
    queues = ret_ans;
    if (rowCount() != 0) {
        for (int i = 0, j; i < rowCount(); ++i) {
            for (j = 0; j < queues.size(); ++j) {
                QString help = record(i).value(j).toString();
                if (help != "") {
                    queues[j].first->enqueue(help);
                }
            }
        }
    }
    Ui::mut2.unlock();
}

void DBDriver::LastTime() {
    updTimer.stop();
    Handling();
}

void DBDriver::Handling() {
    long long newRowCount = 0, oldRowCount = rowCount();
    Ui::mut2.lock();
    for (auto [q, s] : queues)
        newRowCount = qMax(newRowCount, q->size() + (*s != ""));
    if (oldRowCount < newRowCount)
         insertRows(oldRowCount, newRowCount - oldRowCount);
    else {
        for (long long i = oldRowCount - 1; i >= newRowCount; --i) {
            deleteRowFromTable(i);
            if (isDirty() && !submit())
                qDebug() << lastError().text();
        }
        select();
    }
    bool changed, remain;
    QSqlRecord help;
    QSet<int> indexes;
    for (int i = 0, j; i < newRowCount; ++i) {
        help = record(i);
        changed = false;
        for (j = 0; j < queues.size(); ++j) {
            auto bruh = help.value(j);
            remain = help.isGenerated(j);
            if (i) {
                if (queues[j].first->size() >= i) {
                    help.setGenerated(j, (remain || queues[j].first->at(i - 1) != bruh));
                    help.setValue(j, queues[j].first->at(i - 1));
                } else {
                    help.setGenerated(j, (remain || "" != bruh));
                    help.setValue(j, "");
                }
            } else {
                help.setGenerated(j, (remain || (*(queues[j].second)) != bruh));
                help.setValue(j, *(queues[j].second));
            }
            changed |= help.isGenerated(j);
        }
        setRecord(i, help);
        if (changed == true)
            indexes.insert(i);
    }
    if (indexes.size()) qDebug() << indexes;
    Ui::mut2.unlock();
}

DBDriver::~DBDriver() {
    updTimer.stop();
    Handling();
}
