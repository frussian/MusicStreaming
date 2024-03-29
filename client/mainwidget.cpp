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
#include <QGroupBox>
#include <QMouseEvent>
#include <QScrollArea>

#include "mainwidget.h"
#include "networkparser.h"
#include "audioplayer.h"

#include <iostream>

#define BAND_CLICK_ID 0
#define ALBUM_CLICK_ID 1
#define SONG_CLICK_ID 2
#define MUSICIAN_CLICK_ID 3

#define BAND_TABLE_ID 0
#define ALBUM_TABLE_ID 1
#define SONG_TABLE_ID 2
#define CONCERT_TABLE_ID 3
#define BAND_PAGE_ID 4
#define ALBUM_PAGE_ID 5
#define MUSICIAN_PAGE_ID 6

#define TABLE_GROW 3

int MainWidget::dataRole = Qt::UserRole + 1;

ClickableLabel::ClickableLabel(int typeId, QString text, QWidget *parent):
	QLabel(text, parent), typeId(typeId)
{
	setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *evt)
{
	emit clicked(typeId, text());
}

AlbumView::AlbumView(QString name, QString band, int nsongs):
	title(name)
{
	QVBoxLayout *lay = new QVBoxLayout;
	setLayout(lay);
	lay->addWidget(new QLabel(name));
	lay->addWidget(new QLabel(band));
	if (nsongs != 0) {
		lay->addWidget(new QLabel("Songs: " + QString::number(nsongs)));
	}
	setFrameShape(QFrame::Box);
	setMaximumSize(QSize(100, 50));
//	setStyleSheet("hover");
	setCursor(Qt::PointingHandCursor);
}

AlbumView::~AlbumView()
{
}

void AlbumView::mousePressEvent(QMouseEvent *evt)
{
	emit clicked(ALBUM_CLICK_ID, title);
}

