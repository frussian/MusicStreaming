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
#include <QDateTime>

#include "mainwidget.h"
#include "networkparser.h"

#define BAND_ID 0
#define ALBUM_ID 1
#define SONG_ID 2
#define CONCERT_ID 3

MainWidget::MainWidget(QThread *th, QWidget *parent) : QWidget(parent)
{
	qRegisterMetaType<uint64_t>("uint64_t");
	qRegisterMetaType<TableAns>("TableAns");
	qRegisterMetaType<EntityType>("EntityType");

	initUI();
	parser = new NetworkParser(th);
	connect(parser, &NetworkParser::tableAns, this, &MainWidget::tableAns);
	connect(parser, &NetworkParser::parserConnected, this, &MainWidget::parserConnected);
	connect(this, &MainWidget::connectToHost, parser, &NetworkParser::connectToHost);
	connect(this, &MainWidget::requestTableParser, parser, &NetworkParser::requestTable);

	emit connectToHost("192.168.1.30", 3018);

//	bool connected = parser->connectToHost("192.168.1.30", 3018);
//	if (!connected) {
//		qDebug() << "Failed to connect";
//	} else {
//		requestTable(0, 5, "", EntityType::BAND);
//	}
}

void MainWidget::initUI()
{
	QFile stylesheetFile(":/files/stylesheet.txt");
	stylesheetFile.open(QIODevice::ReadOnly);
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

	bands->setProperty("index", BAND_ID);
	albums->setProperty("index", ALBUM_ID);
	songs->setProperty("index", SONG_ID);
	concerts->setProperty("index", CONCERT_ID);

	connect(bands, &QPushButton::clicked, this, &MainWidget::clicked);
	connect(albums, &QPushButton::clicked, this, &MainWidget::clicked);
	connect(songs, &QPushButton::clicked, this, &MainWidget::clicked);
//	connect(concerts, &QPushButton::clicked, [this](bool) {
//		this->tables->setCurrentIndex(0);
//	});
}

void MainWidget::setupSearch()
{
	QVBoxLayout *searchLay = new QVBoxLayout;
	searchEdit = new QLineEdit;
	searchEdit->setPlaceholderText("Search");
	searchLay->addWidget(searchEdit);
	searchGroup->setLayout(searchLay);

	connect(searchEdit, &QLineEdit::textChanged,
			this, &MainWidget::searchChanged);
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

	for (int i = 0; i < 3; i++) {
		QTableWidget *w = dynamic_cast<QTableWidget*>(tables->widget(i));
		w->setProperty("index", i);
		connect(w, &QTableWidget::cellDoubleClicked, this, &MainWidget::tableClicked);
	}
}

void MainWidget::requestTable(int first, int last,
							 QString filter, enum EntityType type)
{
	emit requestTableParser(reqId, first, last, filter, type);
	requests[reqId] = Req{first, last};
	reqId++;
}

void MainWidget::clicked()
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	if (tables->currentIndex() == id) return;

	this->tables->setCurrentIndex(id);
	QString filter = searchEdit->text();
	requestTable(0, 5, filter, EntityType(id));
}

void MainWidget::tableClicked(int row, int col)
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	QTableWidget *table = dynamic_cast<QTableWidget*>(tables->widget(id));
	switch (id) {
	case BAND_ID: {
		if (col != 0) return;

	}
	}
}

void MainWidget::searchChanged(QString filter)
{
	int id = tables->currentIndex();
	requestTable(0, 5, filter, EntityType(id));
}

QString getGenre(enum Genre genre)
{
	switch (genre) {
	case ROCK: return "Rock";
	case ALTERNATIVE: return "Alternative";
	case INDIE: return "Indie";
	case BLUES: return "Blues";
	case METAL: return "Metal";
	}
	return "";
}

