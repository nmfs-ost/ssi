/* This is a work of the U.S. Government and is not subject to copyright
 * protection in the United States. Foreign copyrights may apply.
 * See copyright.txt for more information.
 */

#ifndef DIALOG_ABOUT_GUI_H
#define DIALOG_ABOUT_GUI_H

#include <QDialog>

#include "dialog_fileview.h"

namespace Ui {
class Dialog_about_gui;
}

class Dialog_about_gui : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_about_gui(QWidget *parent = 0);
    ~Dialog_about_gui();

public slots:
    void showReadme();
private:
    Ui::Dialog_about_gui *ui;
    Dialog_fileView *readmeview;

signals:
    void showGuide();
    void showAboutQt();
    void showCopyright();
};

#endif // DIALOG_ABOUT_GUI_H
