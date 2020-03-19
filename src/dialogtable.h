#ifndef DIALOGTABLE_H
#define DIALOGTABLE_H

#include <QDialog>
#include <QString>

#include "tableview.h"
#include "tablemodel.h"

namespace Ui {
class DialogTable;
}

class DialogTable : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTable(QWidget *parent = nullptr);
    ~DialogTable();

    void setTitle (QString title);

    tablemodel *getTable() const;
    void setTable(tablemodel *value);

    void closeTable();

public slots:
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    void setVisible(bool flag);

private:
    Ui::DialogTable *ui;

    tablemodel *table;
    tableview *view;

    QRect currRect;
};

#endif // DIALOGTABLE_H
