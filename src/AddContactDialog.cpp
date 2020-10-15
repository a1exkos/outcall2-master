/*
 * Класс служит для добавления физ. лица.
 */

#include "AddContactDialog.h"
#include "ui_AddContactDialog.h"

#include <QMessageBox>
#include <QDebug>

AddContactDialog::AddContactDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AddContactDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    connect(ui->comment, &QTextEdit::textChanged, this, &AddContactDialog::onTextChanged);
    connect(ui->saveButton, &QAbstractButton::clicked, this, &AddContactDialog::onSave);

//    for(qint32 i = 0; i < ui->phonesLayout->count(); ++i)
//    {
//        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(ui->phonesLayout->itemAt(i)->widget());
//           if (lineEdit)
//               phonesList.append(lineEdit);
//    }

    phonesList = { ui->firstNumber, ui->secondNumber, ui->thirdNumber, ui->fourthNumber, ui->fifthNumber };

    QRegularExpression regExp("^[\\+]?[0-9]*$");
    phonesValidator = new QRegularExpressionValidator(regExp, this);

    for (qint32 i = 0; i < phonesList.length(); ++i)
        phonesList.at(i)->setValidator(phonesValidator);

    regExp.setPattern("^[0-9]*$");
    vyborIdValidator = new QRegularExpressionValidator(regExp, this);

    ui->vyborId->setValidator(vyborIdValidator);
}

AddContactDialog::~AddContactDialog()
{
    delete phonesValidator;
    delete vyborIdValidator;
    delete ui;
}

/**
 * Выполняет проверку введенных данных и их последующее сохранение в БД.
 */
void AddContactDialog::onSave()
{
    QSqlQuery query(db);

    QString lastName = ui->lastName->text();
    QString firstName = ui->firstName->text();
    QString patronymic = ui->patronymic->text();

    QStringList phonesListRegExp;

    for (qint32 i = 0; i < phonesList.length(); ++i)
    {
        phonesList.at(i)->setStyleSheet("border: 1px solid grey");

        phonesListRegExp.append(phonesList.at(i)->text().remove(QRegularExpression("^[\\+]?[3]?[8]?")));
    }

    bool empty_field = false;

    if (ui->firstName->text().isEmpty())
    {
         ui->label_15->setText("<span style=\"color: red;\">" + tr("Заполните обязательное поле!") + "</span>");

         ui->firstName->setStyleSheet("border: 1px solid red");

         empty_field = true;
    }
    else
    {
        ui->label_15->setText("");

        ui->firstName->setStyleSheet("border: 1px solid grey");
    }

    if (ui->firstNumber->text().isEmpty())
    {
        ui->label_14->show();
        ui->label_14->setText("<span style=\"color: red;\">" + tr("Заполните обязательное поле!") + "</span>");

        ui->firstNumber->setStyleSheet("border: 1px solid red");

        empty_field = true;
    }
    else
    {
        ui->label_14->setText("");

        ui->firstNumber->setStyleSheet("border: 1px solid grey");
    }

    if (empty_field)
        return;

    bool invalid_phones = false;

    for (qint32 i = 0; i < phonesList.length(); ++i)
    {
        if (!phonesList.at(i)->text().isEmpty())
        {
            QString phone = phonesList.at(i)->text();

            if (isPhone(&phone))
                phonesList.at(i)->setStyleSheet("border: 1px solid grey");
            else
            {
                phonesList.at(i)->setStyleSheet("border: 1px solid red");

                invalid_phones = true;
            }
        }
    }

    if (invalid_phones)
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Номер не соответствует формату!"), QMessageBox::Ok);

        return;
    }

    bool same_phones = false;

    for (qint32 i = 0; i < phonesList.length(); ++i)
        for (qint32 j = 0; j < phonesList.length(); ++j)
            if (!phonesList.at(i)->text().isEmpty() && phonesListRegExp.at(i) == phonesListRegExp.at(j) && i != j)
            {
                phonesList.at(i)->setStyleSheet("border: 1px solid red");
                phonesList.at(j)->setStyleSheet("border: 1px solid red");

                same_phones = true;
            }

    if (same_phones)
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Присутсвуют одинаковые номера!"), QMessageBox::Ok);

        return;
    }

    bool existing_phones = false;

    for (qint32 i = 0; i < phonesList.length(); ++i)
        if (!phonesList.at(i)->text().isEmpty())
        {
            query.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + phonesListRegExp.at(i) + "')");
            query.exec();
            query.next();

            if (query.value(0) != 0)
            {
                phonesList.at(i)->setStyleSheet("border: 1px solid red");

                existing_phones = true;
            }
        }

    if (existing_phones)
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Введены существующие номера!"), QMessageBox::Ok);

        return;
    }

    query.prepare("INSERT INTO entry (entry_type, entry_name, entry_person_org_id, entry_person_lname, entry_person_fname, entry_person_mname, entry_city, entry_address, entry_email, entry_vybor_id, entry_comment)"
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue("person");

    if (ui->lastName->text().isEmpty())
        query.addBindValue(firstName + ' ' + patronymic);
    else
        query.addBindValue(lastName + ' ' + firstName + ' ' + patronymic);

    if (!orgId.isNull())
        query.addBindValue(orgId);
    else
        query.addBindValue(QVariant(QVariant::Int));

    query.addBindValue(lastName);
    query.addBindValue(firstName);
    query.addBindValue(patronymic);
    query.addBindValue(ui->city->text());
    query.addBindValue(ui->address->text());
    query.addBindValue(ui->email->text());
    query.addBindValue(ui->vyborId->text());
    query.addBindValue(ui->comment->toPlainText().trimmed());
    query.exec();

    qint32 id = query.lastInsertId().toInt();

    for (qint32 i = 0; i < phonesList.length(); ++i)
        if (!phonesList.at(i)->text().isEmpty())
        {
            query.prepare("INSERT INTO fones (entry_id, fone)"
                           "VALUES(?, ?)");
            query.addBindValue(id);
            query.addBindValue(phonesListRegExp.at(i));
            query.exec();
        }

    if (!addOrgToPerson.isNull())
        addOrgToPerson.data()->close();

    emit sendData(true);

    close();

    QMessageBox::information(this, tr("Уведомление"), tr("Запись успешно добавлена!"), QMessageBox::Ok);
}