MainWidget::MainWidget(QThread *th, QWidget *parent) : QWidget(parent)
{
	qRegisterMetaType<uint64_t>("uint64_t");
	qRegisterMetaType<TableAns>("TableAns");
	qRegisterMetaType<SimpleAns>("SimpleAns");
	qRegisterMetaType<StreamAns>("StreamAns");
	qRegisterMetaType<EntityType>("EntityType");
	parser = new NetworkParser(th);
	player = new AudioPlayer;

	searchTimer.setSingleShot(true);
	connect(&searchTimer, &QTimer::timeout, this, &MainWidget::searchChangedTimer);

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
	connect(this, &MainWidget::cancelStreamRequestParser, parser, &NetworkParser::cancelStreamReq);

	connect(this, &MainWidget::startPlayer, player, &AudioPlayer::start);
	connect(this, &MainWidget::stopPlayer, player, &AudioPlayer::stop);
	connect(player, &AudioPlayer::processedUSecs, this, &MainWidget::processedUSecs);
	connect(this, &MainWidget::seekPlayer, player, &AudioPlayer::seek);

	emit connectToHost("127.0.0.1", 3018);

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

	mainPage = new QStackedWidget;
	setupTables();
	setupPages();
	mainPage->setCurrentIndex(BAND_TABLE_ID);
	lay->addWidget(mainPage, 1, 1);

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

	bands->setProperty("index", BAND_TABLE_ID);
	albums->setProperty("index", ALBUM_TABLE_ID);
	songs->setProperty("index", SONG_TABLE_ID);
	concerts->setProperty("index", CONCERT_TABLE_ID);

	connect(bands, &QPushButton::clicked, this, &MainWidget::clickedLeftPanel);
	connect(albums, &QPushButton::clicked, this, &MainWidget::clickedLeftPanel);
	connect(songs, &QPushButton::clicked, this, &MainWidget::clickedLeftPanel);
	connect(concerts, &QPushButton::clicked, this, &MainWidget::clickedLeftPanel);
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
	playSlider = new QSlider(Qt::Horizontal);
	connect(playSlider, &QSlider::sliderReleased, this, &MainWidget::playSliderChanged);
	playSlider->setMinimum(0);
	playSlider->setMaximum(100);
	playSlider->setValue(0);
	playSlider->setStyleSheet(stylesheet);

	volSlider = new QSlider(Qt::Horizontal);
	connect(volSlider, &QSlider::valueChanged, player, &AudioPlayer::volChanged);
	volSlider->setMinimum(0);
	volSlider->setMaximum(100);
	volSlider->setValue(100);
	volSlider->setStyleSheet(stylesheet);

	playBtn = new QPushButton;
	playBtn->setStyleSheet(stylesheet);
	playBtn->setCheckable(true);

	connect(playBtn, &QPushButton::toggled, this, &MainWidget::playPressed);

	QVBoxLayout *songInfoLay = new QVBoxLayout;
	songNameLbl = new QLabel("");
	bandNameLbl = new ClickableLabel(BAND_CLICK_ID, "");
	connect(bandNameLbl, &ClickableLabel::clicked, this, &MainWidget::clickedReq);
	songInfoLay->addWidget(songNameLbl);
	songInfoLay->addWidget(bandNameLbl);
	songInfoLay->addStretch();

	currTimeBtn = new QPushButton;
	currTimeBtn->setProperty("isFlat", true);
	currTimeBtn->setStyleSheet("QPushButton { border: none; }");

	endTimeBtn = new QPushButton;
	endTimeBtn->setProperty("isFlat", true);
	endTimeBtn->setStyleSheet("QPushButton { border: none; }");

	QLabel endLbl("end");
//	bandNameBtn->setStyleSheet("QPushButton { border: none; }");

	playLay->addWidget(playBtn, 0, 2, Qt::AlignCenter);
//	playLay->setAlignment(playBtn, Qt::AlignCenter);
	playLay->addWidget(playSlider, 1, 2);
	playLay->addWidget(currTimeBtn, 1, 1);
	playLay->addWidget(endTimeBtn, 1, 3);
	playLay->addWidget(volSlider, 1, 4);
	playLay->addLayout(songInfoLay, 0, 0, 2, 1);

//	playLay->setColumnStretch(0, 3);
//	playLay->setColumnStretch(1, 3);
	playLay->setColumnStretch(2, 3);
//	playLay->setColumnStretch(3, 3);
	playLay->setColumnStretch(4, 1);

	playGroup->setLayout(playLay);
}

void MainWidget::playPressed(bool checked)
{
	if (!checked) {
		emit startPlayer(false);
	} else {
		emit stopPlayer(false);
	}
}

void MainWidget::processedUSecs(quint64 usecs)
{
	int secs = usecs / 1000000;
	playSlider->setValue(secs / (float)currSongSecs*100);
	QTime curr = QTime(0, 0, 0, 0).addSecs(secs);
	currTimeBtn->setText(curr.toString("mm:ss"));
//	qDebug() << secs << secs / (float)currSongSecs*100;
}

void MainWidget::playSliderChanged()
{
	qDebug() << playSlider->value();
	QString timeStr = endTimeBtn->text();
	QTime time = QTime::fromString(timeStr, "mm:ss");
	int endSecs = time.minute() * 60 + time.second();

	int val = playSlider->value();
	int secs = val / 100.f * endSecs;
//	emit seekPlayer(secs);
	qDebug() << secs << endSecs;
	//TODO: set offset and add later to elapsed seconds
}

