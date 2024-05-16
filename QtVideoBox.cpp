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

	// �����ļ���ģ��
	QStandardItemModel* inputModel = new QStandardItemModel(this);
	ui.tableView->setModel(inputModel);

	// ����ļ���ģ��
	QStandardItemModel* outputModel = new QStandardItemModel(this);
	ui.listView_out->setModel(outputModel);

	QObject::connect(ui.pushButton_in, &QPushButton::clicked, [=]() {
		QString filePath = QFileDialog::getOpenFileName(this, "ѡ�������ļ�", QDir::homePath(), "��Ƶ�ļ� (*.mp4 *.avi *.mov);; �����ļ� (*.*)");
		qDebug() << QString("�����ļ�·��:") << filePath;
		if (!filePath.isEmpty()) {
			inputModel->clear();
			int rowCount = inputModel->rowCount();
			inputModel->setItem(rowCount, 0, new QStandardItem(filePath));
		}
		});

	QObject::connect(ui.pushButton_out, &QPushButton::clicked, [=]() {
		QString filePath = QFileDialog::getSaveFileName(this, "ѡ������ļ�", QDir::homePath(), "��Ƶ�ļ� (*.mp4 *.avi *.mov);; �����ļ� (*.*)");
		qDebug() << QString("����ļ�·��:") << filePath;
		if (!filePath.isEmpty()) {
			outputModel->clear();
			int rowCount = outputModel->rowCount();
			outputModel->setItem(rowCount, 0, new QStandardItem(filePath));
		}
		});


	QObject::connect(ui.pushButton_run, &QPushButton::clicked, [=]() {
		// ��ȡ���������ļ�·��
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
				// ����һ����Ϣ��������ʾ��Ϣ
				QMessageBox msgBox;
				msgBox.setText("success!");
				msgBox.exec();
			}
			else {
				// ����һ����Ϣ��������ʾ��Ϣ
				QMessageBox msgBox;
				msgBox.setText("fail!");
				msgBox.exec();
			}
		}
		catch (const std::exception& e) {
			// ����һ����Ϣ��������ʾ��Ϣ
			QMessageBox msgBox;
			msgBox.setText(e.what());
			msgBox.exec();
		}
		});

}

QtVideoBox::~QtVideoBox()
{}
