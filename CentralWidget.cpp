#include "CentralWidget.h"
#include <QSqlTableModel>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlField>
#include <QtCore/QFile>
#include "Document.h"
#include "DataType.h"
#include "MainWindow3D.h"

CentralWidget::CentralWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.tableView->verticalHeader()->hide();
	ui.tableView->setAlternatingRowColors(true);
// 	ui.tableView->setStyleSheet("QTableView{background-color: rgb(250, 250, 115);"
// 		"alternate-background-color: rgb(141, 163, 215);}");

	connect(ui.pushButton_Submit, SIGNAL(clicked()), this, SLOT(slotSubmit()));
	connect(ui.pushButton_Revert, SIGNAL(clicked()), this, SLOT(slotRevert()));
	connect(ui.pushButton_Delete, SIGNAL(clicked()), this, SLOT(slotDelete()));

	connect(ui.pushButton_3DView, SIGNAL(clicked()), this, SLOT(slot3DView()));
}

CentralWidget::~CentralWidget()
{

}

void CentralWidget::slotSubmit()
{
	QSqlTableModel* model = (QSqlTableModel*)ui.tableView->model();
	if (!model)
		return;

	//model->database().open();

	model->database().transaction();//开始事物操作
	if (model->submitAll())
		model->database().commit();
	else{
		model->database().rollback(); //回滚
		QMessageBox::warning(this, tr("tableModel"),
			QStringLiteral("数据库错误:") + tr("%1").arg(model->lastError().text()));
	}

	//model->database().close();
}

void CentralWidget::slotRevert()
{
	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();

	if (pTableModel)
		pTableModel->revertAll();
}

void CentralWidget::slotBeforeDelete(int nRow)
{
	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();
	if (!pTableModel)
		return;

	if (Document::needCopyFile())
	{
		QSqlRecord record = pTableModel->record(nRow);
		QJsonArray fields = m_currentObject.value(Class_Fields).toArray();
		for (int i = 0; i < fields.count(); i++)
		{
			QJsonObject field = fields.at(i).toObject();
			QString dataType = field.value(Field_DataType).toString();
			QString fieldName = field.value(Field_Name).toString();

			bool bTag = record.isGenerated(fieldName);

			if (dataType == FieldType_File_Des)
			{
				QString strPath = record.field(fieldName).value().toString();
				QString strFilePath = m_strCurrentDir + "/" + strPath;
				QString strDataDir = Document::dataDir();
				QString strTypeDir = Document::typeDir();
				strFilePath.remove(0, strTypeDir.size());
				strFilePath = strDataDir + strFilePath;
				QFile::remove(strFilePath);
			}
		}
	}
}

void CentralWidget::slot3DView()
{
	static MainWindow3D* pMainwindow3D = new MainWindow3D;
	pMainwindow3D->showMaximized();
}

void CentralWidget::slotDelete()
{
	//获取选中行
	int curRow = ui.tableView->currentIndex().row();
	if (curRow < 0)
		return;

	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();
	if (!pTableModel)
		return;

	//删除该行
	pTableModel->removeRow(curRow); //只是model中删除，还未提交到数据库

	int ok = QMessageBox::warning(this, QStringLiteral("删除当前行"),
		QStringLiteral("确定与否？"),
		QMessageBox::Yes, QMessageBox::No);
	if (ok == QMessageBox::No)
		pTableModel->revertAll();
	else
		pTableModel->submitAll();
}

void CentralWidget::showBlank()
{
	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();
	if (pTableModel == nullptr)
		return;

	pTableModel->clear();
	ui.tableView->repaint();
}

void CentralWidget::showDBTable(const QString& strCurrentDir, const QJsonObject& currentObj)
{
	m_strCurrentDir = strCurrentDir;
	QString strTableName = currentObj.value(Class_Guid).toString();
	m_currentObject = currentObj;

	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();
	if (pTableModel == nullptr)
		return;

	//pTableModel->SetJsonObj(currentObj);
	setDelegate(currentObj);
	pTableModel->setTable(strTableName);
	pTableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
	pTableModel->select(); //选取整个表的所有行
	// pTableModel->removeColumn(1); //不显示第二列,如果这时添加记录，则该属性的值添加不上

	ui.tableView->repaint();
	//View->setEditTriggers(QAbstractItemView::NoEditTriggers); 使其不可编辑
}

void CentralWidget::showSelectDBTable(const QString& strCurrentDir, const QJsonObject& currentObj, const QString& strSQL)
{
	m_strCurrentDir = strCurrentDir;
	QString strTableName = currentObj.value(Class_Guid).toString();
	m_currentObject = currentObj;

	QSqlTableModel* pTableModel = (QSqlTableModel*)ui.tableView->model();
	if (pTableModel == nullptr)
		return;

	//pTableModel->SetJsonObj(currentObj);
	setDelegate(currentObj);
	pTableModel->setTable(strTableName);
	pTableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
	pTableModel->setFilter(strSQL);
	pTableModel->select(); //选取整个表的所有行
	// pTableModel->removeColumn(1); //不显示第二列,如果这时添加记录，则该属性的值添加不上

	ui.tableView->repaint();
	//View->setEditTriggers(QAbstractItemView::NoEditTriggers); 使其不可编辑
}

void CentralWidget::setDelegate(const QJsonObject& currentObj)
{
	QJsonArray fields = currentObj.value(Class_Fields).toArray();
	for (int i = 0; i < fields.count(); i++)
	{
		QJsonObject field = fields.at(i).toObject();
		QString strFieldType = field.value(Field_DataType).toString();

		QAbstractItemDelegate* pDelegate = ui.tableView->itemDelegateForColumn(i + 1);
		if (pDelegate)
		{
			delete pDelegate;
		}

		if (strFieldType.compare(FieldType_Int_Des) == 0)
		{
			IntDelegate* pNewDelegate = new IntDelegate;
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
		else if (strFieldType.compare(FieldType_Double_Des) == 0)
		{
			DoubleDelegate* pNewDelegate = new DoubleDelegate;
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
		else if (strFieldType.compare(FieldType_Bool_Des) == 0)
		{
			BoolDelegate* pNewDelegate = new BoolDelegate;
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
		else if (strFieldType.compare(FieldType_String_Des) == 0)
		{
			
		}
		else if (strFieldType.compare(FieldType_File_Des) == 0)
		{
			FileDelegate* pNewDelegate = new FileDelegate;
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
		else if (strFieldType.compare(FieldType_DateTime_Des) == 0)
		{
			DateDelegate* pNewDelegate = new DateDelegate;
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
		else if (strFieldType.compare(FieldType_OBMode_Des) == 0
			|| strFieldType.compare(FieldType_OBTarget_Des) == 0
			|| strFieldType.compare(FieldType_Polarity_Des) == 0)
		{
			EnumDelegate* pNewDelegate = new EnumDelegate(strFieldType);
			ui.tableView->setItemDelegateForColumn(i + 1, pNewDelegate);
		}
	}
}

void CentralWidget::changeDatabase()
{
	QAbstractItemModel* pModel = ui.tableView->model();
	if (pModel)
	{
		disconnect(pModel, SIGNAL(beforeDelete(int)), this, SLOT(slotBeforeDelete(int)));
		delete pModel;
	}

	//model与database是相关联的，需要重新创建
	QSqlTableModel* model = new QSqlTableModel(this);
	ui.tableView->setModel(model);

	connect(model, SIGNAL(beforeDelete(int)), this, SLOT(slotBeforeDelete(int)));
}