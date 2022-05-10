#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QSlider>
#include <QFile>
#include <QDebug>
#include <QLabel>
#include <QStyle>
#include <QStackedWidget>
#include <QHeaderView>

#include "mainwidget.h"
#include "networkparser.h"

MainWidget::MainWidget(QThread *th, QWidget *parent) : QWidget(parent)
{
	initUI();
	parser = new NetworkParser(th);
	bool connected = parser->connectToHost("192.168.1.30", 3018);
	if (!connected) {
		qDebug() << "Failed to connect";
	} else {
		parser->requestTable(1, 0, 5, "", EntityType::BAND);
	}
}

void MainWidget::initUI()
{
	QFile stylesheetFile(":/files/stylesheet.txt");
	bool open = stylesheetFile.open(QIODevice::ReadOnly);
	qDebug() << open;
	QByteArray bytes = stylesheetFile.readAll();
	QString stylesheet(bytes);

	QGridLayout *lay = new QGridLayout();

	leftGroup = new QGroupBox();
	setupLeftPanel();
	lay->addWidget(leftGroup, 0, 0, 2, 1);

	searchGroup = new QGroupBox();
	setupSearch();
	lay->addWidget(searchGroup, 0, 1);

	setupTables();
	lay->addWidget(tables, 1, 1);

	playGroup = new QGroupBox;
	setupPlayArea(stylesheet);
	lay->addWidget(playGroup, 2, 0, 1, 2);

	setLayout(lay);
}

void MainWidget::setupLeftPanel()
{
	QVBoxLayout *leftLay = new QVBoxLayout;
	QPushButton *bands = new QPushButton(tr("Bands"));
	QPushButton *albums = new QPushButton(tr("Albums"));
	QPushButton *songs = new QPushButton(tr("Songs"));
	QPushButton *concerts = new QPushButton(tr("Concerts"));
	leftLay->addWidget(bands);
	leftLay->addWidget(albums);
	leftLay->addWidget(songs);
	leftLay->addWidget(concerts);
	leftLay->addStretch(20);
	leftGroup->setLayout(leftLay);

	connect(bands, &QPushButton::clicked, [this](bool) {
		this->tables->setCurrentIndex(0);
	});
	connect(albums, &QPushButton::clicked, [this](bool) {
		this->tables->setCurrentIndex(1);
	});
	connect(songs, &QPushButton::clicked, [this](bool) {
		this->tables->setCurrentIndex(2);
	});
//	connect(concerts, &QPushButton::clicked, [this](bool) {
//		this->tables->setCurrentIndex(0);
//	});
}

void MainWidget::setupSearch()
{
	QVBoxLayout *searchLay = new QVBoxLayout;
	QLineEdit *searchEdit = new QLineEdit;
	searchEdit->setPlaceholderText("Search");
	searchLay->addWidget(searchEdit);
	searchGroup->setLayout(searchLay);
}

void MainWidget::setupPlayArea(QString stylesheet)
{
	QGridLayout *playLay = new QGridLayout;

//	QProgressBar *progress = new QProgressBar;
//	progress->setMinimum(0);
//	progress->setMaximum(60);
//	progress->setValue(43);
//	progress->setMaximumHeight(20);
//	progress->setTextVisible(false);
//	progress->setStyleSheet(customStyleSheet);
//	playLay->addWidget(progress);
	QSlider *playSlider = new QSlider(Qt::Horizontal);
	playSlider->setMinimum(0);
	playSlider->setMaximum(100);
	playSlider->setValue(63);
	playSlider->setStyleSheet(stylesheet);

	QPushButton *playBtn = new QPushButton;
//	playBtn->setFixedSize(70, 70);
//	QRect rect(QPoint(), playBtn->size());
//	rect.adjust(10, 10, -10, -10);
//	QRegion region(rect,QRegion::Ellipse);
	playBtn->setStyleSheet(stylesheet);
//	playBtn->setMask(region);
//	playBtn->setFixedSize(50, 50);

	QVBoxLayout *songInfoLay = new QVBoxLayout;
	QPushButton *songNameBtn = new QPushButton("SongName");
	songNameBtn->setProperty("isFlat", true);
	songNameBtn->setStyleSheet("QPushButton { border: none; }");
	QPushButton *bandNameBtn = new QPushButton("BandName");
	bandNameBtn->setFlat(true);
	songInfoLay->addWidget(songNameBtn);
	songInfoLay->addWidget(bandNameBtn);
	songInfoLay->addStretch();
//	bandNameBtn->setStyleSheet("QPushButton { border: none; }");


	playLay->addWidget(playBtn, 0, 1);
	playLay->setAlignment(playBtn, Qt::AlignCenter);
	playLay->addWidget(playSlider, 1, 1);
	playLay->addLayout(songInfoLay, 0, 0, 2, 1);
	playGroup->setLayout(playLay);
}

void MainWidget::setupTables()
{
	tables = new QStackedWidget;

	QTableWidget *bands = new QTableWidget();
	QStringList cols;
	cols.append("Band");
	cols.append("Genre");
	cols.append("Founding date");
	cols.append("Termination date");
	bands->setRowCount(5);
	bands->setColumnCount(cols.length());
	bands->setHorizontalHeaderLabels(cols);
	bands->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	bands->setEditTriggers(QAbstractItemView::NoEditTriggers);
//	bands->horizontalHeader()->setVisible(true);
//	bands->verticalHeader()->setVisible(false);

	cols.clear();
	cols.append("Albums");
	cols.append("Band");
	cols.append("Release date");
	cols.append("Songs");
	QTableWidget *albums = new QTableWidget;
	albums->setRowCount(5);
	albums->setColumnCount(cols.length());
	albums->setHorizontalHeaderLabels(cols);
	albums->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	albums->setEditTriggers(QAbstractItemView::NoEditTriggers);

	cols.clear();
	cols.append("Song");
	cols.append("Album");
	cols.append("Band");
	cols.append("Length");
	QTableWidget *songs = new QTableWidget;
	songs->setRowCount(5);
	songs->setColumnCount(cols.length());
	songs->setHorizontalHeaderLabels(cols);
	songs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	songs->setEditTriggers(QAbstractItemView::NoEditTriggers);

	tables->addWidget(bands);
	tables->addWidget(albums);
	tables->addWidget(songs);

	tables->setCurrentIndex(0);
}
