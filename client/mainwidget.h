#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

class QGroupBox;
class QStackedWidget;
class NetworkParser;

class MainWidget : public QWidget
{
	Q_OBJECT
public:
	explicit MainWidget(QThread *th, QWidget *parent = nullptr);

signals:

private:
	void initUI();
	void setupLeftPanel();
	void setupSearch();
	void setupPlayArea(QString stylesheet);
	void setupTables();
	QGroupBox *leftGroup;
	QGroupBox *searchGroup;
	QGroupBox *playGroup;
	QStackedWidget *tables;
	NetworkParser *parser;
};

#endif // MAINWIDGET_H
