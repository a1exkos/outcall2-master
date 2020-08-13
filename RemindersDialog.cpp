#include "RemindersDialog.h"
#include "ui_RemindersDialog.h"

#include "PopupReminder.h"
#include "PopupNotification.h"
#include "Global.h"
#include "QSqlQueryModelReminders.h"

#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QSqlQuery>
#include <QLabel>
#include <QRegularExpression>

#define TIME_TO_UPDATE 5000 // msec

RemindersDialog::RemindersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RemindersDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    QRegularExpression regExp("^[0-9]*$");
    validator = new QRegularExpressionValidator(regExp, this);
    ui->lineEdit_page->setValidator(validator);

    ui->tableView->verticalHeader()->setSectionsClickable(false);
    ui->tableView->horizontalHeader()->setSectionsClickable(false);
    ui->tableView_2->verticalHeader()->setSectionsClickable(false);
    ui->tableView_2->horizontalHeader()->setSectionsClickable(false);
    ui->tableView_3->verticalHeader()->setSectionsClickable(false);
    ui->tableView_3->horizontalHeader()->setSectionsClickable(false);

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged()));
    connect(ui->addReminderButton, &QPushButton::clicked, this, &RemindersDialog::onAddReminder);
    connect(ui->tableView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(onEditReminder(const QModelIndex &)));
    connect(ui->tableView_2, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(onEditReminder(const QModelIndex &)));
    connect(ui->tableView_3, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(onEditReminder(const QModelIndex &)));
    connect(&timer, &QTimer::timeout, this, &RemindersDialog::onTimer);

    ui->tableView->setStyleSheet("QTableView { selection-color: black; selection-background-color: #18B7FF; }");
    ui->tableView_2->setStyleSheet("QTableView { selection-color: black; selection-background-color: #18B7FF; }");
    ui->tableView_3->setStyleSheet("QTableView { selection-color: black; selection-background-color: #18B7FF; }");

    ui->comboBox_list->setVisible(false);

    my_number = global::getSettingsValue(global::getExtensionNumber("extensions"), "extensions_name").toString();

    ui->tabWidget->setCurrentIndex(0);

    languages = global::getSettingsValue("language", "settings").toString();

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("UPDATE reminders SET completed = true WHERE phone_from <> ? AND phone_to = ? AND active = false");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    resizeCells = true;

    go="default";
    page = "1";

    loadRelevantReminders();

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_from <> ? AND phone_to = ? AND viewed = false ORDER BY id DESC");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    if (query.next())
        oldReceivedReminders = query.value(0).toInt();

    QList<QString> ids;
    QList<QDateTime> dateTimes;
    QList<QString> notes;

    query.prepare("SELECT id, datetime, content FROM reminders WHERE phone_to = '" + my_number + "' AND datetime > '" + QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss") + "' AND active IS TRUE");
    query.exec();

    while (query.next())
    {
        ids.append(query.value(0).value<QString>());
        dateTimes.append(query.value(1).value<QDateTime>());
        notes.append(query.value(2).value<QString>());
    }

    remindersThread = new QThread;
    remindersThreadManager = new RemindersThread(my_number, ids, dateTimes, notes);

    remindersThreadManager->moveToThread(remindersThread);

    connect(remindersThread, SIGNAL(started()), remindersThreadManager, SLOT(process()));
    connect(remindersThreadManager, SIGNAL(notify(QString, QDateTime, QString)), this, SLOT(onNotify(QString, QDateTime, QString)));
    connect(remindersThreadManager, SIGNAL(finished()), remindersThread, SLOT(quit()));
    connect(remindersThreadManager, SIGNAL(finished()), remindersThreadManager, SLOT(deleteLater()));
    connect(remindersThread, SIGNAL(finished()), remindersThread, SLOT(deleteLater()));

    remindersThread->start();

    timer.setInterval(TIME_TO_UPDATE);
    timer.start();
}

RemindersDialog::~RemindersDialog()
{
    remindersThread->requestInterruption();

    delete ui;
}

void RemindersDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("UPDATE reminders SET viewed = true WHERE phone_from <> ? AND phone_to = ? AND viewed = false");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    emit reminders(false);

    resizeCells = false;

    selectionRelevant.clear();
    selectionIrrelevant.clear();
    selectionDelegated.clear();

    onUpdate();
}

void RemindersDialog::closeEvent(QCloseEvent *event)
{
    QDialog::closeEvent(event);

    QDialog::clearFocus();

    ui->tabWidget->setCurrentIndex(0);

    go = "default";
    page = "1";
}

