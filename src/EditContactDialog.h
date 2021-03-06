#ifndef EDITCONTACTDIALOG_H
#define EDITCONTACTDIALOG_H

#include "AddOrgToPerson.h"
#include "ChooseNumber.h"

#include <QDialog>
#include <QValidator>
#include <QLineEdit>
#include <QKeyEvent>
#include <QPointer>
#include <QTextCursor>

class ViewContactDialog;

namespace Ui {
class EditContactDialog;
}

class EditContactDialog : public QDialog
{
    Q_OBJECT

signals:
    void sendData(bool update, qint32 x, qint32 y);
    void sendDataUpdate(bool update);

public slots:
    void receiveOrg(const QString& id, const QString& name);
    void setPos(qint32 x, qint32 y);

public:
    explicit EditContactDialog(QWidget* parent = 0);
    ~EditContactDialog();

    void setValues(const QString& id);
    void hideBackButton();

private slots:
    void onSave();
    void onReturn();
    void onTextChanged();
    void onCursorPosChanged();
    void changeEntryType();
    void updatePhonesOrder();

    void on_addOrgButton_clicked();
    void on_deleteOrgButton_clicked();
    void on_phonesOrderButton_clicked();

    bool isPhone(QString* str);

    void keyPressEvent(QKeyEvent* event);
    void closeEvent(QCloseEvent* event);

    bool eventFilter(QObject*, QEvent* event);

private:
    Ui::EditContactDialog* ui;

    QSqlDatabase m_db;

    QPointer<AddOrgToPerson> m_addOrgToPerson;
    QPointer<ChooseNumber> m_chooseNumber;

    QList<QLineEdit*> m_phones;
    QList<QLineEdit*> m_phonesComments;

    QStringList m_oldPhones;
    QStringList m_oldComments;

    QMap<QString, QLineEdit*> m_managers;
    QMap<QString, QString> m_oldManagers;

    QMap<QString, qint32> m_move;

    QString m_contactId;
    QString m_orgId;

    QTextCursor m_textCursor;
};

#endif // EDITCONTACTDIALOG_H
