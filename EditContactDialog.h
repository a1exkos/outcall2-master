#ifndef EDITCONTACTDIALOG_H
#define EDITCONTACTDIALOG_H

#include <QDialog>
#include <QValidator>
#include <QStringList>

namespace Ui {
class EditContactDialog;
}

class EditContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditContactDialog(QWidget *parent = 0);
    void setValuesContacts(QString &);
    void setValuesCallHistory(QString &);
    void setValuesPopupWindow(QString &number);
    ~EditContactDialog();

protected slots:
    void onSave();
    void onComboBoxSelected();

private:
    Ui::EditContactDialog *ui;
    QValidator *validator;
    QValidator *validator2;
    QString updateID;
    QString firstNumber;
    QString secondNumber;
    QString thirdNumber;
    QString fourthNumber;
    QString fifthNumber;
    QString number;
    QStringList numbers;

signals:
    void sendData(bool);
};

#endif // EDITCONTACTDIALOG_H
