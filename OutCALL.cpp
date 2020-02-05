#include "OutCALL.h"
#include "DebugInfoDialog.h"
#include "SettingsDialog.h"
#include "ContactsDialog.h"
#include "CallHistoryDialog.h"
#include "Global.h"
#include "ContactManager.h"
#include "PlaceCallDialog.h"
#include "AsteriskManager.h"
#include "Notifier.h"
#include "PopupWindow.h"
#include "PopupHelloWindow.h"

#include <QMenu>
#include <QDebug>
#include <QTcpSocket>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QMessageBox>

OutCall::OutCall() :
    QWidget()
{
    m_systemTryIcon       = new QSystemTrayIcon(this);
    m_menu                = new QMenu(this);
    m_settingsDialog      = new SettingsDialog;
    m_contactsDialog      = new ContactsDialog;
    m_debugInfoDialog     = new DebugInfoDialog;
    m_callHistoryDialog   = new CallHistoryDialog;
    m_placeCallDialog     = new PlaceCallDialog;

    connect(m_systemTryIcon,    &QSystemTrayIcon::activated,            this, &OutCall::onActivated);
    connect(g_pAsteriskManager, &AsteriskManager::messageReceived,      this, &OutCall::onMessageReceived);
    connect(g_pAsteriskManager, &AsteriskManager::callDeteceted,        this, &OutCall::onCallDeteceted);
    connect(g_pAsteriskManager, &AsteriskManager::callReceived,         this, &OutCall::onCallReceived);
    connect(g_pAsteriskManager, &AsteriskManager::error,                this, &OutCall::displayError);
    connect(g_pAsteriskManager, &AsteriskManager::stateChanged,         this, &OutCall::onStateChanged);
    connect(&m_timer,           &QTimer::timeout,                       this, &OutCall::changeIcon);

    global::setSettingsValue("InstallDir", g_AppDirPath.replace("/", "\\"));

    createContextMenu();

    m_systemTryIcon->setContextMenu(m_menu);

    QString path(":/images/disconnected.png");
    m_systemTryIcon->setIcon(QIcon(path));

    automaticlySignIn();
}

OutCall::~OutCall()
{
    delete m_settingsDialog;
    delete m_debugInfoDialog;
    delete m_contactsDialog;
    delete m_callHistoryDialog;
    delete m_placeCallDialog;
}

void OutCall::createContextMenu()
{
    // Exit action
    QAction* exitAction = new QAction(tr("Выход"), m_menu);
    connect(exitAction, &QAction::triggered, this, &OutCall::close);

    // Sign In
    m_signIn  = new QAction(tr("Sign In"), m_menu);
    connect(m_signIn, &QAction::triggered, this, &OutCall::signInOut);

    // SettingsDialog
    QAction* settingsAction = new QAction(tr("Настройки"), m_menu);
    connect(settingsAction, &QAction::triggered, this, &OutCall::onSettingsDialog);

    QAction* debugInfoAction = new QAction(tr("Отладка"), m_menu);
    connect(debugInfoAction, &QAction::triggered, this, &OutCall::onDebugInfo);

    // ContactsDialog
    QAction* contactsInfoAction = new QAction(tr("Контакты"), m_menu);
    connect(contactsInfoAction, &QAction::triggered, this, &OutCall::onContactsInfo);

    // Call History
    QAction* callHistoryAction = new QAction(tr("История звонков"), m_menu);
    connect(callHistoryAction, &QAction::triggered, this, &OutCall::onCallHistory);

    // Place a Call
    m_placeCall = new QAction(tr("Позвонить"), 0);
    QFont font = m_placeCall->font();
    font.setBold(true);
    m_placeCall->setFont(font);
    connect(m_placeCall, &QAction::triggered, this, &OutCall::onPlaceCall);

    // Add actions
    m_menu->addSeparator();

    m_menu->addAction(settingsAction);
    m_menu->addAction(debugInfoAction);
    m_menu->addSeparator();

    m_menu->addAction(m_placeCall);
    m_menu->addAction(callHistoryAction);
    m_menu->addAction(contactsInfoAction);
    m_menu->addSeparator();

    m_menu->addAction(m_signIn);
    m_menu->addSeparator();

    m_menu->addAction(exitAction);
}

void OutCall::automaticlySignIn()
{
    bool autoSingIn = global::getSettingsValue("auto_sign_in", "general").toBool();
    if(autoSingIn)
        signInOut();
}

void OutCall::signInOut()
{
    if (m_signIn->text() == "Cancel Sign In")
    {
        g_pAsteriskManager->signOut();
        return;
    }

    if (g_pAsteriskManager->isSignedIn() == false)
    {
        QString server = global::getSettingsValue("servername", "settings").toString();
        QString port = global::getSettingsValue("port", "settings", "5038").toString();
        g_pAsteriskManager->signIn(server, port.toUInt());
    }
    else
    {
        g_pAsteriskManager->signOut();
    }
}

void OutCall::displayError(QAbstractSocket::SocketError socketError, const QString &msg)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        MsgBoxInformation(tr("Удаленный хост закрыл соединение."));
        break;
    case QAbstractSocket::HostNotFoundError:
        MsgBoxInformation(tr("Хост не был найден. Пожалуйста, проверьте имя хоста "
                             "и настройки порта."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        MsgBoxInformation(tr("The connection was refused by the peer. "
                             "Make sure the server is running, "
                             "and check that the host name and port "
                             "settings are correct."));
        break;
    default:
        MsgBoxInformation(msg);
    }
}

