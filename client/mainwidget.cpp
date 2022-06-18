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

#define BAND_TABLE_ID 0
#define ALBUM_TABLE_ID 1
#define SONG_TABLE_ID 2
#define CONCERT_TABLE_ID 3
#define BAND_PAGE_ID 4

#define TABLE_GROW 3

int MainWidget::dataRole = Qt::UserRole + 1;

ClickableLabel::ClickableLabel(QString text, QWidget *parent):
	QLabel(text, parent)
{
	setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *evt)
{
	if (evt->flags() & Qt::MouseEventCreatedDoubleClick) {
		emit clicked();
	}
}

AlbumView::AlbumView(QString name, QString band, int nsongs)
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
	if (evt->flags() & Qt::MouseEventCreatedDoubleClick) {
		emit clicked();
	}
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

	mainPage = new QStackedWidget;
	setupTables();
	setupPages();
	for (auto w: mainPage->widget(BAND_PAGE_ID)->findChildren<QGroupBox*>()) {
		qDebug() << "child" << w;
	}
	mainPage->setCurrentIndex(BAND_PAGE_ID);
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
	connect(playSlider, &QSlider::sliderReleased, this, &MainWidget::sliderChanged);
	playSlider->setMinimum(0);
	playSlider->setMaximum(100);
	playSlider->setValue(0);
	playSlider->setStyleSheet(stylesheet);

	QPushButton *playBtn = new QPushButton;
//	playBtn->setFixedSize(70, 70);
//	QRect rect(QPoint(), playBtn->size());
//	rect.adjust(10, 10, -10, -10);
//	QRegion region(rect,QRegion::Ellipse);
	playBtn->setStyleSheet(stylesheet);
//	playBtn->setMask(region);
//	playBtn->setFixedSize(50, 50);

	playBtn->setCheckable(true);
	connect(playBtn, &QPushButton::toggled, this, &MainWidget::playPressed);

	QVBoxLayout *songInfoLay = new QVBoxLayout;
	songNameBtn = new QPushButton("");
	songNameBtn->setProperty("isFlat", true);
	songNameBtn->setStyleSheet("QPushButton { border: none; }");
	bandNameBtn = new QPushButton("");
	bandNameBtn->setFlat(true);
	songInfoLay->addWidget(songNameBtn);
	songInfoLay->addWidget(bandNameBtn);
	songInfoLay->addStretch();

	currTimeBtn = new QPushButton;
	currTimeBtn->setProperty("isFlat", true);
	currTimeBtn->setStyleSheet("QPushButton { border: none; }");

	endTimeBtn = new QPushButton;
	endTimeBtn->setProperty("isFlat", true);
	endTimeBtn->setStyleSheet("QPushButton { border: none; }");

	QLabel endLbl("end");
//	bandNameBtn->setStyleSheet("QPushButton { border: none; }");

	playLay->addWidget(playBtn, 0, 2);
	playLay->setAlignment(playBtn, Qt::AlignCenter);
	playLay->addWidget(playSlider, 1, 2);
	playLay->addWidget(currTimeBtn, 1, 1);
	playLay->addWidget(endTimeBtn, 1, 3);
	playLay->addLayout(songInfoLay, 0, 0, 2, 1);

	playGroup->setLayout(playLay);
}