void RemindersDialog::onTimer()
{
    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_from <> ? AND phone_to = ? AND viewed = false ORDER BY id DESC");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    int newReceivedReminders = 0;

    if (query.next())
        newReceivedReminders = query.value(0).toInt();

    if (newReceivedReminders > oldReceivedReminders)
    {
        emit reminders(true);

        query.prepare("SELECT id, phone_from, content FROM reminders WHERE phone_from <> ? AND phone_to = ? AND viewed = false ORDER BY id DESC LIMIT 0,?");
        query.addBindValue(my_number);
        query.addBindValue(my_number);
        query.addBindValue(newReceivedReminders - oldReceivedReminders);
        query.exec();

        while (query.next())
            PopupNotification::showNotification(this, query.value(0).toString(), query.value(1).toString(), query.value(2).toString());
    }
    else
        resizeCells = false;

    oldReceivedReminders = newReceivedReminders;

    go = "default";

    onUpdate();

    sendNewValues();
}

void RemindersDialog::deleteObjects()
{
    if (ui->tabWidget->currentIndex() == 0)
    {
        selectionRelevant = ui->tableView->selectionModel()->selectedRows();

        for (int i = 0; i < widgetsRelevant.size(); ++i)
            widgetsRelevant[i]->deleteLater();

        for (int i = 0; i < layoutsRelevant.size(); ++i)
            layoutsRelevant[i]->deleteLater();

        for (int i = 0; i < labelsRelevant.size(); ++i)
            labelsRelevant[i]->deleteLater();

        for (int i = 0; i < boxesRelevant.size(); ++i)
            boxesRelevant[i]->deleteLater();

        for (int i = 0; i < queriesRelevant.size(); ++i)
            queriesRelevant[i]->deleteLater();

        queriesRelevant.clear();
        widgetsRelevant.clear();
        layoutsRelevant.clear();
        boxesRelevant.clear();
        labelsRelevant.clear();
    }
    if (ui->tabWidget->currentIndex() == 1)
    {
        selectionIrrelevant = ui->tableView_2->selectionModel()->selectedRows();

        for (int i = 0; i < widgetsIrrelevant.size(); ++i)
            widgetsIrrelevant[i]->deleteLater();

        for (int i = 0; i < layoutsIrrelevant.size(); ++i)
            layoutsIrrelevant[i]->deleteLater();

        for (int i = 0; i < labelsIrrelevant.size(); ++i)
            labelsIrrelevant[i]->deleteLater();

        for (int i = 0; i < boxesIrrelevant.size(); ++i)
            boxesIrrelevant[i]->deleteLater();

        for (int i = 0; i < queriesIrrelevant.size(); ++i)
            queriesIrrelevant[i]->deleteLater();

        queriesIrrelevant.clear();
        widgetsIrrelevant.clear();
        layoutsIrrelevant.clear();
        boxesIrrelevant.clear();
        labelsIrrelevant.clear();
    }
    if (ui->tabWidget->currentIndex() == 2)
    {
        selectionDelegated = ui->tableView_3->selectionModel()->selectedRows();

        for (int i = 0; i < widgetsDelegated.size(); ++i)
            widgetsDelegated[i]->deleteLater();

        for (int i = 0; i < layoutsDelegated.size(); ++i)
            layoutsDelegated[i]->deleteLater();

        for (int i = 0; i < labelsDelegated.size(); ++i)
            labelsDelegated[i]->deleteLater();

        for (int i = 0; i < boxesDelegated.size(); ++i)
            boxesDelegated[i]->deleteLater();

        for (int i = 0; i < queriesDelegated.size(); ++i)
            queriesDelegated[i]->deleteLater();

        queriesDelegated.clear();
        widgetsDelegated.clear();
        layoutsDelegated.clear();
        boxesDelegated.clear();
        labelsDelegated.clear();
    }
}

void RemindersDialog::sendNewValues()
{
    QList<QString> ids;
    QList<QDateTime> dateTimes;
    QList<QString> notes;

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT id, datetime, content FROM reminders WHERE phone_to = '" + my_number + "' AND datetime > '" + QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss") + "' AND active IS TRUE");
    query.exec();
    while (query.next())
    {
        ids.append(query.value(0).value<QString>());
        dateTimes.append(query.value(1).value<QDateTime>());
        notes.append(query.value(2).value<QString>());
    }

    remindersThreadManager->receiveNewValues(ids, dateTimes, notes);
}

