#include "SettingsDialog.h"
#include "NotesDialog.h"
#include "ui_NotesDialog.h"
#include "CallHistoryDialog.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QTime>
#include <QSqlQuery>


NotesDialog::NotesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NotesDialog)
{
    ui->setupUi(this);

    connect(ui->saveButton, &QAbstractButton::clicked, this, &NotesDialog::onSave);
    connect(ui->updateButton, &QAbstractButton::clicked, this, &NotesDialog::onUpdate);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    ui->tableView->verticalHeader()->setSectionsClickable(false);
    ui->tableView->horizontalHeader()->setSectionsClickable(false);

    settingsDialog = new SettingsDialog();
    my_number = settingsDialog->getExtension();
}

NotesDialog::~NotesDialog()
{
    delete ui;
    delete settingsDialog;
    deleteObjects();
}

void NotesDialog::setCallId(QString &uniqueid, QString &state_call)
{
    callId = uniqueid;
    state = state_call;
    loadNotes();
}

void NotesDialog::loadNotes() {
    query = new QSqlQueryModel;
    query->setQuery("SELECT datetime, author, note FROM calls WHERE uniqueid = '" + callId + "' ORDER BY datetime DESC");
    query->setHeaderData(0, Qt::Horizontal, QObject::tr("Дата и время"));
    query->setHeaderData(1, Qt::Horizontal, tr("Автор"));
    query->setHeaderData(2, Qt::Horizontal, tr("Заметка"));
    ui->tableView->setModel(query);
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    if(state=="save_disable")
    {
        ui->label->setDisabled(true);
        ui->textEdit->setDisabled(true);
        ui->saveButton->setDisabled(true);
    }
}

void NotesDialog::onSave()
{
    QSqlDatabase db;
    QSqlQuery query(db);
    QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    if(ui->textEdit->toPlainText().isEmpty() || ui->textEdit->toPlainText()== 0 )
        return;

    query.prepare("SELECT entry_name FROM entry_phone WHERE entry_phone = " + my_number);
    query.exec();
    query.next();
    QString author = query.value(0).toString();

    query.prepare("INSERT INTO calls (uniqueid, datetime, note, author) VALUES(?, ?, ?, ?)");
    query.addBindValue(callId);
    query.addBindValue(dateTime);
    query.addBindValue(ui->textEdit->toPlainText());
    query.addBindValue(author);
    query.first();
    query.exec();

    if(state == "all")
        emit sendDataToAllCalls();
    else if(state == "missed")
       emit sendDataToMissed();
    else if(state == "received")
       emit sendDataToReceived();
    else if(state == "placed")
        emit sendDataToPlaced();

    close();
   // QMessageBox::information(this, trUtf8("Уведомление"), trUtf8("Заметка успешно добавлена!"), QMessageBox::Ok);
    destroy(true);
}

void NotesDialog::onUpdate() {
    deleteObjects();
    loadNotes();
}

void NotesDialog::deleteObjects() {
    delete query;
}