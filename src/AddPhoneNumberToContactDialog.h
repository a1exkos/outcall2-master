#ifndef ADDPHONENUMBERTOCONTACTDIALOG_H
#define ADDPHONENUMBERTOCONTACTDIALOG_H

#include <QDialog>
#include <QSqlQueryModel>
#include <QHeaderView>
#include <QBoxLayout>
#include <QTableView>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QValidator>
#include <QPointer>

namespace Ui {
class AddPhoneNumberToContactDialog;
}

class AddPhoneNumberToContactDialog : public QDialog
{
    Q_OBJECT

signals:
    void sendData(bool update);

public:
    explicit AddPhoneNumberToContactDialog(QWidget* parent = 0);
    ~AddPhoneNumberToContactDialog();

    void setPhoneNumber(const QString& phoneNumber);

private slots:
    void loadContacts();
    void currentIndexChanged();
    void addPhoneNumber(const QModelIndex& index);
    void on_searchButton_clicked();
    void searchFunction();

    void on_previousButton_clicked();
    void on_nextButton_clicked();
    void on_nextEndButton_clicked();
    void on_previousStartButton_clicked();
    void on_lineEdit_page_returnPressed();
    void on_lineEdit_returnPressed();

private:
    Ui::AddPhoneNumberToContactDialog* ui;

    QSqlDatabase m_db;

    QPointer<QSqlQueryModel> m_queryModel;

    QString m_phoneNumber;
    QString m_page;
    QString m_go;

    bool m_filter;
};

#endif // ADDPHONENUMBERTOCONTACTDIALOG_H