void OutCall::onMessageReceived(const QString &message)
{
    if (m_debugInfoDialog)
        m_debugInfoDialog->updateDebug(message);
}

void OutCall::onCallDeteceted(const QMap<QString, QVariant> &call, AsteriskManager::CallState state)
{
     //m_callHistoryDialog->addCall(call, (CallHistoryDialog::Calls)state);
//    if (state == 0 )
//    {
//        QMap<QString, QVariant> call1;

//        QSqlDatabase dbAsterisk = QSqlDatabase::database("Second");
//        QSqlQuery query(dbAsterisk);
//        query.prepare("SELECT date_time FROM cdr WHERE src = ? AND dst = ? ORDER BY date_time DESC");
//        query.addBindValue(call["from"]);
//        query.addBindValue(call["to"]);
//        query.exec();
//        query.first();
//        call1["from"] = call["from"];
//        call1["to"] = call["to"];
//        call1["date_time"] = query.value(0).toString();
//        m_callHistoryDialog->addCall(call1, (CallHistoryDialog::Calls)state);
//    }
//    QString state_call = "recieved";
//    m_callHistoryDialog->clear();
//    m_callHistoryDialog->loadCalls(state_call);
    qDebug() << "1243";
}

void OutCall::onCallReceived(const QMap<QString, QVariant> &call)
{
    QString from            = call.value("from").toString();
    QString callerIDName    = call.value("callerIDName").toString();

    QSqlDatabase db;
    QSqlQuery query(db);
    query.prepare("SELECT EXISTS(SELECT entry_name FROM entry WHERE id IN (SELECT entry_id FROM phone WHERE phone ="+from+"))");
    query.exec();
    query.first();
    if(query.value(0) != 0)
    {
        query.prepare("SELECT entry_name FROM entry WHERE id IN (SELECT entry_id FROM phone WHERE phone = "+from+")");
        query.exec();
        query.first();
        callerIDName = query.value(0).toString();
    }
    else
    {
        callerIDName = "Неизвестный";
    }

    PopupWindow::showCallNotification(from, QString("%1 (%2)").arg(callerIDName).arg(from));
}

void OutCall::onStateChanged(AsteriskManager::AsteriskState state)
{
    if (state == AsteriskManager::CONNECTED)
    {
        QString path(":/images/connected.png");
        m_systemTryIcon->setIcon(QIcon(path));

        m_signIn->setText(tr("Выйти из аккаунта"));

        PopupHelloWindow::showInformationMessage(tr(APP_NAME), tr("Вы успешно вошли"));
        m_systemTryIcon->setToolTip(tr(APP_NAME) + tr(" - ") + tr("Вы успешно вошли"));
        m_placeCall->setEnabled(true);
        m_timer.stop();
    }
    else if (state == AsteriskManager::CONNECTING)
    {
        m_signIn->setText(tr("Отменить вход"));
        m_systemTryIcon->setToolTip(tr(APP_NAME) + tr(" - ") + tr("Вход в аккаунт"));
        m_placeCall->setEnabled(false);
        m_timer.start(500);
    }
    else if (state == AsteriskManager::DISCONNECTED)
    {
        QString path(":/images/disconnected.png");
        m_systemTryIcon->setIcon(QIcon(path));

        m_signIn->setText(tr("&Войти в аккаунт"));
        m_systemTryIcon->setToolTip(tr(APP_NAME) + tr(" - ") + tr("Вы не вошли"));
        m_placeCall->setEnabled(false);
        m_timer.stop();
    }
    else if (state == AsteriskManager::AUTHENTICATION_FAILED)
    {
        QString path(":/images/started.png");
        m_systemTryIcon->setIcon(QIcon(path));

        PopupHelloWindow::showInformationMessage(tr(APP_NAME), tr("Ошибка аутентификации"));
        m_systemTryIcon->setToolTip(tr(APP_NAME) + tr(" - ") + tr("Не настроен"));
        m_signIn->setText(tr("&Войти в аккаунт"));
        m_placeCall->setEnabled(false);
        m_timer.stop();
    }
}

void OutCall::changeIcon()
{
    if (m_switch)
    {
        QString path(":/images/connected.png");
        m_systemTryIcon->setIcon(QIcon(path));
        m_switch = false;
    }
    else
    {
        QString path(":/images/disconnected.png");
        m_systemTryIcon->setIcon(QIcon(path));
        m_switch = true;
    }
}

void OutCall::onCallHistory()
{
    m_callHistoryDialog->show();
    m_callHistoryDialog->raise();
}

void OutCall::onSettingsDialog()
{
    SettingsDialog dialog;
    dialog.exec();
}

void OutCall::onDebugInfo()
{
    m_debugInfoDialog->show();
    m_debugInfoDialog->raise();
}

void OutCall::onPlaceCall()
{
    m_placeCallDialog->show();
    m_placeCallDialog->raise();
}

void OutCall::onContactsInfo()
{
    m_contactsDialog->show();
    m_contactsDialog->raise();
}

void OutCall::close()
{
    g_pAsteriskManager->signOut();
    QApplication::quit();
}

void OutCall::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        m_debugInfoDialog->activateWindow();
        m_settingsDialog->activateWindow();
        m_callHistoryDialog->activateWindow();
        m_contactsDialog->activateWindow();
        m_placeCallDialog->activateWindow();
    }
    else if (reason == QSystemTrayIcon::DoubleClick)
    {
        m_placeCallDialog->show();
        m_placeCallDialog->activateWindow();
    }
}

void OutCall::show()
{
    m_systemTryIcon->show();
}

