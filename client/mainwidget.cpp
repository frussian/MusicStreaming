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
#include <QScrollBar>

#include "mainwidget.h"
#include "networkparser.h"
#include "audioplayer.h"

#include <iostream>

#define BAND_ID 0
#define ALBUM_ID 1
#define SONG_ID 2
#define CONCERT_ID 3

#define TABLE_GROW 3

int MainWidget::dataRole = Qt::UserRole + 1;

MainWidget::MainWidget(QThread *th, QWidget *parent) : QWidget(parent)
{
	qRegisterMetaType<uint64_t>("uint64_t");
	qRegisterMetaType<TableAns>("TableAns");
	qRegisterMetaType<SimpleAns>("SimpleAns");
	qRegisterMetaType<StreamAns>("StreamAns");
	qRegisterMetaType<EntityType>("EntityType");
	parser = new NetworkParser(th);
	initUI();

	connect(parser, &NetworkParser::tableAns, this, &MainWidget::tableAns);
	connect(parser, &NetworkParser::simpleAns, this, &MainWidget::simpleAns);
	connect(parser, &NetworkParser::streamAns, this, &MainWidget::streamAns);

	connect(parser, &NetworkParser::parserConnected, this, &MainWidget::parserConnected);
	connect(this, &MainWidget::connectToHost, parser, &NetworkParser::connectToHost);

	connect(parser, &NetworkParser::reqFailed, this, &MainWidget::reqFailed);
	connect(this, &MainWidget::requestTableParser, parser, &NetworkParser::requestTable);
	connect(this, &MainWidget::simpleRequestParser, parser, &NetworkParser::simpleRequest);
	connect(this, &MainWidget::streamRequestParser, parser, &NetworkParser::streamRequest);

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

//	playGroup = new QGroupBox;
	player = new AudioPlayer(stylesheet);
//	playGroup->
//	lay->addWidget(player, 2, 0, 1, 2);

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
	connect(concerts, &QPushButton::clicked, this, &MainWidget::clicked);
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

void MainWidget::setupTables()
{
	tables = new QStackedWidget;
	int rowNum = 10;

	QTableWidget *bands = new QTableWidget();
	QStringList cols;

	cols << "Band" << "Genre" << "Founding date" << "Termination date";
	bands->setRowCount(rowNum);
	bands->setColumnCount(cols.length());
	bands->setHorizontalHeaderLabels(cols);

	cols.clear();
	cols << "Albums" << "Band" << "Release date" << "Songs";
	QTableWidget *albums = new QTableWidget;
	albums->setRowCount(rowNum);
	albums->setColumnCount(cols.length());
	albums->setHorizontalHeaderLabels(cols);

	cols.clear();
	cols << "Song" << "Album" << "Band" << "Length";
	QTableWidget *songs = new QTableWidget;
	songs->setRowCount(rowNum);
	songs->setColumnCount(cols.length());
	songs->setHorizontalHeaderLabels(cols);

	cols.clear();
	cols << "Description" << "Location" << "Time";
	QTableWidget *concerts = new QTableWidget;
	concerts->setRowCount(rowNum);
	concerts->setColumnCount(cols.length());
	concerts->setHorizontalHeaderLabels(cols);

	tables->addWidget(bands);
	tables->addWidget(albums);
	tables->addWidget(songs);
	tables->addWidget(concerts);

	tables->setCurrentIndex(0);

	for (int i = 0; i < 4; i++) {
		QTableWidget *w = dynamic_cast<QTableWidget*>(tables->widget(i));
		w->setProperty("index", i);
		connect(w, &QTableWidget::cellDoubleClicked, this, &MainWidget::tableClicked);
		w->verticalScrollBar()->setProperty("index", i);
		connect(w->verticalScrollBar(), &QScrollBar::valueChanged,
				this, &MainWidget::tableScrolled);
		w->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		w->setEditTriggers(QAbstractItemView::NoEditTriggers);
//		w->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	}
}

void MainWidget::requestTable(int first, int last,
							 QString filter, enum EntityType type)
{
	for (auto old: requests) {
		if (old.first <= first && first <= old.last) {
			first = old.last;
		}
		if (old.first <= last && last <= old.last) {
			last = old.first;
		}
	}
	if (first > last) {
		qDebug() << "req already exists";
		return;
	}
	emit requestTableParser(reqId, first, last, filter, type);
	requests[reqId] = Req{first, last, "", type};
	reqId++;
}

void MainWidget::simpleRequest(QString name, enum EntityType type)
{
	emit simpleRequestParser(reqId, name, type);
	Req req;
	req.name = name;
	req.type = type;
	requests[reqId] = req;
	reqId++;
}

void MainWidget::streamRequest(QString name, uint32_t size, enum EntityType type)
{
	emit streamRequestParser(reqId, name, size, type);
	Req req;
	req.name = name;
	req.type = type;
	requests[reqId] = req;
	reqId++;
}

void MainWidget::clicked()
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	if (tables->currentIndex() == id) return;

	this->tables->setCurrentIndex(id);
	QString filter = searchEdit->text();
	scrollTable(id);
//	requestTable(0, 5, filter, EntityType(id));
}

void MainWidget::tableClicked(int row, int col)
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	QTableWidget *table = dynamic_cast<QTableWidget*>(tables->widget(id));
	auto item = table->item(row, col);
	if (item) {
		qDebug() << table->visualItemRect(item);
	} else {
		qDebug() << "null";
	}

	switch (id) {
	case BAND_ID: {
		if (col != 0) return;
		QTableWidgetItem *item = table->item(row, 0);
		if (!item) return;
//		QVariant var = item->data(dataRole);
//		Band b = var.value<Band>();
//		qDebug() << QString::fromStdString(b.bandname());
		QString band = item->data(Qt::DisplayRole).toString();
		simpleRequest(band, EntityType::BAND);
		break;
	}
	case SONG_ID: {
		if (col != 0) return;
		QTableWidgetItem *item = table->item(row, 0);
		if (!item) return;
		QVariant var = item->data(Qt::DisplayRole);
		QString song = var.toString();
		qDebug() << song;
		streamRequest(song, 8192, EntityType::SONG);
	}
	}
}