void MainWidget::setupTables()
{
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

	mainPage->addWidget(bands);
	mainPage->addWidget(albums);
	mainPage->addWidget(songs);
	mainPage->addWidget(concerts);

	for (int i = 0; i < 4; i++) {
		QTableWidget *w = dynamic_cast<QTableWidget*>(mainPage->widget(i));
		w->setProperty("index", i);
//		connect(w, &QTableWidget::cellDoubleClicked, this, &MainWidget::tableClicked);
		w->verticalScrollBar()->setProperty("index", i);
		connect(w->verticalScrollBar(), &QScrollBar::valueChanged,
				this, &MainWidget::tableScrolled);
		w->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		w->setEditTriggers(QAbstractItemView::NoEditTriggers);
//		w->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	}
}

void MainWidget::setupPages()
{
	setupBandPage();
	setupAlbumPage();
	setupMusicianPage();
}

void MainWidget::setupBandPage()
{
	QVBoxLayout *lay = new QVBoxLayout;
	QWidget *page = new QWidget;
	page->setLayout(lay);

	QGroupBox *box = new QGroupBox;
	box->setTitle("Band");
	QVBoxLayout *bandlay = new QVBoxLayout;
	bandlay->setSizeConstraint(QLayout::SetMinimumSize);

	QLabel * lbl = new QLabel("BandName");
	lbl->setObjectName("band");
	lbl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	bandlay->addWidget(lbl, 1, Qt::AlignLeft);

	lbl = new QLabel("Description");
	lbl->setObjectName("description");
	lbl->setWordWrap(true);
	lbl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	bandlay->addWidget(lbl, 15, Qt::AlignLeft);

	box->setLayout(bandlay);
	lay->addWidget(box);

	box = new QGroupBox;
	box->setTitle("Albums");
	box->setLayout(new QVBoxLayout);
	lay->addWidget(box);

	box = new QGroupBox;
	box->setTitle("Participants");
	QStringList cols;
	cols << "Musician" << "Entry Date" << "Quit date";
	QTableWidget *parts = new QTableWidget(0, cols.size());
	parts->setHorizontalHeaderLabels(cols);
	parts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	parts->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QVBoxLayout *blay = new QVBoxLayout;
	blay->addWidget(parts);
	box->setLayout(blay);

	lay->addWidget(box);

	box = new QGroupBox;
	box->setTitle("Concerts");
	cols.clear();
	cols << "Location" << "Time" << "Description" << "Capacity";
	QTableWidget *concerts = new QTableWidget(0, cols.size());
	concerts->setHorizontalHeaderLabels(cols);
	concerts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	concerts->setEditTriggers(QAbstractItemView::NoEditTriggers);
	blay = new QVBoxLayout;
	blay->addWidget(concerts);
	box->setLayout(blay);

	lay->addWidget(box);

	auto area = new QScrollArea;
	area->setWidget(page);
//	area->set
	area->setWidgetResizable(true);

	mainPage->addWidget(area);
}

void MainWidget::setupAlbumPage()
{
	QVBoxLayout *lay = new QVBoxLayout;
	QWidget *page = new QWidget;
	page->setLayout(lay);

	QHBoxLayout *hlay = new QHBoxLayout;

	ClickableLabel *clbl = new ClickableLabel(BAND_CLICK_ID);
	clbl->setObjectName("band");
	connect(clbl, &ClickableLabel::clicked, this, &MainWidget::clickedReq);
	hlay->addWidget(clbl);

	QLabel *lbl = new QLabel;
	lbl->setObjectName("album");
	hlay->addWidget(lbl);

	lbl = new QLabel;
	lbl->setObjectName("date");
	hlay->addWidget(lbl);

	lay->addLayout(hlay);

	QStringList cols;
	cols << "Name" << "Length";
	QTableWidget *songs = new QTableWidget(0, cols.size());
	songs->setHorizontalHeaderLabels(cols);
	songs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	songs->setEditTriggers(QAbstractItemView::NoEditTriggers);
//	connect(songs, &QTableWidget::cellDoubleClicked, this, &MainWidget::albumSongsTableClicked);

	lay->addWidget(songs);

	mainPage->addWidget(page);
}

void MainWidget::setupMusicianPage()
{
	QVBoxLayout *lay = new QVBoxLayout;
	QWidget *page = new QWidget;
	page->setLayout(lay);

	QLabel *lbl = new QLabel;
	lbl->setObjectName("name");
	lay->addWidget(lbl);

	lbl = new QLabel;
	lbl->setObjectName("date");
	lay->addWidget(lbl);

	lbl = new QLabel;
	lbl->setObjectName("bio");
	lay->addWidget(lbl);

	QStringList cols;
	cols << "Band" << "Entry date" << "Quit date";
	QTableWidget *table = new QTableWidget(0, cols.size());
	table->setHorizontalHeaderLabels(cols);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	lay->addWidget(table);

	mainPage->addWidget(page);
}

void MainWidget::requestTable(int first, int last,
							 QString filter, enum EntityType type)
{
	for (auto old: requests) {
		if (type != old.type) continue;
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
	if (type == SONG) {
		if (currSongReqId != 0) {
			emit cancelStreamRequestParser(currSongReqId);
		}
		currSongReqId = reqId;
	}
	emit streamRequestParser(reqId, name, size, type);
	Req req;
	req.name = name;
	req.type = type;
	requests[reqId] = req;
	reqId++;
}

ClickableLabel *MainWidget::getLabel(int typeId, QString text)
{
	ClickableLabel *l = new ClickableLabel(typeId, text);
	connect(l, &ClickableLabel::clicked, this, &MainWidget::clickedReq);
	return l;
}

void MainWidget::processSongClick(Song &song)
{
	QString songName = QString::fromStdString(song.songname());
	QString bandName = QString::fromStdString(song.bandname());
	currSongSecs = song.lengthsec();

	player->reset();  //in different thread
	playBtn->setChecked(false);
	emit startPlayer(true);
#if RANGE_REQUESTS_FOR_SONGS == 0
	streamRequest(songName, 8000, EntityType::SONG);
#endif
	songNameLbl->setText(songName);
	bandNameLbl->setText(bandName);
	currTimeBtn->setText("0:00");
	QTime length = QTime(0, 0, 0, 0).addSecs(song.lengthsec());
	endTimeBtn->setText(length.toString("mm:ss"));

}

//void MainWidget::albumSongsTableClicked(int row, int col)
//{
//	if (col != 0) return;
//	QTableWidget *songs = dynamic_cast<QTableWidget*>(sender());
//	if (!songs) return;

//	QTableWidgetItem *item = songs->item(row, 0);
//	if (!item) return;
//	Song song = item->data(dataRole).value<Song>();
//	QString band = QString::fromStdString(song.bandname());
//	processSongClick(song, band);
//}

void MainWidget::clickedReq(int typeId, QString text)
{
	qDebug() << text << typeId;

	switch (typeId) {
	case BAND_CLICK_ID: {
		simpleRequest(text, EntityType::BAND);
		break;
	}
	case ALBUM_CLICK_ID: {
		simpleRequest(text, EntityType::ALBUM);
		break;
	}
	case SONG_CLICK_ID: {
		ClickableLabel *lbl = dynamic_cast<ClickableLabel*>(sender());
		if (!lbl) return;

		Song song = lbl->property("song").value<Song>();
		qDebug() << QString::fromStdString(song.songname()) <<
					QString::fromStdString(song.bandname());
		QString band = QString::fromStdString(song.bandname());
		processSongClick(song);
		break;
	}
	case MUSICIAN_CLICK_ID: {
		simpleRequest(text, EntityType::MUSICIAN);
		break;
	}
	}
}

void MainWidget::clickedLeftPanel()
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	if (mainPage->currentIndex() == id) return;

	this->mainPage->setCurrentIndex(id);
	QString filter = searchEdit->text();
	scrollTable(id);
//	requestTable(0, 5, filter, EntityType(id));
}

//void MainWidget::tableClicked(int row, int col)
//{
//	QObject *obj = sender();
//	int id = obj->property("index").toInt();
//	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));

//	switch (id) {
//	case BAND_TABLE_ID: {
//		if (col != 0) return;
//		QTableWidgetItem *item = table->item(row, 0);
//		if (!item) return;
////		QVariant var = item->data(dataRole);
////		Band b = var.value<Band>();
////		mainPage->setCurrentIndex(BAND_PAGE_ID);
//		QString band = item->data(Qt::DisplayRole).toString();
//		simpleRequest(band, EntityType::BAND);
//		break;
//	}
//	case SONG_TABLE_ID: {
//		if (col != 0) return;
//		QTableWidgetItem *item = table->item(row, 0);
//		if (!item) return;
//		Song song = item->data(dataRole).value<Song>();
//		item = table->item(row, 2);
//		QString band = item->data(Qt::DisplayRole).toString();
//		processSongClick(song, band);
//		break;
//	}
//	case ALBUM_TABLE_ID: {
//		if (col != 0) return;
//		QTableWidgetItem *item = table->item(row, 0);
//		if (!item) return;
//		QString album = item->data(Qt::DisplayRole).toString();
//		clickedReq(ALBUM_CLICK_ID, album);
//		break;
//	}
//	}
//}

void MainWidget::tableScrolled(int value)
{
	(void)value;
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	scrollTable(id);
}

void MainWidget::scrollTable(int id)
{
	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
	int h = table->viewport()->height();
	int first = -1;
	int last = 0;
	for (int i = 0; i < table->rowCount(); i++) {
		int pos = table->rowViewportPosition(i);
		if (0 <= pos && pos <= h &&
				(!table->cellWidget(i, 0) && !table->item(i, 0))) {
			if (first == -1) {
				first = i;
			}
			last = i;
		}

	}

	if (first == -1) {
		return;
	}

	requestTable(first, last, searchEdit->text(), EntityType(id));
}

void MainWidget::searchChanged(QString filter)
{
	(void)filter;
	searchTimer.stop();
	searchTimer.start(500);
//	requestTable(0, 5, filter, EntityType(id));
}

void MainWidget::searchChangedTimer()
{
	qDebug() << "search changed timer";
	int id = mainPage->currentIndex();
	if (id >= BAND_PAGE_ID) return;
	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
	table->clearContents();
	scrollTable(id);
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

QString getFmtDateFromUnix(int64_t unix, QString fmt) {
	if (unix) {
		QDateTime d = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
		return d.toString(fmt);
	}
	return "";
}

void MainWidget::handleBandInsertion(Req &req, TableAns *ans)
{
	QTableWidget *bands = dynamic_cast<QTableWidget*>(mainPage->widget(BAND_TABLE_ID));
//	bands->clearContents();
	for (int i = 0; i < ans->bands_size(); i++) {
		Band band = ans->bands(i);

		const std::string &name = band.bandname();
//		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(name));
//		item->setData(dataRole, QVariant::fromValue(band));
//		bands->setItem(i+req.first, 0, item);
		ClickableLabel *lbl = getLabel(BAND_CLICK_ID,
									   QString::fromStdString(name));
		bands->setCellWidget(i+req.first, 0, lbl);

		QString genre = getGenre(band.genre());
		QTableWidgetItem *item = new QTableWidgetItem(genre);
		bands->setItem(i+req.first, 1, item);

		int64_t unix = band.unixfounddate();
		item = new QTableWidgetItem(getFmtDateFromUnix(unix, "yyyy"));
		bands->setItem(i+req.first, 2, item);

		unix = band.unixtermdate();
		item = new QTableWidgetItem(getFmtDateFromUnix(unix, "yyyy"));
		bands->setItem(i+req.first, 3, item);
	}
}

void MainWidget::handleAlbumInsertion(Req &req, TableAns *ans)
{
	QTableWidget *albums = dynamic_cast<QTableWidget*>(mainPage->widget(ALBUM_TABLE_ID));
	QTableWidgetItem *item;
	for (int i = 0; i < ans->albums_size(); i++) {
		Album album = ans->albums(i);

		const std::string &albumname = album.title();
		ClickableLabel *lbl = getLabel(ALBUM_CLICK_ID,
									   QString::fromStdString(albumname));
		lbl->setProperty("album", QVariant::fromValue(album));
		albums->setCellWidget(i+req.first, 0, lbl);
		const std::string &bandname = album.bandname();
		lbl = getLabel(BAND_CLICK_ID, QString::fromStdString(bandname));
		albums->setCellWidget(i+req.first, 1, lbl);

		int64_t unix = album.unixreleasedate();
		QString reldate = getFmtDateFromUnix(unix, "yyyy-MM-dd");
		item = new QTableWidgetItem(reldate);
		albums->setItem(i+req.first, 2, item);

		int nsongs = album.songs_size();
		item = new QTableWidgetItem(QString::number(nsongs));
		albums->setItem(i+req.first, 3, item);
	}
}

void MainWidget::handleSongInsertion(Req &req, TableAns *ans)
{
	QTableWidget *songs = dynamic_cast<QTableWidget*>(mainPage->widget(SONG_TABLE_ID));
	QTableWidgetItem *item;
	for (int i = 0; i < ans->songs_size(); i++) {
		Song song = ans->songs(i);

		const std::string &songname = song.songname();
		ClickableLabel *lbl = getLabel(SONG_CLICK_ID,
									   QString::fromStdString(songname));
		lbl->setProperty("song", QVariant::fromValue(song));
		songs->setCellWidget(i+req.first, 0, lbl);

		const std::string &albumname = song.albumname();
		lbl = getLabel(ALBUM_CLICK_ID, QString::fromStdString(albumname));
		songs->setCellWidget(i+req.first, 1, lbl);

		const std::string &bandname = song.bandname();
		lbl = getLabel(BAND_CLICK_ID, QString::fromStdString(bandname));
		songs->setCellWidget(i+req.first, 2, lbl);

		QTime length = QTime(0, 0, 0, 0).addSecs(song.lengthsec());
		item = new QTableWidgetItem(length.toString("mm:ss"));
		songs->setItem(i+req.first, 3, item);
	}
}

void MainWidget::handleConcertInsertion(Req &req, TableAns *ans)
{
	QTableWidget *concerts = dynamic_cast<QTableWidget*>(mainPage->widget(CONCERT_TABLE_ID));

	for (int i = 0; i < ans->concerts_size(); i++) {
		Concert concert = ans->concerts(i);

		const std::string &descr = concert.description();
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(descr));
		item->setData(dataRole, QVariant::fromValue(concert));
		concerts->setItem(i+req.first, 0, item);

		const std::string &loc = concert.location();
		item = new QTableWidgetItem(QString::fromStdString(loc));
		concerts->setItem(i+req.first, 1, item);

		int64_t unix = concert.unixdatetime();
		QString datetime = getFmtDateFromUnix(unix, "dd-MM-yyyy hh:mm");
		item = new QTableWidgetItem(datetime);
		concerts->setItem(i+req.first, 2, item);
	}
}

void MainWidget::handleBandPageInsertion(SimpleAns *ans)
{
	QList<QGroupBox*> boxes = mainPage->widget(BAND_PAGE_ID)->findChildren<QGroupBox*>();

	if (boxes.length() < 4) {
		qWarning() << "page box length" << boxes.length() << "< 4";
		return;
	}

	Band band = ans->band();

	QGroupBox *box = boxes.at(0);
	QLabel* label = box->findChild<QLabel*>("band");
	if (!label) return;
	label->setText(QString::fromStdString(band.bandname()));
	label->adjustSize();

	label = box->findChild<QLabel*>("description");
	if (!label) return;
	label->setText(QString::fromStdString(band.description()));
	label->adjustSize();

	box = boxes.at(1);
	QString bandName = QString::fromStdString(band.bandname());
	QHBoxLayout *lay = nullptr;
	QVBoxLayout *vlay = dynamic_cast<QVBoxLayout*>(box->layout());

	QLayoutItem *item1, *item2;

	while ( (item1 = vlay->takeAt(0)) != nullptr) {
		QHBoxLayout *lay2 = dynamic_cast<QHBoxLayout*>(item1);
		if (!lay2) break;
		while ( (item2 = lay2->takeAt(0)) != nullptr) {
			delete item2->widget();
			delete item2;
		}
		delete item1->widget();
		delete item1;
	}

	for (int i = 0; i < band.albumnames_size(); i++) {
		if (i % 4 == 0) {
			if (lay) {
				vlay->addLayout(lay);
			}
			lay = new QHBoxLayout;
			lay->setAlignment(Qt::AlignLeft);
		}
		AlbumView *album = new AlbumView(QString::fromStdString(band.albumnames(i)),
										 bandName);
		connect(album, &AlbumView::clicked, this, &MainWidget::clickedReq);
		lay->addWidget(album);
	}

	if (lay) {
		vlay->addLayout(lay);
	}

	box = boxes.at(2);
	QTableWidget *parts = box->findChild<QTableWidget*>();
	if (!parts) return;
	parts->clearContents();
	parts->setRowCount(band.participants_size());
	for (int i = 0; i < band.participants_size(); i++) {
		Membership memb = band.participants(i);
		auto lbl = getLabel(MUSICIAN_CLICK_ID, QString::fromStdString(memb.musname()));
		parts->setCellWidget(i, 0, lbl);

		QTableWidgetItem *item;
		int64_t unix = memb.unixentrydate();
		item = new QTableWidgetItem(getFmtDateFromUnix(unix, "yyyy-MM-dd"));
		parts->setItem(i, 1, item);

		unix = memb.unixquitdate();
		item = new QTableWidgetItem(getFmtDateFromUnix(unix, "yyyy-MM-dd"));
		parts->setItem(i, 2, item);
	}

	box = boxes.at(3);
	QTableWidget *concerts = box->findChild<QTableWidget*>();
	if (!concerts) return;
	concerts->clearContents();
	concerts->setRowCount(band.concerts_size());
	for (int i = 0; i < band.concerts_size(); i++) {
		Concert c = band.concerts(i);
		concerts->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(c.location())));

		QTableWidgetItem *item;
		int64_t unix = c.unixdatetime();
		item = new QTableWidgetItem(getFmtDateFromUnix(unix, "dd-MM-yyyy hh:mm"));
		concerts->setItem(i, 1, item);

		concerts->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(c.description())));
		concerts->setItem(i, 3, new QTableWidgetItem(QString::number(c.capacity())));
	}