void RemindersDialog::receiveData(bool updating)
{
    if (updating)
    {
        emit reminders(false);

        selectionRelevant.clear();
        selectionIrrelevant.clear();
        selectionDelegated.clear();

        go = "default";

        onUpdateTab();
    }
}

void RemindersDialog::loadRelevantReminders()
{
    if (!queriesRelevant.isEmpty())
        deleteObjects();

    query1 = new QSqlQueryModelReminders;
    query2 = new QSqlQueryModelReminders;

    queriesRelevant.append(query1);
    queriesRelevant.append(query2);

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_to = '" + my_number + "' AND active = true");
    query.exec();
    query.first();

    count = query.value(0).toInt();

    if (count <= ui->comboBox_list->currentText().toInt())
        pages = "1";
    else
    {
        remainder = count % ui->comboBox_list->currentText().toInt();
        if (remainder)
            remainder = 1;
        else
            remainder = 0;
        pages = QString::number(count / ui->comboBox_list->currentText().toInt() + remainder);
    }
    if (go == "previous" && page != "1")
        page = QString::number(page.toInt() - 1);
    else if (go == "previousStart" && page != "1")
        page = "1";
    else if (go == "next" && page.toInt() < pages.toInt())
        page = QString::number(page.toInt() + 1);
    else if (go == "next" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "nextEnd" && page.toInt() < pages.toInt())
        page = pages;
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > 0 && ui->lineEdit_page->text().toInt() <= pages.toInt())
        page = ui->lineEdit_page->text();
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > pages.toInt()) {}
    else if (go == "default" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "default" && page == "1")
        page = "1";

    ui->lineEdit_page->setText(page);
    ui->label_pages->setText(tr("из ") + pages);

    QString queryString1 = "SELECT id, phone_from, phone_to, datetime FROM reminders WHERE phone_to = '" + my_number + "' "
                                    "AND active = true ORDER BY datetime ASC LIMIT ";
    QString queryString2 = "SELECT active, viewed, completed, content FROM reminders WHERE phone_to = '" + my_number + "' "
                                    "AND active = true ORDER BY datetime ASC LIMIT ";

    if (ui->lineEdit_page->text() == "1")
    {
        queryString1.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
        queryString2.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
    }
    else
    {
        queryString1.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
        queryString2.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
    }

    query1->setQuery(queryString1);
    query2->setQuery(queryString2);

    query1->insertColumn(1);
    query1->setHeaderData(1, Qt::Horizontal, QObject::tr("Активно"));
    query1->setHeaderData(2, Qt::Horizontal, QObject::tr("От"));
    query1->setHeaderData(4, Qt::Horizontal, QObject::tr("Дата и время"));
    query1->insertColumn(5);
    query1->setHeaderData(5, Qt::Horizontal, QObject::tr("Содержание"));
    query1->insertColumn(6);
    query1->setHeaderData(6, Qt::Horizontal, QObject::tr("Выполнено"));

    ui->tableView->setModel(query1);

    query1->setParentTable(ui->tableView);

    for (int row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        if (query1->data(query1->index(row_index, 2), Qt::EditRole).toString() != query1->data(query1->index(row_index, 3), Qt::EditRole).toString())
        {
            ui->tableView->setIndexWidget(query1->index(row_index, 1), addWidgetActive());
            ui->tableView->setIndexWidget(query1->index(row_index, 6), addCheckBoxCompleted(row_index));
        }
        else
        {
            ui->tableView->setIndexWidget(query1->index(row_index, 1), addCheckBoxActive(row_index));
            ui->tableView->setIndexWidget(query1->index(row_index, 6), addWidgetCompleted());
        }

        QRegularExpressionMatchIterator hrefIterator = hrefRegExp.globalMatch(query2->data(query2->index(row_index, 3), Qt::EditRole).toString());

        if (hrefIterator.hasNext())
            ui->tableView->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, "URL"));
        else
            ui->tableView->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, ""));
    }

    ui->tableView->setColumnHidden(0, true);
    ui->tableView->setColumnHidden(3, true);

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());

    if (resizeCells)
    {
        ui->tableView->resizeRowsToContents();
        ui->tableView->resizeColumnsToContents();
    }

    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    if (!selectionRelevant.isEmpty())
        for (int i = 0; i < selectionRelevant.length(); ++i)
        {
            QModelIndex index = selectionRelevant.at(i);
            ui->tableView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

    resizeCells = true;
}