void MainWidget::tableScrolled(int value)
{
	(void)value;
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	scrollTable(id);
}

void MainWidget::scrollTable(int id)
{
	QTableWidget *table = dynamic_cast<QTableWidget*>(tables->widget(id));
	int h = table->viewport()->height();
	qDebug() << h;
	int first = -1;
	int last = 0;
	for (int i = 0; i < table->rowCount(); i++) {
		int pos = table->rowViewportPosition(i);
		if (0 <= pos && pos <= h && !table->item(i, 0)) {
			qDebug() << i+1 << "visible" << pos << table->item(i, 0);
			if (first == -1) {
				first = i;
			}
			last = i;
		}
//		qDebug() << "row" << i << table->rowViewportPosition(i);

	}

	if (first == -1) {
		return;
	}

	qDebug() << "req" << first << last;
	requestTable(first, last, searchEdit->text(), EntityType(id));
}

void MainWidget::searchChanged(QString filter)
{
	(void)filter;
	int id = tables->currentIndex();
	QTableWidget *table = dynamic_cast<QTableWidget*>(tables->widget(id));
	table->clearContents();
	scrollTable(id);
//	requestTable(0, 5, filter, EntityType(id));
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
//	bands->clearContents();
	for (int i = 0; i < ans->bands_size(); i++) {
		Band band = ans->bands(i);

		const std::string &name = band.bandname();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(name));
		item->setData(dataRole, QVariant::fromValue(band));
		bands->setItem(i+req.first, 0, item);

		QString genre = getGenre(band.genre());
		item = new QTableWidgetItem(genre);
		bands->setItem(i+req.first, 1, item);

		int64_t unix = band.unixfounddate();
		if (unix) {
			QDateTime found = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(found.toString("yyyy"));
			bands->setItem(i+req.first, 2, item);
		}
		unix = band.unixtermdate();
		if (unix) {
			QDateTime term = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(term.toString("yyyy"));
			bands->setItem(i+req.first, 3, item);
		}
	}
}

