#include "QtVideoBox.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QVBoxLayout>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

QtVideoBox::QtVideoBox(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    QStandardItemModel* model = new QStandardItemModel(this);
    ui.tableView->setModel(model);

    QObject::connect(ui.pushButton, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(this, "choose file", QDir::homePath(), "video file (*.mp4 *.avi *.mov);; all file (*.*) ");
        qDebug() << QString("file path:") << filePath;
        if (!filePath.isEmpty()) {
            int rowCount = model->rowCount();
            model->setItem(rowCount, 0, new QStandardItem(filePath));
        }
        });
}


QtVideoBox::~QtVideoBox()
{}
