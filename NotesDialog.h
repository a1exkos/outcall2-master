#ifndef NOTESDIALOG_H
#define NOTESDIALOG_H

#include "SettingsDialog.h"

#include <QDialog>
#include <QSqlQueryModel>
#include <QTableView>

namespace Ui {
class NotesDialog;
}

class NotesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotesDialog(QWidget *parent = 0);
    void setCallId(QString uniqueid, QString state_call);
    ~NotesDialog();

protected:
    void onUpdate();
    void setNote();
    void loadNotes();
    void deleteObjects();

protected slots:
    bool eventFilter(QObject *object, QEvent *event);
    void onTextChanged();

private:
    Ui::NotesDialog *ui;
    QString callId;
    QString state;
    QString my_number;
    QSqlQueryModel *query;

private slots:
    void onSave();

signals:
    void sendDataToAllCalls();
    void sendDataToMissed();
    void sendDataToReceived();
    void sendDataToPlaced();
};

#endif // NOTESDIALOG_H
