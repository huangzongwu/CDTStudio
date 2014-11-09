#include "cdtattributedockwidget.h"
#include "stable.h"
#include "cdtimagelayer.h"
#include "cdtsegmentationlayer.h"
#include "dialoggenerateattributes.h"
#include "cdtplot2ddockwidget.h"
#include "cdttableexporter.h"

#include "mainwindow.h"

CDTAttributeDockWidget::CDTAttributeDockWidget(QWidget *parent) :
    CDTDockWidget(parent),
    tabWidget(new QTabWidget(this)),
    segmentationLayer(NULL)
{
    setWindowTitle(tr("Attributes Manager"));

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    widget->setLayout(layout);    
    setWidget(widget);
    layout->addWidget(tabWidget);
    tabWidget->setMovable(true);
    tabWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *actionExportCurrentTable = new QAction(QIcon(":/Icon/Save.png"),tr("Export current table to file"), this);
    QAction *actionExportAllTables = new QAction(QIcon(":/Icon/Save.png"),tr("Export all tables to file"), this);
    connect(actionExportCurrentTable,SIGNAL(triggered()),SLOT(onActionExportCurrentTable()));
    connect(actionExportAllTables,SIGNAL(triggered()),SLOT(onActionExportAllTables()));
    tabWidget->addAction(actionExportCurrentTable);
    tabWidget->addAction(actionExportAllTables);

    logger()->info("Constructed");
}

CDTAttributeDockWidget::~CDTAttributeDockWidget()
{
}

CDTSegmentationLayer *CDTAttributeDockWidget::segmLayer() const
{
    return segmentationLayer;
}

void CDTAttributeDockWidget::setCurrentLayer(CDTBaseLayer *layer)
{
    if (segmentationLayer == layer)
        return;

    clear();
    segmentationLayer = qobject_cast<CDTSegmentationLayer *>(layer->getAncestor("CDTSegmentationLayer"));
    if (segmentationLayer)
    {
        logger()->info("Find ancestor class of CDTSegmentationLayer");
        setTableModels(segmentationLayer->tableModels());
        setEnabled(true);
    }
    else
    {
        logger()->info("No ancestor class of CDTSegmentationLayer found");
        return;
    }
}

void CDTAttributeDockWidget::onDockClear()
{
    clear();
}

void CDTAttributeDockWidget::clear()
{
    this->setEnabled(false);
    dbConnInfo = CDTDatabaseConnInfo();
    clearTables();
    QSqlDatabase::removeDatabase("attribute");
    segmentationLayer =NULL;
}

void CDTAttributeDockWidget::setTableModels(QList<QAbstractTableModel *> models)
{
    clearTables();
    foreach (QAbstractTableModel *model, models) {
        QTableView* widget = new QTableView(tabWidget);
        model->setParent(widget);
        widget->setAlternatingRowColors(true);
        widget->setModel(model);
        widget->setSelectionMode(QTableView::SingleSelection);
        widget->setEditTriggers(QTableView::NoEditTriggers);
        widget->setItemDelegateForColumn(0,new CDTObjectIDDelegate(this));
        widget->verticalHeader()->hide();
        widget->resizeColumnsToContents();
        widget->resizeRowsToContents();

        tabWidget->addTab(widget,model->property("name").toString());
        connect(widget,SIGNAL(clicked(QModelIndex)),SLOT(onItemClicked(QModelIndex)));
    }
}

void CDTAttributeDockWidget::onItemClicked(QModelIndex index)
{    
    QTableView *view = (QTableView *)(tabWidget->currentWidget());
    if (view==NULL)return;

    QString tableName = tabWidget->tabText(tabWidget->currentIndex());
    QString featureName = view->model()->headerData(index.column(),Qt::Horizontal).toString();

    if (featureName.toLower() == "objectid")
        return;

    MainWindow::getPlot2DDockWidget()->setDataSource(QSqlDatabase::database("attribute"),tableName,featureName);
}

void CDTAttributeDockWidget::onActionExportCurrentTable()
{
    if (tabWidget->currentIndex()<0)
        return;

    QString path = QFileDialog::getSaveFileName(this,tr("Export current table to"),QString(),tr("CSV Files(*.csv);;Text Files(*.txt)"));
    if (path.isEmpty())
        return;

    QSqlDatabase db = QSqlDatabase::database("attribute");
    QString tableName = tabWidget->tabText(tabWidget->currentIndex());

    QString errorText;
    if (CDTTableExporter::exportSingleTable(db,tableName,path,true,errorText)==false)
    {
        QMessageBox::critical(this,tr("Error"),tr("Fialed! Error:%1").arg(errorText));
        return;
    }
}

void CDTAttributeDockWidget::onActionExportAllTables()
{
    if (tabWidget->count()<0)
        return;

    QString path = QFileDialog::getSaveFileName(this,tr("Export all tables to"),QString(),tr(""));
}

void CDTAttributeDockWidget::clearTables()
{
    while (tabWidget->count()>0)
    {
        QWidget *widget = tabWidget->widget(0);
        tabWidget->removeTab(0);
        delete widget;
    }
}
