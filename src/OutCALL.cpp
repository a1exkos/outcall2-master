/*
 * Класс служит для управления основной логикой приложения.
 */

#include "OutCALL.h"

#include "DebugInfoDialog.h"
#include "SettingsDialog.h"
#include "ContactsDialog.h"
#include "CallHistoryDialog.h"
#include "Global.h"
#include "PlaceCallDialog.h"
#include "AsteriskManager.h"
#include "PopupWindow.h"
#include "PopupHelloWindow.h"
#include "RemindersDialog.h"
#include "PopupReminder.h"
#include "PopupNotification.h"
#include "InternalContactsDialog.h"

#include <QMenu>
#include <QApplication>
#include <QSqlDatabase>
#include <QProcess>
#include <QPainter>
#include <QPixmap>

OutCall::OutCall() :
    QWidget()
{
    m_systemTrayIcon         = new QSystemTrayIcon(this);
    m_menu                   = new QMenu(this);
    m_settingsDialog         = new SettingsDialog;
    m_contactsDialog         = new ContactsDialog;
    m_debugInfoDialog        = new DebugInfoDialog;
    m_callHistoryDialog      = new CallHistoryDialog;
    m_placeCallDialog        = new PlaceCallDialog;
    m_remindersDialog        = new RemindersDialog;
    m_internalContactsDialog = new InternalContactsDialog;

    connect(m_systemTrayIcon,   &QSystemTrayIcon::activated,            this, &OutCall::onActivated);

    connect(g_pAsteriskManager, &AsteriskManager::messageReceived,      this, &OutCall::onMessageReceived);
    connect(g_pAsteriskManager, &AsteriskManager::callReceived,         this, &OutCall::onCallReceived);
    connect(g_pAsteriskManager, &AsteriskManager::error,                this, &OutCall::displayError);
    connect(g_pAsteriskManager, &AsteriskManager::stateChanged,         this, &OutCall::onStateChanged);
    connect(&m_timer,           &QTimer::timeout,                       this, &OutCall::changeIcon);

    connect(m_remindersDialog, &RemindersDialog::reminders, this, &OutCall::changeIconReminders);
    connect(m_settingsDialog, &SettingsDialog::restart, this, &OutCall::hideTrayIcon);

    connect(this, &OutCall::showReminders, m_remindersDialog, &RemindersDialog::showReminders);

    my_number = global::getSettingsValue(global::getExtensionNumber("extensions"), "extensions_name").toString();

    show_call_popup = global::getSettingsValue("show_call_popup", "general", true).toBool();

    createContextMenu();

    m_systemTrayIcon->setContextMenu(m_menu);

    QString path(":/images/disconnected.png");
    m_systemTrayIcon->setIcon(QIcon(path));

    g_pAsteriskManager->setAutoSignIn(global::getSettingsValue("auto_sign_in", "general", true).toBool());

    automaticlySignIn();
}

OutCall::~OutCall()
{
    delete m_settingsDialog;
    delete m_debugInfoDialog;
    delete m_contactsDialog;
    delete m_callHistoryDialog;
    delete m_placeCallDialog;
    delete m_remindersDialog;
    delete m_internalContactsDialog;
}

/**
 * Выполняет создание элементов контекстного меню.
 */
