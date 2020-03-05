#ifndef VIEWCONTACTDIALOG_H
#define VIEWCONTACTDIALOG_H

#include "Global.h"
#include "SettingsDialog.h"
#include "EditContactDialog.h"
#include "ChooseNumber.h"

#include <QDialog>
#include <QSqlQueryModel>
#include <QTableView>
#include <QList>
#include <QWidget>

namespace Ui {
class ViewContactDialog;
}

class ViewContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewContactDialog(QWidget *parent = 0);
    void setValuesContacts(QString &);
    void setValuesCallHistory(QString &);
    enum Calls
    {
        MISSED = 0,
        RECIEVED = 1,
        PLACED = 2
    };
    void addCall(const QMap<QString, QVariant> &call, Calls calls);
    ~ViewContactDialog();

signals:
    void sendData(bool);
    void sendNumber(QString &);

public slots:
    void receiveData(bool);
    void receiveNumber(const QString &);

protected slots:
    void onCall();
    void onEdit();
    void loadMissedCalls();
    void loadReceivedCalls();
    void loadPlacedCalls();
    void deleteObjects();
    void updateCalls();

private:
    Ui::ViewContactDialog *ui;
    SettingsDialog *settingsDialog;
    ChooseNumber *chooseNumber;
    EditContactDialog *editContactDialog;
    QSqlQueryModel *query1;
    QValidator *validator;
    QString updateID;
    QString uniqueid;
    QString number;
    QString my_number;
    int count2 = 1;
    QString days;
    QString contact_number;
    QString firstNumber;
    QString secondNumber;
    QString thirdNumber;
    QString fourthNumber;
    QString fifthNumber;
    QWidget* loadNote();
    QList<QWidget*> widgets;
    QList<QLabel*> notes;
    QList<QSqlQueryModel*> queries;
};

#endif // VIEWCONTACTDIALOG_H
