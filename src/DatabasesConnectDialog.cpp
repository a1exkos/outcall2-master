/*
 * Класс служит для подключения к базам данных.
 */

#include "DatabasesConnectDialog.h"
#include "ui_DatabasesConnectDialog.h"
#include "Global.h"
#include "Outcall.h"

#include <QMessageBox>
#include <QSqlDatabase>

DatabasesConnectDialog::DatabasesConnectDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::DatabasesConnectDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() ^ Qt::WindowCloseButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QByteArray byte_password_1 = global::getSettingsValue("password_1", "settings").toByteArray();
    QString password_1 = QString(QByteArray::fromBase64(byte_password_1));
    ui->hostName_1->setText(global::getSettingsValue("hostName_1", "settings").toString());
    ui->databaseName_1->setText(global::getSettingsValue("databaseName_1", "settings").toString());
    ui->userName_1->setText(global::getSettingsValue("userName_1", "settings").toString());
    ui->password_1->setText(password_1);
    ui->port_1->setText(global::getSettingsValue("port_1", "settings").toString());

    QByteArray byte_password_2 = global::getSettingsValue("password_2", "settings").toByteArray();
    QString password_2 = QString(QByteArray::fromBase64(byte_password_2));
    ui->hostName_2->setText(global::getSettingsValue("hostName_2", "settings").toString());
    ui->databaseName_2->setText(global::getSettingsValue("databaseName_2", "settings").toString());
    ui->userName_2->setText(global::getSettingsValue("userName_2", "settings").toString());
    ui->password_2->setText(password_2);
    ui->port_2->setText(global::getSettingsValue("port_1", "settings").toString());

    connect(ui->saveButton, &QAbstractButton::clicked, this, &DatabasesConnectDialog::onSave);
    connect(ui->closeButton, &QAbstractButton::clicked, this, &DatabasesConnectDialog::close);
}

DatabasesConnectDialog::~DatabasesConnectDialog()
{
    delete ui;
}

/**
 * Выполняет проверку введенных данных и сохраняет их.
 */
void DatabasesConnectDialog::onSave()
{
    setSettingsDb();
    setSettingsDbCalls();

    if (m_state == "twoDbs")
    {
        openDb();
        openDbCalls();

        if (!m_db.isOpen() && !m_dbCalls.isOpen())
            MsgBoxError(tr("Подключение не создано!"));
        else if (!m_db.isOpen())
        {
            ui->tabWidget_2->setCurrentIndex(0);
            ui->tabWidget_2->setTabEnabled(1, false);

            MsgBoxError(tr("Подключение к базе контактов не создано!"));

            setSettingsDbCalls();
        }
        else if (!m_dbCalls.isOpen())
        {
            ui->tabWidget_2->setCurrentIndex(1);
            ui->tabWidget_2->setTabEnabled(0, false);

            MsgBoxError(tr("Подключение к базе звонков не создано!"));

            setSettingsDb();
        }
        else if (m_db.isOpen() && m_dbCalls.isOpen())
        {
            setSettingsDb();
            setSettingsDbCalls();

            MsgBoxInformation(tr("Подключение успешно создано!"));

            close();
        }
    }
    else if (m_state == "db")
    {
        openDb();

        if (!m_db.isOpen())
            MsgBoxError(tr("Подключение к базе контактов не создано!"));
        else
        {
            setSettingsDb();

            MsgBoxInformation(tr("Подключение успешно создано!"));

            close();
        }
    }
    else if (m_state == "dbCalls")
    {
        openDbCalls();

        if (!m_dbCalls.isOpen())
            MsgBoxError(tr("Подключение к базе звонков не создано!"));
        else
        {
            setSettingsDbCalls();

            MsgBoxInformation(tr("Подключение успешно создано!"));

            close();
        }
    }
}

/**
 * Выполняет сохранение в реестр данных с полей окна подключения к базе контактов.
 */
void DatabasesConnectDialog::setSettingsDb()
{
    global::setSettingsValue("hostName_1", ui->hostName_1->text(), "settings");
    global::setSettingsValue("databaseName_1",   ui->databaseName_1->text(), "settings");
    global::setSettingsValue("userName_1",   ui->userName_1->text(), "settings");
    QByteArray password_1;
    password_1.append(ui->password_1->text().toLatin1());
    global::setSettingsValue("password_1", password_1.toBase64(), "settings");
    global::setSettingsValue("port_1", ui->port_1->text(), "settings");
}

/**
 * Выполняет сохранение в реестр данных с полей окна подключения к базе звонков.
 */
void DatabasesConnectDialog::setSettingsDbCalls()
{
    global::setSettingsValue("hostName_2", ui->hostName_2->text(), "settings");
    global::setSettingsValue("databaseName_2", ui->databaseName_2->text(), "settings");
    global::setSettingsValue("userName_2", ui->userName_2->text(), "settings");
    QByteArray password_2;
    password_2.append(ui->password_2->text().toLatin1());
    global::setSettingsValue("password_2", password_2.toBase64(), "settings");
    global::setSettingsValue("port_2", ui->port_2->text(), "settings");
}

/**
 * Получает параметры баз данных из main.
 */
void DatabasesConnectDialog::setDatabases(const QSqlDatabase& db, const QSqlDatabase& dbCalls, const QString& state)
{
    m_state = state;

    if (m_state == "db")
    {
        ui->tabWidget_2->setCurrentIndex(0);
        ui->tabWidget_2->setTabEnabled(1, false);
    }
    else if (m_state == "dbCalls")
    {
        ui->tabWidget_2->setCurrentIndex(1);
        ui->tabWidget_2->setTabEnabled(0, false);
    }

    m_db = db;
    m_dbCalls = dbCalls;
}

/**
 * Выполняет установку параметров и открытие соединения с базой контактов.
 */
void DatabasesConnectDialog::openDb()
{
    m_db.setHostName(ui->hostName_1->text());
    m_db.setDatabaseName(ui->databaseName_1->text());
    m_db.setUserName(ui->userName_1->text());
    m_db.setPassword(ui->password_1->text());
    m_db.setPort(ui->port_1->text().toUInt());
    m_db.open();
}

/**
 * Выполняет установку параметров и открытие соединения с базой звонков.
 */
void DatabasesConnectDialog::openDbCalls()
{
    m_dbCalls.setHostName(ui->hostName_2->text());
    m_dbCalls.setDatabaseName(ui->databaseName_2->text());
    m_dbCalls.setUserName(ui->userName_2->text());
    m_dbCalls.setPassword(ui->password_2->text());
    m_dbCalls.setPort(ui->port_2->text().toUInt());
    m_dbCalls.open();
}

/**
 * Выполняет обработку нажатий клавиш.
 * Особая обработка для клавиши Enter.
 */
void DatabasesConnectDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return)
        onSave();
    else
        QDialog::keyPressEvent(event);
}