void OutCall::createContextMenu()
{
    // Exit action
    QAction* exitAction = new QAction(tr("Выход"), m_menu);
    connect(exitAction, &QAction::triggered, this, &OutCall::close);

    // Sign In
    m_signIn  = new QAction(tr("Войти в аккаунт"), m_menu);
    connect(m_signIn, &QAction::triggered, this, &OutCall::signInOut);

    // Settings
    QAction* settingsAction = new QAction(tr("Настройки"), m_menu);
    connect(settingsAction, &QAction::triggered, this, &OutCall::onSettingsDialog);

    QAction* debugInfoAction = new QAction(tr("Отладка"), m_menu);
    connect(debugInfoAction, &QAction::triggered, this, &OutCall::onDebugInfo);

    // Contacts
    contactsAction = new QAction(tr("Контакты"), m_menu);
    connect(contactsAction, &QAction::triggered, this, &OutCall::onContactsDialog);

    // Internal Contacts
    internalContactsAction = new QAction(tr("Внутренние"), m_menu);
    connect(internalContactsAction, &QAction::triggered, this, &OutCall::onInternalContactsDialog);

    // Call History
    callHistoryAction = new QAction(tr("История звонков"), m_menu);
    connect(callHistoryAction, &QAction::triggered, this, &OutCall::onCallHistory);

    // Reminders
    remindersAction = new QAction(tr("Напоминания"), m_menu);
    connect(remindersAction, &QAction::triggered, this, &OutCall::onRemindersDialog);

    // Place a Call
    m_placeCall = new QAction(tr("Позвонить"), 0);
    QFont font = m_placeCall->font();
    font.setBold(true);
    m_placeCall->setFont(font);
    connect(m_placeCall, &QAction::triggered, this, &OutCall::onPlaceCall);

    // Add actions
    m_menu->addSeparator();

    m_menu->addAction(m_placeCall);
    m_menu->addAction(callHistoryAction);
    m_menu->addAction(contactsAction);
    m_menu->addAction(internalContactsAction);
    m_menu->addSeparator();

    m_menu->addAction(remindersAction);
    m_menu->addSeparator();

    m_menu->addAction(settingsAction);
    //m_menu->addAction(debugInfoAction);
    m_menu->addSeparator();

    m_menu->addAction(m_signIn);
    m_menu->addSeparator();

    m_menu->addAction(exitAction);

    m_menu->setStyleSheet("QMenu                         {background-color: #F2F2F2;}"
                          "QMenu::item                   {padding-left: 30px; padding-right: 30px; padding-top: 5px; padding-bottom: 5px; margin-top: 2px; margin-bottom:2px; background-color: #F2F2F2;}"
                          "QMenu::separator              {height: 1px; margin-left: 30px;}"
                          "QMenu::item:selected          {background-color: #90C8F6; color: black;}"
                          "QMenu::item:disabled          {background-color: #F2F2F2; color: #A9A9A9;}"
                          "QMenu::item:disabled:selected {background-color: #F2F2F2; color: #A9A9A9;}");
}

/**
 * Выполняет автоматический вход в аккаунт.
 */
void OutCall::automaticlySignIn()
{
    bool autoSingIn = global::getSettingsValue("auto_sign_in", "general", true).toBool();

    if (autoSingIn)
        signInOut();
}

/**
 * Выполняет вход / выход из аккаунта.
 */
void OutCall::signInOut()
{
    if (m_signIn->text() == tr("Выйти из аккаунта") || m_signIn->text() == tr("Отменить вход"))
    {
        g_pAsteriskManager->signOut();

        return;
    }

    if (g_pAsteriskManager->isSignedIn() == false)
    {
        QString server = global::getSettingsValue("servername", "settings").toString();
        QString port = global::getSettingsValue("port", "settings").toString();

        g_pAsteriskManager->signIn(server, port.toUInt());
    }
    else
        g_pAsteriskManager->signOut();
}

/**
 * Выполняет отображение ошибок, связанных с сервером Asterisk.
 */
void OutCall::displayError(const QAbstractSocket::SocketError& socketError, const QString& msg)
{
    switch (socketError)
    {
        case QAbstractSocket::RemoteHostClosedError:
            MsgBoxInformation(tr("Удаленный хост закрыл соединение"));
            break;
        case QAbstractSocket::HostNotFoundError:
            MsgBoxInformation(tr("Хост не был найден. Пожалуйста, проверьте имя хоста "
                             "и настройки порта"));
            break;
        case QAbstractSocket::ConnectionRefusedError:
            MsgBoxInformation(tr("Соединение было отклонено узлом. "
                             "Убедитесь, что сервер работает, "
                             "и проверьте правильность имени хоста "
                             "и настроек порта"));
            break;
        default:
            MsgBoxInformation(msg);
    }
}

/**
 * Получает сообщение из класса AsteriskManager
 * для его последующей передачи классу DebugInfoDialog.
 */
void OutCall::onMessageReceived(const QString& message)
{
    if (m_debugInfoDialog)
        m_debugInfoDialog->updateDebug(message);
}

/**
 * Получает данные входящего звонка из класса AsteriskManager
 * для их последующей передачи классу PopupWindow.
 */
void OutCall::onCallReceived(const QMap<QString, QVariant>& call)
{
    QString dateTime        = call.value("dateTime").toString();
    QString from            = call.value("from").toString();
    QString callerIdName    = call.value("callerIdName").toString();
    QString uniqueid        = call.value("uniqueid").toString();
    QString my_number       = call.value("to").toString();

    if (from == callerIdName)
        callerIdName = tr("Неизвестный");

    if (show_call_popup)
        PopupWindow::showCall(dateTime, uniqueid, from, QString("<b style='color:white'>%1</b><br><b>%2</b>").arg(from).arg(callerIdName), my_number);
}