//	for (auto box: boxes) {
//		box->adjustSize();
//	}

	QScrollArea *area = dynamic_cast<QScrollArea*>(mainPage->widget(BAND_PAGE_ID));
	area->widget()->adjustSize();
	mainPage->setCurrentIndex(BAND_PAGE_ID);
}

void MainWidget::handleAlbumPageInsertion(SimpleAns *ans)
{
	QWidget *widget = mainPage->widget(ALBUM_PAGE_ID);
	Album album = ans->album();
	ClickableLabel *clbl = widget->findChild<ClickableLabel*>("band");
	if (!clbl) {
		qDebug() << "cant find band lbl";
		return;
	}
	clbl->setText(QString::fromStdString(album.bandname()));

	QLabel *lbl = widget->findChild<QLabel*>("album");
	if (!lbl) {
		qDebug() << "cant find album lbl";
		return;
	}
	lbl->setText(QString::fromStdString(album.title()));

	lbl = widget->findChild<QLabel*>("date");
	if (!lbl) {
		qDebug() << "cant find date lbl";
		return;
	}
	int64_t unix = album.unixreleasedate();
	QString val = getFmtDateFromUnix(unix, "dd-MM-yyyy");
	lbl->setText(val);

	QTableWidget *songs = widget->findChild<QTableWidget*>();
	songs->clearContents();
	songs->setRowCount(album.songs_size());

	for (int i = 0; i < album.songs_size(); i++) {
		Song song = album.songs(i);

		const std::string &songname = song.songname();
//		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(songname));
//		item->setData(dataRole, QVariant::fromValue(song));
//		songs->setItem(i, 0, item);
		ClickableLabel *lbl = getLabel(SONG_CLICK_ID,
									   QString::fromStdString(songname));
		lbl->setProperty("song", QVariant::fromValue(song));
		songs->setCellWidget(i, 0, lbl);

		QTime length = QTime(0, 0, 0, 0).addSecs(song.lengthsec());
		QTableWidgetItem *item = new QTableWidgetItem(length.toString("mm:ss"));
		songs->setItem(i, 1, item);
	}

	mainPage->setCurrentIndex(ALBUM_PAGE_ID);
}