void RemindersDialog::loadIrrelevantReminders()
{
    if (!queriesIrrelevant.isEmpty())
        deleteObjects();

    query1 = new QSqlQueryModelReminders;
    query2 = new QSqlQueryModelReminders;

    queriesIrrelevant.append(query1);
    queriesIrrelevant.append(query2);

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_to = '" + my_number + "' AND active = false");
    query.exec();
    query.first();

    count = query.value(0).toInt();

    if (count <= ui->comboBox_list->currentText().toInt())
        pages = "1";
    else
    {
        remainder = count % ui->comboBox_list->currentText().toInt();
        if (remainder)
            remainder = 1;
        else
            remainder = 0;
        pages = QString::number(count / ui->comboBox_list->currentText().toInt() + remainder);
    }
    if (go == "previous" && page != "1")
        page = QString::number(page.toInt() - 1);
    else if (go == "previousStart" && page != "1")
        page = "1";
    else if (go == "next" && page.toInt() < pages.toInt())
        page = QString::number(page.toInt() + 1);
    else if (go == "next" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "nextEnd" && page.toInt() < pages.toInt())
        page = pages;
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > 0 && ui->lineEdit_page->text().toInt() <= pages.toInt())
        page = ui->lineEdit_page->text();
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > pages.toInt()) {}
    else if (go == "default" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "default" && page == "1")
        page = "1";

    ui->lineEdit_page->setText(page);
    ui->label_pages->setText(tr("из ") + pages);

    QString queryString1 = "SELECT id, phone_from, phone_to, datetime FROM reminders WHERE phone_to = '" + my_number + "' AND active = false ORDER BY datetime DESC LIMIT ";
    QString queryString2 = "SELECT active, viewed, completed, content FROM reminders WHERE phone_to = '" + my_number + "' AND active = false ORDER BY datetime DESC LIMIT ";

    if (ui->lineEdit_page->text() == "1")
    {
        queryString1.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
        queryString2.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
    }
    else
    {
        queryString1.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
        queryString2.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
    }

    query1->setQuery(queryString1);
    query2->setQuery(queryString2);

    query1->insertColumn(1);
    query1->setHeaderData(1, Qt::Horizontal, QObject::tr("Активно"));
    query1->setHeaderData(2, Qt::Horizontal, QObject::tr("От"));
    query1->setHeaderData(4, Qt::Horizontal, QObject::tr("Дата и время"));
    query1->insertColumn(5);
    query1->setHeaderData(5, Qt::Horizontal, QObject::tr("Содержание"));
    query1->insertColumn(6);
    query1->setHeaderData(6, Qt::Horizontal, QObject::tr("Выполнено"));

    ui->tableView_2->setModel(query1);

    query1->setParentTable(ui->tableView_2);

    for (int row_index = 0; row_index < ui->tableView_2->model()->rowCount(); ++row_index)
    {        
        if (query1->data(query1->index(row_index, 2), Qt::EditRole).toString() != query1->data(query1->index(row_index, 3), Qt::EditRole).toString())
        {
            ui->tableView_2->setIndexWidget(query1->index(row_index, 1), addWidgetActive());
            ui->tableView_2->setIndexWidget(query1->index(row_index, 6), addCheckBoxCompleted(row_index));
        }
        else
        {
            ui->tableView_2->setIndexWidget(query1->index(row_index, 1), addCheckBoxActive(row_index));
            ui->tableView_2->setIndexWidget(query1->index(row_index, 6), addWidgetCompleted());
        }

        QRegularExpressionMatchIterator hrefIterator = hrefRegExp.globalMatch(query2->data(query2->index(row_index, 3), Qt::EditRole).toString());

        if (hrefIterator.hasNext())
            ui->tableView_2->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, "URL"));
        else
            ui->tableView_2->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, ""));
    }

    ui->tableView_2->setColumnHidden(0, true);
    ui->tableView_2->setColumnHidden(3, true);

    ui->tableView_2->horizontalHeader()->setDefaultSectionSize(maximumWidth());

    if (resizeCells)
    {
        ui->tableView_2->resizeRowsToContents();
        ui->tableView_2->resizeColumnsToContents();
    }

    ui->tableView_2->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    if (!selectionIrrelevant.isEmpty())
        for (int i = 0; i < selectionIrrelevant.length(); ++i)
        {
            QModelIndex index = selectionIrrelevant.at(i);
            ui->tableView_2->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

    resizeCells = true;
}

