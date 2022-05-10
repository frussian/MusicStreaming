#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	MainWindow(QThread *th, QWidget *parent = nullptr);
    ~MainWindow();
signals:
	void startDecoding();
private slots:
//	void on_searchEdit_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
	MainWidget *mainWidget;
};
#endif // MAINWINDOW_H
