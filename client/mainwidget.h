#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMap>
#include <QMetaType>
#include <QLabel>
#include <QFrame>
#include <QTimer>

class QGroupBox;
class QStackedWidget;
class NetworkParser;
class QLineEdit;
class AudioPlayer;
class QPushButton;
class QSlider;

#include "messages.pb.h"

class ClickableLabel: public QLabel
{
	Q_OBJECT
public:
	ClickableLabel(int typeId, QString text = "", QWidget *parent = nullptr);
protected:
	void mousePressEvent(QMouseEvent *evt);
signals:
	void clicked(int typeId, QString text);
private:
	int typeId;
};

class AlbumView: public QFrame
{
	Q_OBJECT
public:
	AlbumView(QString title, QString band, int nsongs = 0);
	~AlbumView();
protected:
	void mousePressEvent(QMouseEvent *evt);
signals:
	void clicked(int typeId, QString text);
private:
	QString title;
};

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	explicit MainWidget(QThread *th, QWidget *parent = nullptr);

signals:
	void connectToHost(QString host, int port);
	void requestTableParser(uint64_t reqId, int first, int last, QString filter,
						   enum EntityType type);
	void simpleRequestParser(uint64_t reqId, QString name, enum EntityType type);
	void streamRequestParser(uint64_t reqId, QString name, uint32_t size, enum EntityType type);
	void cancelStreamRequestParser(uint64_t reqId);

	void startPlayer(bool clear);
	void stopPlayer(bool clear);
	void seekPlayer(int secs);
private slots:
	ClickableLabel *getLabel(int typeId, QString text);
	void processSongClick(Song &song);
//	void albumSongsTableClicked(int row, int col);
	void clickedReq(int typeId, QString text);
	void clickedLeftPanel();
//	void tableClicked(int row, int column);
	void tableScrolled(int value);
	void searchChanged(QString filter);
	void searchChangedTimer();
	void tableAns(uint64_t reqId, TableAns ans);
	void simpleAns(uint64_t reqId, SimpleAns ans);
	void streamAns(uint64_t reqId, StreamAns ans);
	void parserConnected();
	void reqFailed(uint64_t reqId);
	void playPressed(bool checked);
	void processedUSecs(quint64);
	void playSliderChanged();
//	void simpleAns(uint64_t reqId, SimpleAns ans);
//	void streamAns(uint64_t reqId, StreamAns ans);
private:
	struct Req {
		int first;
		int last;
		QString name;
		enum EntityType type;
	};

	void initUI();
	void setupLeftPanel();
	void setupSearch();
	void setupPlayArea(QString stylesheet);
	void setupTables();
	void setupPages();
	void setupBandPage();
	void setupAlbumPage();
	void setupMusicianPage();
	void scrollTable(int id);

	void requestTable(int first, int last, QString filter,
					 enum EntityType type);
	void simpleRequest(QString name, enum EntityType type);
	void streamRequest(QString name, uint32_t size, enum EntityType type);

	void handleBandInsertion(Req&, TableAns*);
	void handleAlbumInsertion(Req&, TableAns*);
	void handleSongInsertion(Req&, TableAns*);
	void handleConcertInsertion(Req&, TableAns*);

	void handleBandPageInsertion(SimpleAns*);
	void handleAlbumPageInsertion(SimpleAns*);
	void handleMusicianPageInsertion(SimpleAns*);

	static int dataRole;

	QGroupBox *leftGroup;
	QGroupBox *searchGroup;
	QGroupBox *playGroup;
	QLineEdit *searchEdit;
	QTimer searchTimer;

	QLabel *songNameLbl;
	ClickableLabel *bandNameLbl;
	QPushButton *currTimeBtn;
	QPushButton *endTimeBtn;
	QSlider *playSlider;
	QPushButton *playBtn;
	QSlider *volSlider;
//	QGroupBox *playGroup;

	QStackedWidget *mainPage;
	uint64_t reqId = 1;

	uint64_t currSongReqId = 0;
	int currSongSecs = 0;

	QMap<uint64_t, Req> requests;
	NetworkParser *parser;
	AudioPlayer *player;
};

Q_DECLARE_METATYPE(uint64_t);
Q_DECLARE_METATYPE(Song);
Q_DECLARE_METATYPE(Album);
Q_DECLARE_METATYPE(Band);
Q_DECLARE_METATYPE(Concert);

#endif // MAINWIDGET_H