void MainWidget::handleBandInsertion(Req &req, TableAns *ans)
{
	QTableWidget *bands = dynamic_cast<QTableWidget*>(tables->widget(BAND_ID));
	bands->clearContents();
	for (int i = 0; i < ans->bands_size(); i++) {
		Band band = ans->bands(i);

		const std::string &name = band.bandname();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(name));
		bands->setItem(i+req.first, 0, item);

		QString genre = getGenre(band.genre());
		item = new QTableWidgetItem(genre);
		bands->setItem(i+req.first, 1, item);

		int64_t unix = band.unixfounddate();
		if (unix) {
			QDateTime found = QDateTime::fromSecsSinceEpoch(unix);
			item = new QTableWidgetItem(found.toString("yyyy"));
			bands->setItem(i+req.first, 2, item);
		}
		unix = band.unixtermdate();
		if (unix) {
			QDateTime term = QDateTime::fromSecsSinceEpoch(unix);
			item = new QTableWidgetItem(term.toString("yyyy"));
			bands->setItem(i+req.first, 3, item);
		}
	}
}

void MainWidget::handleAlbumInsertion(Req &req, TableAns *ans)
{
	QTableWidget *albums = dynamic_cast<QTableWidget*>(tables->widget(ALBUM_ID));
	albums->clearContents();
	for (int i = 0; i < ans->albums_size(); i++) {
		Album album = ans->albums(i);

		const std::string &albumname = album.title();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(albumname));
		albums->setItem(i+req.first, 0, item);

		const std::string &bandname = album.bandname();
		item = new QTableWidgetItem(QString::fromStdString(bandname));
		albums->setItem(i+req.first, 1, item);

		int64_t unix = album.unixreleasedate();
		if (unix) {
			QDateTime found = QDateTime::fromSecsSinceEpoch(unix);
			item = new QTableWidgetItem(found.toString("yyyy-MM-dd"));
			albums->setItem(i+req.first, 2, item);
		}

		int nsongs = album.songs_size();
		item = new QTableWidgetItem(QString::number(nsongs));
		albums->setItem(i+req.first, 3, item);
	}
}

void MainWidget::handleSongInsertion(Req &req, TableAns *ans)
{
	QTableWidget *songs = dynamic_cast<QTableWidget*>(tables->widget(SONG_ID));
	songs->clearContents();
	for (int i = 0; i < ans->songs_size(); i++) {
		Song song = ans->songs(i);

		const std::string &songname = song.songname();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(songname));
		songs->setItem(i+req.first, 0, item);

		const std::string &albumname = song.albumname();
		item = new QTableWidgetItem(QString::fromStdString(albumname));
		songs->setItem(i+req.first, 1, item);

		const std::string &bandname = song.bandname();
		item = new QTableWidgetItem(QString::fromStdString(bandname));
		songs->setItem(i+req.first, 2, item);

		QTime length = QTime(0, 0, 0, 0).addSecs(song.lengthsec());
		item = new QTableWidgetItem(length.toString("mm:ss"));
		songs->setItem(i+req.first, 3, item);
	}
}

void MainWidget::parserConnected()
{
	int id = tables->currentIndex();
	requestTable(0, 5, searchEdit->text(), EntityType(id));
}

void MainWidget::tableAns(uint64_t reqId, TableAns ans)
{
	switch(ans.type()) {
	case EntityType::BAND: {
		QMap<uint64_t, Req>::iterator req = requests.find(reqId);
		if (req == requests.end()) {
			qWarning() << "unknown req id" << reqId;
			return;
		}
		handleBandInsertion(req.value(), &ans);
		requests.erase(req);
		break;
	}
	case EntityType::ALBUM: {
		QMap<uint64_t, Req>::iterator req = requests.find(reqId);
		if (req == requests.end()) {
			qWarning() << "unknown req id" << reqId;
			return;
		}
		handleAlbumInsertion(req.value(), &ans);
		requests.erase(req);
		break;
	}
	case EntityType::SONG: {
		QMap<uint64_t, Req>::iterator req = requests.find(reqId);
		if (req == requests.end()) {
			qWarning() << "unknown req id" << reqId;
			return;
		}
		handleSongInsertion(req.value(), &ans);
		requests.erase(req);
		break;
	}
	}

}







