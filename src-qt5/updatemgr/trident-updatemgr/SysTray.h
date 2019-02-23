#ifndef _TRI_UP_MGR_TRAY_H
#define _TRI_UP_MGR_TRAY_H

#include <QSystemTrayIcon>
#include <QStringList>

#include "mainUI.h"

class SysTray : public QSystemTrayIcon {
	Q_OBJECT
public:
	SysTray();
	~SysTray();

public slots:
	void newInputs(QStringList list);

private:
	MainUI *window;
	QTimer *checkTimer;

private slots:
	void closeApp();
	void checkForUpdates();
	void activated(QSystemTrayIcon::ActivationReason);
	void launchWindow();
	void closeWindow();
	void updateIcon(bool firstrun = false);

};

#endif
