#include "QtVideoBox.h"
#include"FFmpegUtils.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QVBoxLayout>

extern "C" {
#include <libavcodec/avcodec.h>
}

QtVideoBox::QtVideoBox(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// 输入文件的模型
	QStandardItemModel* inputModel = new QStandardItemModel(this);
	ui.tableView->setModel(inputModel);

	// 输出文件的模型
	QStandardItemModel* outputModel = new QStandardItemModel(this);
	ui.listView_out->setModel(outputModel);

	QObject::connect(ui.pushButton_in, &QPushButton::clicked, [=]() {
		QString filePath = QFileDialog::getOpenFileName(this, "选择输入文件", QDir::homePath(), "视频文件 (*.mp4 *.avi *.mov);; 所有文件 (*.*)");
		qDebug() << QString("输入文件路径:") << filePath;
		if (!filePath.isEmpty()) {
			inputModel->clear();
			int rowCount = inputModel->rowCount();
			inputModel->setItem(rowCount, 0, new QStandardItem(filePath));
		}
		});

	QObject::connect(ui.pushButton_out, &QPushButton::clicked, [=]() {
		QString filePath = QFileDialog::getSaveFileName(this, "选择输出文件", QDir::homePath(), "视频文件 (*.mp4 *.avi *.mov);; 所有文件 (*.*)");
		qDebug() << QString("输出文件路径:") << filePath;
		if (!filePath.isEmpty()) {
			outputModel->clear();
			int rowCount = outputModel->rowCount();
			outputModel->setItem(rowCount, 0, new QStandardItem(filePath));
		}
		});


	QObject::connect(ui.pushButton_run, &QPushButton::clicked, [=]() {
		// 获取输入和输出文件路径
		QString inputFilePath;
		QString outputFilePath;
		if (inputModel->rowCount() > 0)
			inputFilePath = inputModel->item(0, 0)->text();
		if (outputModel->rowCount() > 0)
			outputFilePath = outputModel->item(0, 0)->text();
		try {

			int start = ui.doubleSpinBox->value() * 1000;
			int end = ui.doubleSpinBox_2->value() * 1000;
			if (clipVideo(inputFilePath.toUtf8().constData(), outputFilePath.toUtf8().constData(), start,end)) {
				// 创建一个消息框并设置提示信息
				QMessageBox msgBox;
				msgBox.setText("success!");
				msgBox.exec();
			}
			else {
				// 创建一个消息框并设置提示信息
				QMessageBox msgBox;
				msgBox.setText("fail!");
				msgBox.exec();
			}
		}
		catch (const std::exception& e) {
			// 创建一个消息框并设置提示信息
			QMessageBox msgBox;
			msgBox.setText(e.what());
			msgBox.exec();
		}
		});

}

QtVideoBox::~QtVideoBox()
{}
