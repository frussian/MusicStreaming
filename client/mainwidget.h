#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMap>
#include <QMetaType>

class QGroupBox;
class QStackedWidget;
class NetworkParser;
class QLineEdit;
class AudioPlayer;

#include "messages.pb.h"

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	explicit MainWidget(QThread *th, QWidget *parent = nullptr);

signals:
	void connectToHost(QString host, int port);
	void requestTableParser(uint64_t reqId, int first, int last, QString filter,
						   enum EntityType type);
private slots:
	void clicked();
	void tableClicked(int row, int column);
	void tableScrolled(int value);
	void searchChanged(QString filter);
	void tableAns(uint64_t reqId, TableAns ans);
	void parserConnected();
	void reqFailed(uint64_t reqId);
//	void simpleAns(uint64_t reqId, SimpleAns ans);
//	void streamAns(uint64_t reqId, StreamAns ans);
private:
	struct Req {
		int first;
		int last;
	};

	void initUI();
	void setupLeftPanel();
	void setupSearch();
//	void setupPlayArea(QString stylesheet);
	void setupTables();
	void scrollTable(int id);
	void requestTable(int first, int last, QString filter,
					 enum EntityType type);
	void handleBandInsertion(Req&, TableAns*);
	void handleAlbumInsertion(Req&, TableAns*);
	void handleSongInsertion(Req&, TableAns*);
	void handleConcertInsertion(Req&, TableAns*);

	static int dataRole;

	QGroupBox *leftGroup;
	QGroupBox *searchGroup;
	QLineEdit *searchEdit;
//	QGroupBox *playGroup;
	QStackedWidget *tables;
	uint64_t reqId = 1;
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