/**
 * Выполняет операции при изменении состояния подключения к Asterisk.
 */
void OutCall::onStateChanged(const AsteriskManager::AsteriskState& state)
{
    if (state == AsteriskManager::CONNECTED)
    {
        QSqlDatabase::database().open();
        QSqlDatabase::database("Calls").open();

        if (!QSqlDatabase::database().isOpen() || !QSqlDatabase::database("Calls").isOpen())
            dbsOpened = false;

        m_signIn->setText(tr("Выйти из аккаунта"));

        PopupHelloWindow::showInformationMessage(qApp->applicationName(), tr("Вы успешно вошли"));
        m_systemTrayIcon->setToolTip(tr("Вы успешно вошли"));

        m_timer.stop();

        if (!dbsOpened)
        {
            m_systemTrayIcon->hide();

            qApp->closeAllWindows();
            qApp->quit();

            QProcess::startDetached(qApp->arguments()[0], QStringList() << "restart");
        }
        else
        {
            changeIconReminders(false);

            enableActions();

            emit showReminders(true);
        }
    }
    else if (state == AsteriskManager::CONNECTING)
    {
        m_signIn->setText(tr("Отменить вход"));
        m_systemTrayIcon->setToolTip(tr("") + tr("") + tr("Вход в аккаунт"));

        disableActions();

        m_timer.start(500);
    }
    else if (state == AsteriskManager::DISCONNECTED)
    {
        emit showReminders(false);

        PopupHelloWindow::closeAll();
        PopupWindow::closeAll();
        PopupNotification::closeAll();
        PopupReminder::closeAll();

        QApplication::closeAllWindows();

        QString path(":/images/disconnected.png");
        m_systemTrayIcon->setIcon(QIcon(path));

        m_signIn->setText(tr("&Войти в аккаунт"));
        m_systemTrayIcon->setToolTip(tr("") + tr("") + tr("Вы не вошли"));

        disableActions();

        m_timer.stop();
    }
    else if (state == AsteriskManager::AUTHENTICATION_FAILED)
    {
        emit showReminders(false);

        PopupHelloWindow::closeAll();
        PopupWindow::closeAll();
        PopupNotification::closeAll();
        PopupReminder::closeAll();

        QApplication::closeAllWindows();

        QString path(":/images/started.png");
        m_systemTrayIcon->setIcon(QIcon(path));

        PopupHelloWindow::showInformationMessage(tr(""), tr("Ошибка аутентификации"));

        m_systemTrayIcon->setToolTip(tr("") + tr("") + tr("Не настроен"));
        m_signIn->setText(tr("&Войти в аккаунт"));

        disableActions();

        m_timer.stop();
    }
}

/**
 * Выполняет отключение элементов контекстного меню.
 */
void OutCall::disableActions()
{
    m_placeCall->setEnabled(false);
    callHistoryAction->setEnabled(false);
    contactsAction->setEnabled(false);
    internalContactsAction->setEnabled(false);
    remindersAction->setEnabled(false);
}

/**
 * Выполняет включение элементов контекстного меню.
 */
void OutCall::enableActions()
{
    m_placeCall->setEnabled(true);
    callHistoryAction->setEnabled(true);
    contactsAction->setEnabled(true);
    internalContactsAction->setEnabled(true);
    remindersAction->setEnabled(true);
}

/**
 * Выполняет скрытие иконки приложения в области уведомлений.
 */
void OutCall::hideTrayIcon(bool hide)
{
    if (hide)
        m_systemTrayIcon->hide();
}

/**
 * Выполняет изменение иконки приложения при изменении
 * количества актуальных напоминаний.
 */
