#ifndef POPUPREMINDER_H
#define POPUPREMINDER_H

#include "RemindersDialog.h"
#include "EditReminderDialog.h"
#include "AsteriskManager.h"
#include "Global.h"
#include "ChooseNumber.h"

#include <QDialog>
#include <QDateTime>
#include <QTimer>
#include <QPointer>

namespace Ui {
class PopupReminder;
}

class PopupReminder : public QDialog
{
    Q_OBJECT

public slots:
    void receiveData(bool update);

private:
    struct PopupReminderInfo
    {
        QString text;
        RemindersDialog* remindersDialog;
        QString my_number;
        QString name;
        QString number;
        QStringList numbers;
        QString id;
        QString group_id;
        QDateTime dateTime;
        QString note;
        QString call_id;
        bool active;
    };

public:
    PopupReminder(const PopupReminderInfo& pri, QWidget* parent = 0);
    ~PopupReminder();

    static void showReminder(RemindersDialog* remindersDialog, const QString& number, const QString& id, const QDateTime& dateTime, const QString& note);
    static void closeAll();

private slots:
    void closeAndDestroy();
    void onOpenAccess();
    void onCall();
    void onTimer();
    void onClosePopup();
    void onSelectTime(qint32 index);

    bool isInternalPhone(QString* str);

    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

private:
    Ui::PopupReminder* ui;

    QSqlDatabase db;
    QSqlDatabase dbCalls = QSqlDatabase::database("Calls");

    QRegularExpression hrefRegExp = QRegularExpression("(https?:\\/\\/\\S+)");

    QPointer<EditReminderDialog> editReminderDialog;
    QPointer<ChooseNumber> chooseNumber;

    qint32 m_nStartPosX, m_nStartPosY, m_nTaskbarPlacement;
    qint32 m_nCurrentPosX, m_nCurrentPosY;
    qint32 m_nIncrement;

    bool m_bAppearing;

    QPoint position;

    QTimer m_timer;

    PopupReminderInfo m_pri;

    static QList<PopupReminder*> m_PopupReminders;
};

#endif // POPUPREMINDER_H