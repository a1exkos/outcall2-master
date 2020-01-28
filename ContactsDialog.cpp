#include "ContactsDialog.h"
#include "ui_ContactsDialog.h"
#include "AddContactDialog.h"
#include "AddOrgContactDialog.h"
#include "EditContactDialog.h"
#include "EditOrgContactDialog.h"

#include <QSqlQueryModel>
#include <QHeaderView>
#include <QTableView>
#include <QBoxLayout>
#include <QClipboard>
#include <QSqlDatabase>
#include <QHeaderView>
#include <QDebug>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlRelationalTableModel>
#include <QSqlTableModel>
#include <QGuiApplication>
#include <QScreen>
#include <QLabel>
#include <QAbstractProxyModel>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QModelIndex>

ContactsDialog::ContactsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactsDialog)
{
    ui->setupUi(this);

    this->showMaximized();
    ui->widget->move(0, 0);
    ui->widget->resize(QGuiApplication::screens().at(0)->geometry().width(), QGuiApplication::screens().at(0)->geometry().height());

    query1 = new QSqlQueryModel;
    query2 = new QSqlQueryModel;
    query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep GROUP BY ep.entry_id");
    query2->setQuery("SELECT entry_type FROM entry_phone GROUP BY entry_id");

    query1->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    query1->insertColumn(1);
    query1->setHeaderData(1, Qt::Horizontal, tr("Тип"));
    query1->setHeaderData(2, Qt::Horizontal, QObject::tr("ФИО / Название"));
    query1->setHeaderData(3, Qt::Horizontal, QObject::tr("Телефон"));
    query1->setHeaderData(4, Qt::Horizontal, QObject::tr("Город"));
    query1->setHeaderData(5, Qt::Horizontal, QObject::tr("Адрес"));
    query1->setHeaderData(6, Qt::Horizontal, QObject::tr("Email"));
    query1->setHeaderData(7, Qt::Horizontal, QObject::tr("VyborID"));
    query1->setHeaderData(8, Qt::Horizontal, QObject::tr("Заметка"));
    query1->insertColumn(9);
    query1->setHeaderData(9, Qt::Horizontal, tr("Редактирование"));
    ui->tableView->setModel(query1);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->addPersonButton, &QAbstractButton::clicked, this, &ContactsDialog::onAddPerson);
    connect(ui->addOrgButton, &QAbstractButton::clicked, this, &ContactsDialog::onAddOrg);
    connect(ui->updateButton, &QAbstractButton::clicked, this, &ContactsDialog::onUpdate);
    connect(ui->tableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));
    connect(ui->tableView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(showCard(const QModelIndex &)));

    for (int row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        ui->tableView->setIndexWidget(query1->index(row_index, 1), addImageLabel(row_index));
        ui->tableView->setIndexWidget(query1->index(row_index, 9), createEditButton(row_index));
    }

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());
    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);

    onComboBoxSelected();
    //ui->tableView->setSortingEnabled(true);
    //ui->tableView->sortByColumn(2, Qt::AscendingOrder);
    //comboBox->model()->sort ( int column, Qt::SortOrder order = Qt::AscendingOrder )

    tmpHeaderView = ui->tableView->horizontalHeader();//#include <QHeaderView>
    //tmpHeaderView->setClickable(true);//#include <QHeaderView>???
    ui->tableView->setSortingEnabled(true);//#include <QHeaderView>

    counter = 0;
    update = "default";
}

ContactsDialog::~ContactsDialog()
{
    delete query1;
    delete query2;
    delete ui;
}

void ContactsDialog::showCard(const QModelIndex &index)
{
    QString updateID = query1->data(query1->index(index.row(), 0)).toString();
    int row = ui->tableView->currentIndex().row();
    if (query2->data(query2->index(row, 0)).toString() == "person")
    {
         viewContactDialog = new ViewContactDialog;
         viewContactDialog->setValuesContacts(updateID);
         viewContactDialog->exec();
         viewContactDialog->deleteLater();
    }
        else
        {
            viewOrgContactDialog = new ViewOrgContactDialog;
            viewOrgContactDialog->setOrgValuesContacts(updateID);
            viewOrgContactDialog->exec();
            viewOrgContactDialog->deleteLater();
        }
}

