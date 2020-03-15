// =========================
//  Project Trident Network Manager
//  Available under the 3-clause BSD License
//  Initially written in February of 2019
//   by Ken Moore <ken@project-trident.org>
// =========================
#ifndef _NETWORK_BACKEND_H
#define _NETWORK_BACKEND_H

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkConfigurationManager>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

class Networking : public QObject {
	Q_OBJECT
public:
	enum State{ StateUnknown=0, StateRunning, StateStopped, StateRestart };
	Networking(QObject *parent = 0);
	~Networking();

	QStringList list_devices();
	QJsonObject list_config();
	QJsonObject current_info(QString device);
	bool set_config(QJsonObject config);
	State deviceState(QString device);

	// Wifi specific functionality
	QJsonObject wifi_scan_results();
	QJsonArray known_wifi_networks();
	QJsonObject active_wifi_network();
	bool is_known(QJsonObject obj);

	bool save_wifi_network(QJsonObject obj, bool clearonly = false);
	bool remove_wifi_network(QString id);
	bool connect_to_wifi_network(QString id, bool noretry = false); //known network ID number

	// DNS specific functionality
	QString current_dns();
	QJsonObject custom_dns_settings();
	bool save_custom_dns_settings(QJsonObject);

	// Wireguard specific functionality
	QJsonObject current_wireguard_profiles();
	bool add_wireguard_profile(QString path);
	bool remove_wireguard_profile(QString name);
	bool start_wireguard_profile(QString name);
	bool stop_wireguard_profile(QString name);

	//General Purpose functions
	static QStringList readFile(QString path);
	static bool writeFile(QString path, QStringList contents);
	static bool sameNetwork(QJsonObject A, QJsonObject B);

private:
	QNetworkConfigurationManager *NETMAN;
	QJsonObject last_wifi_scan;

	QString CmdOutput(QString proc, QStringList args);
	int CmdReturnCode(QString proc, QStringList args);
	bool CmdReturn(QString proc, QStringList args);

	void performWifiScan(); //designed to be run in a separate thread
	void parseWifiScanResults(QStringList info);

public slots:
	bool setDeviceState(QString device, State stat);
	void startWifiScan();

private slots:

signals:
	void starting_wifi_scan();
	void new_wifi_scan_results();
	void finished_wifi_scan();
};
#endif