void RemindersDialog::loadDelegatedReminders()
{
    if (!queriesDelegated.isEmpty())
        deleteObjects();

    query1 = new QSqlQueryModelReminders;
    query2 = new QSqlQueryModelReminders;

    queriesDelegated.append(query1);
    queriesDelegated.append(query2);

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_from = '" + my_number + "' AND phone_to <> '" + my_number + "'");
    query.exec();
    query.first();

    count = query.value(0).toInt();

    if (count <= ui->comboBox_list->currentText().toInt())
        pages = "1";
    else
    {
        remainder = count % ui->comboBox_list->currentText().toInt();
        if (remainder)
            remainder = 1;
        else
            remainder = 0;
        pages = QString::number(count / ui->comboBox_list->currentText().toInt() + remainder);
    }
    if (go == "previous" && page != "1")
        page = QString::number(page.toInt() - 1);
    else if (go == "previousStart" && page != "1")
        page = "1";
    else if (go == "next" && page.toInt() < pages.toInt())
        page = QString::number(page.toInt() + 1);
    else if (go == "next" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "nextEnd" && page.toInt() < pages.toInt())
        page = pages;
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > 0 && ui->lineEdit_page->text().toInt() <= pages.toInt())
        page = ui->lineEdit_page->text();
    else if (go == "enter" && ui->lineEdit_page->text().toInt() > pages.toInt()) {}
    else if (go == "default" && page.toInt() >= pages.toInt())
        page = pages;
    else if (go == "default" && page == "1")
        page = "1";

    ui->lineEdit_page->setText(page);
    ui->label_pages->setText(tr("из ") + pages);

    QString queryString1 = "SELECT id, phone_from, phone_to, datetime FROM reminders WHERE phone_from = '" + my_number + "' AND phone_to <> '" + my_number + "' ORDER BY datetime DESC LIMIT ";
    QString queryString2 = "SELECT active, viewed, completed, content FROM reminders WHERE phone_from = '" + my_number + "' AND phone_to <> '" + my_number + "' ORDER BY datetime DESC LIMIT ";

    if (ui->lineEdit_page->text() == "1")
    {
        queryString1.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
        queryString2.append("0, " + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
    }
    else
    {
        queryString1.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
        queryString2.append("" + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()
                                                 - ui->comboBox_list->currentText().toInt()) + " , "
                            + QString::number(ui->comboBox_list->currentText().toInt()));
    }

    query1->setQuery(queryString1);
    query2->setQuery(queryString2);

    query1->insertColumn(1);
    query1->setHeaderData(1, Qt::Horizontal, QObject::tr("Активно"));
    query1->setHeaderData(3, Qt::Horizontal, QObject::tr("Кому"));
    query1->setHeaderData(4, Qt::Horizontal, QObject::tr("Дата и время"));
    query1->insertColumn(5);
    query1->setHeaderData(5, Qt::Horizontal, QObject::tr("Содержание"));
    query1->insertColumn(6);
    query1->setHeaderData(6, Qt::Horizontal, QObject::tr("Просмотрено"));
    query1->insertColumn(7);
    query1->setHeaderData(7, Qt::Horizontal, QObject::tr("Выполнено"));

    ui->tableView_3->setModel(query1);

    query1->setParentTable(ui->tableView_3);

    for (int row_index = 0; row_index < ui->tableView_3->model()->rowCount(); ++row_index)
    {
        ui->tableView_3->setIndexWidget(query1->index(row_index, 1), addCheckBoxActive(row_index));
        ui->tableView_3->setIndexWidget(query1->index(row_index, 6), addCheckBoxViewed(row_index));
        ui->tableView_3->setIndexWidget(query1->index(row_index, 7), addCheckBoxCompleted(row_index));

        QRegularExpressionMatchIterator hrefIterator = hrefRegExp.globalMatch(query2->data(query2->index(row_index, 3), Qt::EditRole).toString());

        if (hrefIterator.hasNext())
            ui->tableView_3->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, "URL"));
        else
            ui->tableView_3->setIndexWidget(query1->index(row_index, 5), addWidgetContent(row_index, ""));
    }

    ui->tableView_3->setColumnHidden(0, true);
    ui->tableView_3->setColumnHidden(2, true);

    ui->tableView_3->horizontalHeader()->setDefaultSectionSize(maximumWidth());

    if (resizeCells)
    {
        ui->tableView_3->resizeRowsToContents();
        ui->tableView_3->resizeColumnsToContents();
    }

    ui->tableView_3->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    if (!selectionDelegated.isEmpty())
        for (int i = 0; i < selectionDelegated.length(); ++i)
        {
            QModelIndex index = selectionDelegated.at(i);
            ui->tableView_3->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

    resizeCells = true;
}