/**
 * Выполняет проверку на соответсвие номера шаблону.
 */
bool AddContactDialog::isPhone(QString* str)
{
    qint32 pos = 0;

    QRegularExpressionValidator validator(QRegularExpression("(^[\\+][3][8][0][0-9]{9}$|^[3][8][0][0-9]{9}$|^[0][0-9]{9}$)"));

    if (validator.validate(*str, pos) == QValidator::Acceptable)
        return true;

    return false;
}

/**
 * Получает id и название выбранной организации из класса AddOrgToPerson
 * для последующей привязки к физ. лицу.
 */
void AddContactDialog::receiveOrgName(const QString& id, const QString& name)
{
    if (!id.isNull())
    {
        ui->label_org->setText(name);

        orgId = id;
    }
    else
    {
        ui->label_org->setText(tr("Нет"));

        orgId = "0";
    }
}

/**
 * Выполняет открытие окна со списком организаций для выбора определенной на привязку.
 */
void AddContactDialog::on_addOrgButton_clicked()
{
    if (!addOrgToPerson.isNull())
        addOrgToPerson.data()->close();

    addOrgToPerson = new AddOrgToPerson;
    connect(addOrgToPerson.data(), &AddOrgToPerson::sendOrgName, this, &AddContactDialog::receiveOrgName);
    addOrgToPerson.data()->show();
    addOrgToPerson.data()->setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Выполняет отвязку организации.
 */
void AddContactDialog::on_deleteOrgButton_clicked()
{
    ui->label_org->setText(tr("Нет"));

    orgId = "0";
}

/**
 * Получает номер телефона неизвестного физ. лица из классов
 * CallHistoryDialog и PopupWindow.
 */
void AddContactDialog::setValues(const QString& number)
{
    ui->firstNumber->setText(number);
}

/**
 * Выполняет удаление последнего символа в тексте,
 * если его длина превышает 255 символов.
 */
void AddContactDialog::onTextChanged()
{
    if (ui->comment->toPlainText().trimmed().length() > 255)
        ui->comment->textCursor().deletePreviousChar();
}

/**
 * Выполняет обработку нажатий клавиш.
 * Особая обработка для клавиш Esc и Enter.
 */
void AddContactDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        QDialog::close();
    else if (event->key() == Qt::Key_Return)
    {
        if (ui->comment->hasFocus())
            return;
        else
            onSave();
    }
    else
        QDialog::keyPressEvent(event);
}

/**
 * Выполняет обработку закрытия окна.
 */
void AddContactDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);

    if (!addOrgToPerson.isNull())
        addOrgToPerson.data()->close();
}