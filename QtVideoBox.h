#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtVideoBox.h"

class QtVideoBox : public QMainWindow
{
    Q_OBJECT

public:
    QtVideoBox(QWidget *parent = nullptr);
    ~QtVideoBox();

private:
    Ui::QtVideoBoxClass ui;
};