void ContactsDialog::setSortingEnabled()
{
    ui->tableView->horizontalHeader()->setSortIndicatorShown(true);

     if (true)
     {
         disconnect(ui->tableView->horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(selectColumn(int)));

         connect(ui->tableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));

         ui->tableView->sortByColumn(ui->tableView->horizontalHeader()->sortIndicatorSection());
         update = "sort";
         onUpdate();
     }
     else
     {

         connect(ui->tableView->horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(selectColumn(int)));

         disconnect(ui->tableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));
         update = "sort";
         onUpdate();
     }
}

void ContactsDialog::deleteObjects()
{
    for (int i = 0; i < widgets.size(); ++i)
    {
        widgets[i]->deleteLater();
    }
    qDeleteAll(layouts);
    qDeleteAll(labels);
    qDeleteAll(buttons);
    widgets.clear();
    layouts.clear();
    labels.clear();
    buttons.clear();
}

void ContactsDialog::onUpdate()
{
    if (update == "default")
    {
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep GROUP BY ep.entry_id");
        query2->setQuery("SELECT entry_type FROM entry_phone GROUP BY entry_id");
    }

    deleteObjects();

    query1->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    query1->insertColumn(1);
    query1->setHeaderData(1, Qt::Horizontal, tr("Тип"));
    query1->setHeaderData(2, Qt::Horizontal, QObject::tr("ФИО / Название"));
    query1->setHeaderData(3, Qt::Horizontal, QObject::tr("Телефон"));
    query1->setHeaderData(4, Qt::Horizontal, QObject::tr("Город"));
    query1->setHeaderData(5, Qt::Horizontal, QObject::tr("Адрес"));
    query1->setHeaderData(6, Qt::Horizontal, QObject::tr("Email"));
    query1->setHeaderData(7, Qt::Horizontal, QObject::tr("VyborID"));
    query1->setHeaderData(8, Qt::Horizontal, QObject::tr("Заметка"));
    query1->insertColumn(9);
    query1->setHeaderData(9, Qt::Horizontal, tr("Редактирование"));
    ui->tableView->setModel(NULL);
    ui->tableView->setModel(query1);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    for (int row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        ui->tableView->setIndexWidget(query1->index(row_index, 1), addImageLabel(row_index));
        ui->tableView->setIndexWidget(query1->index(row_index, 9), createEditButton(row_index));
    }

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());
    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);

    update = "default";
}

void ContactsDialog::onTableClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        QString cellText = index.data().toString();
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(cellText);
    }
}

void ContactsDialog::onEdit()
{
    QString updateID = sender()->property("updateID").toString();
    int row_index = sender()->property("row_index").toInt();
    if (query2->data(query2->index(row_index, 0)).toString() == "person")
    {
        editContactDialog = new EditContactDialog;
        editContactDialog->setWindowTitle("Редактирование физ. лица");
        editContactDialog->setValuesContacts(updateID);
        editContactDialog->exec();
        editContactDialog->deleteLater();
    }
    else
    {
        editOrgContactDialog = new EditOrgContactDialog;
        editOrgContactDialog->setWindowTitle("Редактирование организации");
        editOrgContactDialog->setOrgValuesContacts(updateID);
        editOrgContactDialog->exec();
        editOrgContactDialog->deleteLater();
    }
}

void ContactsDialog::onAddPerson()
{
    addContactDialog = new AddContactDialog;
    addContactDialog->setWindowTitle("Добавление физ. лица");
    addContactDialog->exec();
    addContactDialog->deleteLater();
}

void ContactsDialog::onAddOrg()
{
    addOrgContactDialog = new AddOrgContactDialog;
    addOrgContactDialog->setWindowTitle("Добавление организации");
    addOrgContactDialog->exec();
    addOrgContactDialog->deleteLater();
}

