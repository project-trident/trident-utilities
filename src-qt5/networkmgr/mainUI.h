// =========================
//  Project Trident Network Manager
//  Available under the 3-clause BSD License
//  Initially written in February of 2019
//   by Ken Moore <ken@project-trident.org>
// =========================
#ifndef _NET_MGR_MAIN_UI_H
#define _NET_MGR_MAIN_UI_H

#include <QObject>
#include <QMainWindow>
#include <QAction>
#include <QActionGroup>
//#include <QWidgetGroup>

#include "network.h"

namespace Ui{
	class mainUI;
};

class mainUI : public QMainWindow{
	Q_OBJECT
public:
	mainUI();
	~mainUI();

public slots:
	void newInputs(QStringList args);

private:
	Ui::mainUI *ui;
	Networking *NETWORK;
	QActionGroup *page_group;

	//Initial page loading (on page change)
	void updateConnections();
	void updateFirewall();
	void updateVPN();
	void updateDNS();

private slots:
	void pageChange(QAction *triggered);
	void updateConnectionInfo();
	void on_radio_conn_dev_dhcp_toggled(bool);
	void on_group_conn_dev_static_clicked(bool);

};

#endif