void MainWidget::playPressed(bool checked)
{
	if (!checked) {
		qDebug() << "start";
		emit startPlayer(false);
	} else {
		qDebug() << "stop";
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

void MainWidget::sliderChanged()
{
	qDebug() << playSlider->value();
	QString timeStr = endTimeBtn->text();
	QTime time = QTime::fromString(timeStr, "mm:ss");
	int endSecs = time.minute() * 60 + time.second();

	int val = playSlider->value();
	int secs = val / 100.f * endSecs;
	emit seekPlayer(secs);
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
		connect(w, &QTableWidget::cellDoubleClicked, this, &MainWidget::tableClicked);
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

void MainWidget::clicked()
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	if (mainPage->currentIndex() == id) return;

	this->mainPage->setCurrentIndex(id);
	QString filter = searchEdit->text();
	scrollTable(id);
//	requestTable(0, 5, filter, EntityType(id));
}

void MainWidget::tableClicked(int row, int col)
{
	QObject *obj = sender();
	int id = obj->property("index").toInt();
	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
	auto item = table->item(row, col);

	switch (id) {
	case BAND_TABLE_ID: {
		if (col != 0) return;
		QTableWidgetItem *item = table->item(row, 0);
		if (!item) return;
//		QVariant var = item->data(dataRole);
//		Band b = var.value<Band>();
//		mainPage->setCurrentIndex(BAND_PAGE_ID);
		QString band = item->data(Qt::DisplayRole).toString();
		simpleRequest(band, EntityType::BAND);
		break;
	}
	case SONG_TABLE_ID: {
		if (col != 0) return;
		QTableWidgetItem *item = table->item(row, 0);
		if (!item) return;
		QVariant var = item->data(Qt::DisplayRole);
		QString songName = var.toString();
		Song song = item->data(dataRole).value<Song>();
		currSongSecs = song.lengthsec();
		item = table->item(row, 2);
		QString band = item->data(Qt::DisplayRole).toString();

		player->reset();  //in different thread
		emit startPlayer(true);
#if RANGE_REQUESTS_FOR_SONGS == 0
		streamRequest(songName, 8000, EntityType::SONG);
#endif
		songNameBtn->setText(songName);
		bandNameBtn->setText(band);
		currTimeBtn->setText("0:00");
		QTime length = QTime(0, 0, 0, 0).addSecs(song.lengthsec());
		endTimeBtn->setText(length.toString("mm:ss"));

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
	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
	int h = table->viewport()->height();
	int first = -1;
	int last = 0;
	for (int i = 0; i < table->rowCount(); i++) {
		int pos = table->rowViewportPosition(i);
		if (0 <= pos && pos <= h && !table->item(i, 0)) {
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
	int id = mainPage->currentIndex();
	if (id >= BAND_PAGE_ID) return;
	QTableWidget *table = dynamic_cast<QTableWidget*>(mainPage->widget(id));
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
	QTableWidget *bands = dynamic_cast<QTableWidget*>(mainPage->widget(BAND_TABLE_ID));
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
	QTableWidget *albums = dynamic_cast<QTableWidget*>(mainPage->widget(ALBUM_TABLE_ID));
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
	QTableWidget *songs = dynamic_cast<QTableWidget*>(mainPage->widget(SONG_TABLE_ID));
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
	QTableWidget *concerts = dynamic_cast<QTableWidget*>(mainPage->widget(CONCERT_TABLE_ID));
//	songs->clearContents();
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
		if (unix) {
			QDateTime t = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(t.toString("dd-MM-yyyy hh:mm"));
			concerts->setItem(i+req.first, 2, item);
		}
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
	qDebug() << box->title();
	QString bandName = QString::fromStdString(band.bandname());
	QHBoxLayout *lay = nullptr;
	QVBoxLayout *vlay = dynamic_cast<QVBoxLayout*>(box->layout());
	qDebug() << vlay << vlay->children().size();

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
		parts->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(memb.musname())));

		QTableWidgetItem *item;
		int64_t unix = memb.unixentrydate();
		if (unix > 0) {
			QDateTime entry = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(entry.toString("yyyy-MM-dd"));
			parts->setItem(i, 1, item);
		}

		unix = memb.unixquitdate();
		if (unix > 0) {
			QDateTime quit = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(quit.toString("yyyy-MM-dd"));
			parts->setItem(i, 2, item);
		}
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
		if (unix) {
			QDateTime t = QDateTime::fromSecsSinceEpoch(unix, Qt::UTC);
			item = new QTableWidgetItem(t.toString("dd-MM-yyyy hh:mm"));
			concerts->setItem(i, 1, item);
		}
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
//		if (mainPage->currentIndex() == BAND_PAGE_ID) {
			handleBandPageInsertion(&ans);
//		}
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
			qDebug() << "end of transfer";
		}
		break;
	}
	default: {
		qDebug() << "unsupported stream type" << req.value().type;
	}
	}
}