QWidget* ContactsDialog::addImageLabel(int &row_index)
{
    QWidget* wgt = new QWidget;
    QBoxLayout* l = new QHBoxLayout;
    QLabel *imageLabel = new QLabel(wgt);
    l->addWidget(imageLabel);
    if (query2->data(query2->index(row_index, 0)).toString() == "person")
    {
        imageLabel->setPixmap(QPixmap("D:/person.png").scaled(30, 30, Qt::KeepAspectRatio));
    }
    else
    {
        imageLabel->setPixmap(QPixmap("D:/org.png").scaled(30, 30, Qt::KeepAspectRatio));
    }
    wgt->setLayout(l);
    widgets.append(wgt);
    layouts.append(l);
    labels.append(imageLabel);
    return wgt;
}

QWidget* ContactsDialog::createEditButton(int &row_index)
{
    QWidget* wgt = new QWidget;
    QBoxLayout* l = new QHBoxLayout;
    QPushButton* editButton = new QPushButton("Редактировать");
    connect(editButton, SIGNAL(clicked(bool)), SLOT(onEdit()));
    editButton->setFocusPolicy(Qt::NoFocus);
    QString updateID = query1->data(query1->index(row_index, 0)).toString();
    editButton->setProperty("updateID", updateID);
    editButton->setProperty("row_index", row_index);
    l->addWidget(editButton);
    wgt->setLayout(l);
    widgets.append(wgt);
    layouts.append(l);
    buttons.append(editButton);
    return wgt;
}

void ContactsDialog::onComboBoxSelected()
{
    ui->comboBox->addItem("Поиск по ФИО / названию");
    ui->comboBox->addItem("Поиск по номеру телефона");
    ui->comboBox->addItem("Поиск по заметке");
}

void ContactsDialog::on_lineEdit_returnPressed()
{
    update = "filter";
    QComboBox::AdjustToContents;

    if(ui->comboBox->currentText() == "Поиск по ФИО / названию")
    {
        QString entry_name = ui->lineEdit->text();
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep WHERE entry_name LIKE '%" + entry_name + "%' GROUP BY ep.entry_id");
        query2->setQuery("SELECT entry_type FROM entry_phone WHERE entry_name LIKE '%" + entry_name + "%' GROUP BY entry_id");

        onUpdate();
    }

    if(ui->comboBox->currentText() == "Поиск по номеру телефона")
    {
        QString entry_phone = ui->lineEdit->text();
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep WHERE entry_phone LIKE '%" + entry_phone + "%' GROUP BY ep.entry_id");
        query2->setQuery("SELECT entry_type FROM entry_phone WHERE entry_phone LIKE '%" + entry_phone + "%' GROUP BY entry_id");

        onUpdate();
//        if(ui->sortButton->clicked(true) == setProperty(clicked(),true)){
//                on_sortButton_clicked();
//    }
    }

    if(ui->comboBox->currentText() == "Поиск по заметке")
    {
        QString entry_comment = ui->lineEdit->text();
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep WHERE entry_comment LIKE '%" + entry_comment + "%' GROUP BY ep.entry_id");
        query2->setQuery("SELECT entry_type FROM entry_phone WHERE entry_comment LIKE '%" + entry_comment + "%' GROUP BY entry_id");

        onUpdate();
    }
}

void ContactsDialog::on_sortButton_clicked()
{
    update = "sort";

    if (counter == 0)
    {
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep GROUP BY ep.entry_id ORDER BY ep.entry_name");
        query2->setQuery("SELECT entry_type FROM entry_phone GROUP BY entry_id ORDER BY entry_name");
        onUpdate();
        counter++;
    }
    else
    {
        query1->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_city, ep.entry_address, ep.entry_email, ep.entry_vybor_id, ep.entry_comment FROM entry_phone ep GROUP BY ep.entry_id ORDER BY ep.entry_name DESC");
        query2->setQuery("SELECT entry_type FROM entry_phone GROUP BY entry_id ORDER BY entry_name DESC");
        onUpdate();
        counter = 0;
    }

//    treeView = new QTreeView;
//    MyItemModel *sourceModel = new MyItemModel(this);
//    proxyModel = new QSortFilterProxyModel(this);
//    proxyModel->setSourceModel(sourceModel);
//    treeView->setModel(proxyModel);

// ui->tableView->sortByColumn(2,Qt::AscendingOrder);

// sourceModel->(2, Qt::AscendingOrder);

// ui->tableView->setSortingEnabled(true);

// ui->tableView->sortByColumn(2,Qt::AscendingOrder);
}


