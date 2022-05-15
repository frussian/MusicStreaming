#include <QDebug>
#include <QDockWidget>
#include <QTextEdit>
#include <QToolBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainwidget.h"

MainWindow::MainWindow(QThread *th, QWidget *parent)
    : QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	auto fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction("p 1");
	fileMenu->addAction("p 2");
	fileMenu->addAction("p 3");
	QToolBar* toolBar = addToolBar(tr("toolbar"));
	toolBar->addAction("test");

	mainWidget = new MainWidget(th);
	setCentralWidget(mainWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

