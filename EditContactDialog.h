#ifndef EDITCONTACTDIALOG_H
#define EDITCONTACTDIALOG_H

#include <QDialog>
#include <QValidator>

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
    ~EditContactDialog();

protected:
    void onSave();

private:
    Ui::EditContactDialog *ui;
    QValidator *validator;
    QString updateID;
    QString firstNumber;
    QString secondNumber;
    QString thirdNumber;
    QString fourthNumber;
    QString fifthNumber;
};

#endif // EDITCONTACTDIALOG_H