void RemindersDialog::onAddReminder()
{
    addReminderDialog = new AddReminderDialog;
    connect(addReminderDialog, SIGNAL(sendData(bool)), this, SLOT(receiveData(bool)));
    addReminderDialog->show();
    addReminderDialog->setAttribute(Qt::WA_DeleteOnClose);
}

void RemindersDialog::onEditReminder(const QModelIndex &index)
{
    if (ui->tabWidget->currentIndex() == 1 && query1->data(query1->index(index.row(), 2), Qt::EditRole).toString() != query1->data(query1->index(index.row(), 3), Qt::EditRole).toString())
        return;
    else if (ui->tabWidget->currentIndex() == 0 && query1->data(query1->index(index.row(), 2), Qt::EditRole).toString() != query1->data(query1->index(index.row(), 3), Qt::EditRole).toString())
    {
        QSqlDatabase db;
        QSqlQuery query(db);

        query.prepare("UPDATE reminders SET viewed = true WHERE id = ?");
        query.addBindValue(query1->data(query1->index(index.row(), 0), Qt::EditRole).toString());
        query.exec();

        emit reminders(false);
    }

    QString id = query1->data(query1->index(index.row(), 0), Qt::EditRole).toString();
    QDateTime dateTime = query1->data(query1->index(index.row(), 4), Qt::EditRole).toDateTime();
    QString note = query2->data(query2->index(index.row(), 3), Qt::EditRole).toString();

    editReminderDialog = new EditReminderDialog;
    editReminderDialog->setValuesReminders(id, dateTime, note);
    connect(editReminderDialog, SIGNAL(sendData(bool)), this, SLOT(receiveData(bool)));
    editReminderDialog->show();
    editReminderDialog->setAttribute(Qt::WA_DeleteOnClose);
}

void RemindersDialog::changeState()
{
    QCheckBox* checkBox = sender()->property("checkBox").value<QCheckBox*>();
    QString id = sender()->property("id").value<QString>();
    QString column = sender()->property("column").value<QString>();
    QDateTime dateTime = sender()->property("dateTime").value<QDateTime>();

    if (!checkBox->isChecked() && dateTime < QDateTime::currentDateTime() && (ui->tabWidget->currentIndex() == 1 || ui->tabWidget->currentIndex() == 2) && column == "active")
    {
        checkBox->setChecked(false);

        QMessageBox::critical(this, QObject::tr("Ошибка"), QObject::tr("Указано прошедшее время!"), QMessageBox::Ok);
    }
    else
    {
        QSqlDatabase db;
        QSqlQuery query(db);

        if (checkBox->isChecked() && column == "active")
        {
            checkBox->setChecked(false);

            query.prepare("UPDATE reminders SET active = false WHERE id = ?");
            query.addBindValue(id);
            query.exec();

            emit reminders(false);
        }
        else if (!checkBox->isChecked() && ui->tabWidget->currentIndex() == 1 && column == "active")
        {
            checkBox->setChecked(true);

            query.prepare("UPDATE reminders SET active = true WHERE id = ?");
            query.addBindValue(id);
            query.exec();

            emit reminders(false);
        }
        else if (!checkBox->isChecked() && ui->tabWidget->currentIndex() == 2 && column == "active")
        {
            checkBox->setChecked(true);

            query.prepare("UPDATE reminders SET active = true, viewed = false, completed = false WHERE id = ?");
            query.addBindValue(id);
            query.exec();
        }
        else if (checkBox->isChecked() && column == "viewed")
        {
            checkBox->setChecked(true);
        }
        else if (!checkBox->isChecked() && column == "viewed")
        {
            checkBox->setChecked(false);
        }
        else if (checkBox->isChecked() && ui->tabWidget->currentIndex() == 1 && column == "completed")
        {
            checkBox->setChecked(true);
        }
        else if (!checkBox->isChecked() && ui->tabWidget->currentIndex() == 0 && column == "completed")
        {
            checkBox->setChecked(true);

            query.prepare("UPDATE reminders SET active = false, viewed = true, completed = true WHERE id = ?");
            query.addBindValue(id);
            query.exec();

            emit reminders(false);
        }
        else if (checkBox->isChecked() && ui->tabWidget->currentIndex() == 2 && column == "completed")
        {
            checkBox->setChecked(true);
        }
        else if (!checkBox->isChecked() && ui->tabWidget->currentIndex() == 2 && column == "completed")
        {
            checkBox->setChecked(false);
        }

        resizeCells = false;

        go = "default";

        onUpdate();
    }
}

