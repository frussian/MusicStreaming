#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QProgressBar>

#include "mainwidget.h"

static QString customStyleSheet = "QPushButton {"
							"color: #333;"
							"border: 2px solid #555;"
							"border-radius: 20px;"
							"border-style: outset;"
							"background: qradialgradient("
							"cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4,"
							"radius: 1.35, stop: 0 #fff, stop: 1 #888"
							");"
							"padding: 5px;"
							"}"

							"QPushButton:hover {"
							"background: qradialgradient("
							"cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4,"
							"radius: 1.35, stop: 0 #fff, stop: 1 #bbb"
							");"
							"}"
							"QProgressBar {"
							"border: 2px solid grey;"
							"border-radius: 5px;"
							"text-align: center;"
							"}"
							"QProgressBar::chunk {"
							"background-color: #05B8CC;"
							"width: 20px;"
							"}";

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
	QGridLayout *lay = new QGridLayout();

	QGroupBox *leftGroup = new QGroupBox();
	QVBoxLayout *leftLay = new QVBoxLayout;
	QPushButton *bands = new QPushButton(tr("Bands"));
	QPushButton *albums = new QPushButton(tr("Albums"));
	QPushButton *songs = new QPushButton(tr("Songs"));
	QPushButton *concerts = new QPushButton(tr("Concerts"));
	leftLay->addWidget(bands);
	leftLay->addWidget(albums);
	leftLay->addWidget(songs);
	leftLay->addWidget(concerts);
	leftGroup->setLayout(leftLay);
	lay->addWidget(leftGroup, 0, 0, 3, 1);

	QGroupBox *searchGroup = new QGroupBox();
	QVBoxLayout *searchLay = new QVBoxLayout;
	QLineEdit *searchEdit = new QLineEdit;
	searchEdit->setPlaceholderText("Search");
	searchLay->addWidget(searchEdit);
	searchGroup->setLayout(searchLay);
	lay->addWidget(searchGroup, 0, 1);

	QTableWidget *table = new QTableWidget;
	table->setWindowTitle("Table");
	lay->addWidget(table, 1, 1);

	QGroupBox *playGroup = new QGroupBox;
	QVBoxLayout *playLay = new QVBoxLayout;
	QProgressBar *progress = new QProgressBar;  //TODO: use qslider
	progress->setMinimum(0);
	progress->setMaximum(60);
	progress->setValue(43);
	progress->setMaximumHeight(20);
	progress->setTextVisible(false);
	progress->setStyleSheet(customStyleSheet);
	playLay->addWidget(progress);

	QPushButton *playBtn = new QPushButton;
	playBtn->setStyleSheet(customStyleSheet);
	playBtn->setFixedSize(50, 50);
	playLay->addWidget(playBtn);
//	playLay->setAlignment(playBtn, )
	playGroup->setLayout(playLay);
	lay->addWidget(playGroup, 2, 1);
	setLayout(lay);
}