static bool findAndSetLbl(QWidget *w, QString name, QString val)
{
	QLabel *lbl = w->findChild<QLabel*>(name);
	if (!lbl) {
		qWarning() << "no lbl" << name;
		return false;
	}
	lbl->setText(val);
	return true;
}

void MainWidget::handleMusicianPageInsertion(SimpleAns *ans)
{
	QWidget *w = mainPage->widget(MUSICIAN_PAGE_ID);
	Musician mus = ans->musician();
	findAndSetLbl(w, "name", QString::fromStdString(mus.musname()));

	int64_t unix = mus.unixdateofbirth();
	QString val = getFmtDateFromUnix(unix, "dd-MM-yyyy");
	findAndSetLbl(w, "date", val);

	findAndSetLbl(w, "bio", QString::fromStdString(mus.bio()));

	QTableWidget *table = w->findChild<QTableWidget*>();
	if (!table) {
		qWarning() << "cant find memb table";
		return;
	}
	table->clearContents();
	table->setRowCount(mus.memberships_size());
	for (int i = 0; i < mus.memberships_size(); i++) {
		Membership memb = mus.memberships(i);
		auto lbl = getLabel(BAND_CLICK_ID, QString::fromStdString(memb.bandname()));
		table->setCellWidget(i, 0, lbl);

		int64_t unix = memb.unixentrydate();
		QString d = getFmtDateFromUnix(unix, "dd-MM-yyyy");
		QTableWidgetItem *item = new QTableWidgetItem(d);
		table->setItem(i, 1, item);

		unix = memb.unixquitdate();
		d = getFmtDateFromUnix(unix, "dd-MM-yyyy");
		item = new QTableWidgetItem(d);
		table->setItem(i, 2, item);
	}

	mainPage->setCurrentIndex(MUSICIAN_PAGE_ID);
}