QWidget* RemindersDialog::addWidgetContent(int row_index, QString url)
{
    QWidget* wgt = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    QLabel* contentLabel = new QLabel(wgt);

    layout->addWidget(contentLabel);

    QString content = query2->data(query2->index(row_index, 3), Qt::EditRole).toString();

    if (url == "URL")
    {
        QRegularExpressionMatchIterator hrefIterator = hrefRegExp.globalMatch(content);
        QStringList hrefs;

        while (hrefIterator.hasNext())
        {
            QRegularExpressionMatch match = hrefIterator.next();
            QString href = match.captured(1);

            if (!hrefs.contains(href))
                hrefs << href;
        }

        for (int i = 0; i < hrefs.length(); ++i)
            content.replace(QRegularExpression("(^|\\s)" + QRegularExpression::escape(hrefs.at(i)) + "(\\s|$)"), QString(" <a href='" + hrefs.at(i) + "'>" + hrefs.at(i) + "</a> "));
    }

    contentLabel->setText(content);
    contentLabel->setOpenExternalLinks(true);
    contentLabel->setWordWrap(true);

    wgt->setLayout(layout);

    if (ui->tabWidget->currentIndex() == 0)
    {
        widgetsRelevant.append(wgt);
        layoutsRelevant.append(layout);
        labelsRelevant.append(contentLabel);
    }
    else if (ui->tabWidget->currentIndex() == 1)
    {
        widgetsIrrelevant.append(wgt);
        layoutsIrrelevant.append(layout);
        labelsIrrelevant.append(contentLabel);
    }
    else if (ui->tabWidget->currentIndex() == 2)
    {
        widgetsDelegated.append(wgt);
        layoutsDelegated.append(layout);
        labelsDelegated.append(contentLabel);
    }

    return wgt;
}

QWidget* RemindersDialog::addWidgetActive()
{
    QWidget* wgt = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    QLabel* imageLabel = new QLabel(wgt);

    layout->addWidget(imageLabel, 0, Qt::AlignCenter);

    imageLabel->setPixmap(QPixmap(":/images/incomingNotification.png").scaled(15, 15, Qt::IgnoreAspectRatio));

    wgt->setLayout(layout);

    if (ui->tabWidget->currentIndex() == 0)
    {
        widgetsRelevant.append(wgt);
        layoutsRelevant.append(layout);
        labelsRelevant.append(imageLabel);
    }
    else if (ui->tabWidget->currentIndex() == 1)
    {
        widgetsIrrelevant.append(wgt);
        layoutsIrrelevant.append(layout);
        labelsIrrelevant.append(imageLabel);
    }

    return wgt;
}

QWidget* RemindersDialog::addCheckBoxActive(int row_index)
{
    QWidget* wgt = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    QCheckBox* checkBox = new QCheckBox(wgt);

    layout->addWidget(checkBox, 0, Qt::AlignCenter);

    if (query2->data(query2->index(row_index, 0), Qt::EditRole) == true)
        checkBox->setChecked(true);
    else
        checkBox->setChecked(false);

    wgt->setLayout(layout);

    if (ui->tabWidget->currentIndex() == 0)
    {
        widgetsRelevant.append(wgt);
        layoutsRelevant.append(layout);
        boxesRelevant.append(checkBox);
    }
    else if (ui->tabWidget->currentIndex() == 1)
    {
        widgetsIrrelevant.append(wgt);
        layoutsIrrelevant.append(layout);
        boxesIrrelevant.append(checkBox);
    }

    QString id = query1->data(query1->index(row_index, 0), Qt::EditRole).toString();
    QString column = "active";
    QDateTime dateTime = query1->data(query1->index(row_index, 4), Qt::EditRole).toDateTime();

    connect(checkBox, SIGNAL(pressed()), this, SLOT(changeState()));
    checkBox->setProperty("checkBox", QVariant::fromValue(checkBox));
    checkBox->setProperty("id", QVariant::fromValue(id));
    checkBox->setProperty("column", QVariant::fromValue(column));
    checkBox->setProperty("dateTime", QVariant::fromValue(dateTime));

    return wgt;
}

