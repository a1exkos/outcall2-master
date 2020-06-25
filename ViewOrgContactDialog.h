#ifndef VIEWORGCONTACTDIALOG_H
#define VIEWORGCONTACTDIALOG_H

#include "ViewContactDialog.h"
#include "EditOrgContactDialog.h"
#include "ChooseNumber.h"
#include "SettingsDialog.h"
#include "Global.h"
#include "AddPersonToOrg.h"
#include "NotesDialog.h"

#include <QHeaderView>
#include <QSqlQueryModel>
#include <QScrollBar>
#include <QTableView>
#include <QList>
#include <QWidget>

namespace Ui {
class ViewOrgContactDialog;
}

class ViewOrgContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewOrgContactDialog(QWidget *parent = 0);
    void setOrgValuesContacts(QString &);
    void setOrgValuesCallHistory(QString &);
    enum Calls
    {
        MISSED = 0,
        RECIEVED = 1,
        PLACED = 2
    };
    void addCall(const QMap<QString, QVariant> &call, Calls calls);
    ~ViewOrgContactDialog();

signals:
    void sendData(bool);
    void sendNumber(QString &);

public slots:
    void receiveData(bool);
    void receiveNumber(QString &);
    void receivePersonID(QString &);

protected slots:
    void onUpdate();
    void onCall();
    void onEdit();
    void onComboBoxSelected();
    void on_lineEdit_returnPressed();
    void showCard(const QModelIndex &index);
    void viewMissedNotes(const QModelIndex &index);
    void viewRecievedNotes(const QModelIndex &index);
    void viewPlacedNotes(const QModelIndex &index);
    void onSectionClicked (int logicalIndex);
    void loadMissedCalls();
    void loadReceivedCalls();
    void loadPlacedCalls();
    void deleteObjects();
    void searchFunction();
    void updateCalls();

private slots:
    void on_pushButton_clicked();

    void on_addPersonToOrg_clicked();

private:
    Ui::ViewOrgContactDialog *ui;
    SettingsDialog *settingsDialog;
    NotesDialog *notesDialog;
    QSqlQuery *query;
    QValidator *validator;
    QHeaderView *m_horiz_header;
    AddPersonToOrg *addPersonToOrg;
    QString updateID;
    QString number;
    QString my_number;
    QString uniqueid;
    int count2 = 1;
    QString contact_number;
    QString firstNumber;
    QString secondNumber;
    QString thirdNumber;
    QString fourthNumber;
    QString fifthNumber;
    QSqlQueryModel *query_model;
    QSqlQueryModel *query1;
    QSqlQueryModel *query2;
    QSqlQueryModel *query3;
    ViewContactDialog *viewContactDialog;
    EditOrgContactDialog *editOrgContactDialog;
    ChooseNumber *chooseNumber;
    QString update;
    QString id;
    QString days;
    bool filter;
    int valueV;
    int valueH;
    QString state_call;
    QString entry_name;
    QString entry_phone;
    QString entry_comment;
    QString sort;
    QWidget* loadNote();
    QList<QWidget*> widgets;
    QList<QLabel*> notes;
    QList<QSqlQueryModel*> queries;
};

#endif // VIEWORGCONTACTDIALOG_H
