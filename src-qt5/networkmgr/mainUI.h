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
#include <QTimer>
#include <QTreeWidgetItem>

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
	//Internal simplifications
	QTreeWidgetItem* generateWifi_item(QJsonObject obj, QJsonObject active);

private slots:
	void pageChange(QAction *triggered);
	void updateConnectionInfo();
	void updateWifiConnections();

	void on_tree_wifi_networks_currentItemChanged(QTreeWidgetItem *it);
	void on_tool_dev_restart_clicked();
	void on_tool_dev_start_clicked();
	void on_tool_dev_stop_clicked();
	void on_tool_forget_wifi_clicked();
	void on_tool_connect_wifi_clicked();

	void starting_wifi_scan();
	void finished_wifi_scan();

	void loadStaticProfiles();
	void on_tool_conn_profile_apply_clicked();
	void on_tool_conn_new_profile_clicked();
	void on_tool_conn_remove_profile_clicked();
	void on_combo_conn_static_profile_currentIndexChanged(int);
	void on_line_static_v4_address_textEdited(const QString &text);
	void on_line_static_v4_gateway_textEdited(const QString &text);
};

#endif