QWidget* RemindersDialog::addCheckBoxViewed(int row_index)
{
    QWidget* wgt = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    QCheckBox* checkBox = new QCheckBox(wgt);

    layout->addWidget(checkBox, 0, Qt::AlignCenter);

    if (query2->data(query2->index(row_index, 1), Qt::EditRole) == true)
        checkBox->setChecked(true);
    else
        checkBox->setChecked(false);

    wgt->setLayout(layout);

    widgetsDelegated.append(wgt);
    layoutsDelegated.append(layout);
    boxesDelegated.append(checkBox);

    QString id = query1->data(query1->index(row_index, 0), Qt::EditRole).toString();
    QString column = "viewed";
    QDateTime dateTime = query1->data(query1->index(row_index, 4), Qt::EditRole).toDateTime();

    connect(checkBox, SIGNAL(pressed()), this, SLOT(changeState()));
    checkBox->setProperty("checkBox", QVariant::fromValue(checkBox));
    checkBox->setProperty("id", QVariant::fromValue(id));
    checkBox->setProperty("column", QVariant::fromValue(column));
    checkBox->setProperty("dateTime", QVariant::fromValue(dateTime));

    return wgt;
}

QWidget* RemindersDialog::addWidgetCompleted()
{
    QWidget* wgt = new QWidget;

    if (ui->tabWidget->currentIndex() == 0)
        widgetsRelevant.append(wgt);
    else if (ui->tabWidget->currentIndex() == 1)
        widgetsIrrelevant.append(wgt);

    return wgt;
}

QWidget* RemindersDialog::addCheckBoxCompleted(int row_index)
{
    QWidget* wgt = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    QCheckBox* checkBox = new QCheckBox(wgt);

    layout->addWidget(checkBox, 0, Qt::AlignCenter);

    if (query2->data(query2->index(row_index, 2), Qt::EditRole) == true)
        checkBox->setChecked(true);
    else
        checkBox->setChecked(false);

    wgt->setLayout(layout);

    if (ui->tabWidget->currentIndex() == 0)
    {
        widgetsRelevant.append(wgt);
        layoutsRelevant.append(layout);
        boxesRelevant.append(checkBox);
    }
    else if (ui->tabWidget->currentIndex() == 1)
    {
        widgetsIrrelevant.append(wgt);
        layoutsIrrelevant.append(layout);
        boxesIrrelevant.append(checkBox);
    }

    QString id = query1->data(query1->index(row_index, 0), Qt::EditRole).toString();
    QString column = "completed";
    QDateTime dateTime = query1->data(query1->index(row_index, 4), Qt::EditRole).toDateTime();

    connect(checkBox, SIGNAL(pressed()), this, SLOT(changeState()));
    checkBox->setProperty("checkBox", QVariant::fromValue(checkBox));
    checkBox->setProperty("id", QVariant::fromValue(id));
    checkBox->setProperty("column", QVariant::fromValue(column));
    checkBox->setProperty("dateTime", QVariant::fromValue(dateTime));

    return wgt;
}

void RemindersDialog::onTabChanged()
{
    go = "default";
    page = "1";

    onUpdateTab();
}

void RemindersDialog::onUpdateTab()
{
    selectionRelevant.clear();
    selectionIrrelevant.clear();
    selectionDelegated.clear();

    ui->tableView->clearSelection();
    ui->tableView_2->clearSelection();
    ui->tableView_3->clearSelection();

    if (ui->tabWidget->currentIndex() == 0)
        loadRelevantReminders();
    else if (ui->tabWidget->currentIndex() == 1)
        loadIrrelevantReminders();
    else if (ui->tabWidget->currentIndex() == 2)
        loadDelegatedReminders();
}

void RemindersDialog::onUpdate()
{
    if (ui->tabWidget->currentIndex() == 0)
        loadRelevantReminders();
    else if (ui->tabWidget->currentIndex() == 1)
        loadIrrelevantReminders();
    else if (ui->tabWidget->currentIndex() == 2)
        loadDelegatedReminders();
}

void RemindersDialog::onNotify(QString reminderId, QDateTime reminderDateTime, QString reminderNote)
{
    PopupReminder::showReminder(this, my_number, reminderId, reminderDateTime, reminderNote);
}

void RemindersDialog::on_previousButton_clicked()
{
    go = "previous";

    onUpdateTab();
}

void RemindersDialog::on_nextButton_clicked()
{
    go = "next";

    onUpdateTab();
}

void RemindersDialog::on_previousStartButton_clicked()
{
    go = "previousStart";

    onUpdateTab();
}

void RemindersDialog::on_nextEndButton_clicked()
{
    go = "nextEnd";

    onUpdateTab();;
}

void RemindersDialog::on_lineEdit_page_returnPressed()
{
    go = "enter";

    onUpdateTab();
}