void MainWidget::handleAlbumInsertion(Req &req, TableAns *ans)
{
	QTableWidget *albums = dynamic_cast<QTableWidget*>(tables->widget(ALBUM_ID));
//	albums->clearContents();
	for (int i = 0; i < ans->albums_size(); i++) {
		Album album = ans->albums(i);

		const std::string &albumname = album.title();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(albumname));
		item->setData(dataRole, QVariant::fromValue(album));
		albums->setItem(i+req.first, 0, item);

		const std::string &bandname = album.bandname();
		item = new QTableWidgetItem(QString::fromStdString(bandname));
		albums->setItem(i+req.first, 1, item);

		int64_t unix = album.unixreleasedate();
		if (unix) {
			QDateTime found = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
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
//	songs->clearContents();
	for (int i = 0; i < ans->songs_size(); i++) {
		Song song = ans->songs(i);

		const std::string &songname = song.songname();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(songname));
		item->setData(dataRole, QVariant::fromValue(song));
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

void MainWidget::handleConcertInsertion(Req &req, TableAns *ans)
{
	QTableWidget *concerts = dynamic_cast<QTableWidget*>(tables->widget(CONCERT_ID));
//	songs->clearContents();
	for (int i = 0; i < ans->concerts_size(); i++) {
		Concert concert = ans->concerts(i);
		qDebug() << concert.description().data();
		qDebug() << concert.location().data();

		const std::string &descr = concert.description();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(descr));
		item->setData(dataRole, QVariant::fromValue(concert));
		concerts->setItem(i+req.first, 0, item);

		const std::string &loc = concert.location();
		item = new QTableWidgetItem(QString::fromStdString(loc));
		concerts->setItem(i+req.first, 1, item);

		int64_t unix = concert.unixdatetime();
		if (unix) {
			QDateTime t = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(t.toString("dd-MM-yyyy hh:mm"));
			concerts->setItem(i+req.first, 2, item);
		}
	}
}

void MainWidget::parserConnected()
{
	int id = tables->currentIndex();
	scrollTable(id);
//	requestTable(0, 5, searchEdit->text(), EntityType(id));
}

void MainWidget::reqFailed(uint64_t reqId)
{
	qDebug() << "request" << reqId << "failed";
	requests.remove(reqId);
}

void MainWidget::tableAns(uint64_t reqId, TableAns ans)
{
	int id = -1;
	QMap<uint64_t, Req>::iterator req = requests.find(reqId);
	if (req == requests.end()) {
		qWarning() << "unknown req id" << reqId;
		return;
	}

	switch(ans.type()) {
	case EntityType::BAND: {
		id = BAND_ID;
		handleBandInsertion(req.value(), &ans);
		break;
	}
	case EntityType::ALBUM: {
		id = ALBUM_ID;
		handleAlbumInsertion(req.value(), &ans);
		break;
	}
	case EntityType::SONG: {
		id = SONG_ID;
		handleSongInsertion(req.value(), &ans);
		break;
	}
	case EntityType::CONCERT: {
		id = CONCERT_ID;
		handleConcertInsertion(req.value(), &ans);
		break;
	}
	}

	requests.erase(req);
	if (id == -1) return;

	QTableWidget *table = dynamic_cast<QTableWidget*>(tables->widget(id));
	int last = table->rowCount()-1;
	if (table->item(last, 0)) {
		table->setRowCount(last+1+TABLE_GROW);
		scrollTable(id);  //load new rows if visible
	}

}

void MainWidget::simpleAns(uint64_t reqId, SimpleAns ans)
{
	requests.remove(reqId);
	auto type = ans.msg_case();
	switch (type) {
	case SimpleAns::kBand: {
		const Band &band = ans.band();
		qDebug() << QString::fromStdString(band.bandname()) <<
					band.albumnames_size() <<
					band.participants_size() <<
					band.concerts_size();
		break;
	}
	}
}

void MainWidget::streamAns(uint64_t reqId, StreamAns ans)
{
	QMap<uint64_t, Req>::iterator req = requests.find(reqId);
	if (req == requests.end()) {
		qWarning() << "unknown req id" << reqId;
		return;
	}

	const std::string &data = ans.data();
//	qDebug() << "data size" << data.size();
	switch (req.value().type) {
	case EntityType::SONG: {
		if (data.size() != 0) {
//			qDebug() << data.;
//			qDebug() << data.at(0) << data.at(1);
//			printf("%d %d\n", data.at(0), data.at(1));
			//QByteArray::fromRawData?
			player->writeOpusData(QByteArray(data.data(), data.size()));
		}

		if (ans.isfinal()) {
			requests.remove(reqId);
			qDebug() << "end of transfer";
		}
		break;
	}
	default: {
		qDebug() << "unsupported stream type" << req.value().type;
	}
	}
}