void OutCall::changeIconReminders(bool change)
{
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_from <> ? AND phone_to = ? AND active = true AND viewed = false");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    qint32 receivedReminders = 0;

    if (query.next())
        receivedReminders = query.value(0).toInt();

    query.prepare("SELECT COUNT(*) FROM reminders WHERE phone_to = ? AND active = true");
    query.addBindValue(my_number);
    query.exec();

    qint32 activeReminders = 0;

    if (query.next())
        activeReminders = query.value(0).toInt();

    if (change)
        setIconReminders(Qt::red, activeReminders);
    else if (!change && activeReminders > 0 && receivedReminders > 0)
        setIconReminders(Qt::red, activeReminders);
    else if (!change && activeReminders > 0 && receivedReminders == 0)
        setIconReminders(Qt::lightGray, activeReminders);
    else if (!change && activeReminders == 0)
    {
        QString path(":/images/connected.png");
        m_systemTrayIcon->setIcon(QIcon(path));
    }
}

/**
 * Выполняет установку иконки в области уведомлений
 * с количеством актуальных напоминаний.
 */
void OutCall::setIconReminders(QColor color, qint32 activeReminders)
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.drawPixmap(0, 0, QPixmap(":/images/connectedTray.png"));
    painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap));
    painter.setBrush(QBrush(color));
    painter.drawEllipse(4, 4, 18, 18);
    QFont font("MS Shell Dlg 2", 8);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(4, 7, 18, 13), Qt::AlignCenter, QString::number(activeReminders));
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.end();

    m_systemTrayIcon->setIcon(QIcon(pixmap));
}

/**
 * Выполняет изменение иконки приложения при подключенном / неподключенном Asterisk.
 */
void OutCall::changeIcon()
{
    if (m_switch)
    {
        QString path(":/images/connected.png");
        m_systemTrayIcon->setIcon(QIcon(path));

        m_switch = false;
    }
    else
    {
        QString path(":/images/disconnected.png");
        m_systemTrayIcon->setIcon(QIcon(path));

        m_switch = true;
    }
}

/**
 * Выполняет открытие окна с историей звонков.
 */
void OutCall::onCallHistory()
{
    m_callHistoryDialog->showNormal();
    m_callHistoryDialog->raise();
}

/**
 * Выполняет открытие окна с настройками приложения.
 */
void OutCall::onSettingsDialog()
{
//    SettingsDialog dialog;
//    dialog.exec();
    m_settingsDialog->showNormal();
    m_settingsDialog->raise();
}

/**
 * Выполняет открытие окна с информацией об отладке приложения.
 */
void OutCall::onDebugInfo()
{
    m_debugInfoDialog->showNormal();
    m_debugInfoDialog->raise();
}

/**
 * Выполняет открытие окна для осуществления звонков.
 */
void OutCall::onPlaceCall()
{
    m_placeCallDialog->showNormal();
    m_placeCallDialog->raise();
}

/**
 * Выполняет открытие окна с контактной книгой.
 */
void OutCall::onContactsDialog()
{
    m_contactsDialog->showMaximized();
    m_contactsDialog->raise();
}

/**
 * Выполняет открытие окна с внутренними контактами.
 */
void OutCall::onInternalContactsDialog()
{
    m_internalContactsDialog->showNormal();
    m_internalContactsDialog->raise();
}

/**
 * Выполняет открытие окна со списком напоминаний.
 */
void OutCall::onRemindersDialog()
{
    m_remindersDialog->showNormal();
    m_remindersDialog->raise();

    QSqlQuery query(db);

    query.prepare("UPDATE reminders SET viewed = true WHERE phone_from <> ? AND phone_to = ?");
    query.addBindValue(my_number);
    query.addBindValue(my_number);
    query.exec();

    m_remindersDialog->reminders(false);
}

/**
 * Выполняет выход из приложения.
 */
void OutCall::close()
{
    g_pAsteriskManager->signOut();

    m_systemTrayIcon->hide();

    QApplication::quit();
}

/**
 * Выполняет открытие контекстного меню.
 */
void OutCall::onActivated(const QSystemTrayIcon::ActivationReason& reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        m_debugInfoDialog->activateWindow();
        m_settingsDialog->activateWindow();
        m_callHistoryDialog->activateWindow();
        m_contactsDialog->activateWindow();
        m_placeCallDialog->activateWindow();
        m_remindersDialog->activateWindow();
    }
    else if (reason == QSystemTrayIcon::DoubleClick)
    {
        if (dbsOpened && g_pAsteriskManager->m_currentState == AsteriskManager::CONNECTED)
        {
            m_placeCallDialog->showNormal();
            m_placeCallDialog->activateWindow();
        }
    }
}

/**
 * Выполняет отображение иконки приложения при запуске.
 */
void OutCall::show()
{
    m_systemTrayIcon->show();
}