void MainWidget::parserConnected()
{
	int id = mainPage->currentIndex();
	if (id >= BAND_PAGE_ID) return;
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
		id = BAND_TABLE_ID;
		handleBandInsertion(req.value(), &ans);
		break;
	}
	case EntityType::ALBUM: {
		id = ALBUM_TABLE_ID;
		handleAlbumInsertion(req.value(), &ans);
		break;
	}
	case EntityType::SONG: {
		id = SONG_TABLE_ID;
		handleSongInsertion(req.value(), &ans);
		break;
	}
	case EntityType::CONCERT: {
		id = CONCERT_TABLE_ID;
		handleConcertInsertion(req.value(), &ans);
		break;
	}
	}

	requests.erase(req);
	if (id == -1) return;

	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
	int last = table->rowCount()-1;
	if (table->cellWidget(last, 0) || table->item(last, 0)) {
		table->setRowCount(last+1+TABLE_GROW);
		scrollTable(id);  //load new rows if visible
	}

}

void MainWidget::simpleAns(uint64_t reqId, SimpleAns ans)
{
	requests.remove(reqId);
	auto type = ans.msg_case();
	qDebug() << "simpleAns" << type;
	switch (type) {
	case SimpleAns::kBand: {
		const Band &band = ans.band();
		qDebug() << QString::fromStdString(band.bandname()) <<
					band.albumnames_size() <<
					band.participants_size() <<
					band.concerts_size();
//		if (mainPage->currentIndex() == BAND_PAGE_ID) {
		handleBandPageInsertion(&ans);
//		}
		break;
	}
	case SimpleAns::kAlbum: {
		handleAlbumPageInsertion(&ans);
		break;
	}
	case SimpleAns::kMusician: {
		handleMusicianPageInsertion(&ans);
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
		if (currSongReqId != reqId) {
			qDebug() << "got old request";
			requests.remove(reqId);
			return; //got old request
		}
		if (data.size() != 0) {
//			qDebug() << data.;
//			qDebug() << data.at(0) << data.at(1);
//			printf("%d %d\n", data.at(0), data.at(1));
			//QByteArray::fromRawData?
			player->writeOpusData(QByteArray(data.data(), data.size()));
		}

		if (ans.isfinal()) {
			requests.remove(reqId);
			currSongReqId = 0;
			qDebug() << "bytes in last pkt" << data.size();
			qDebug() << "end of transfer";
			player->writeOpusData(QByteArray());
		}
		break;
	}
	default: {
		qDebug() << "unsupported stream type" << req.value().type;
	}
	}
}



