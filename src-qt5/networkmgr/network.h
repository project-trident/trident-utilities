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
	QJsonObject scan_wifi_networks();
	QJsonArray known_wifi_networks();
	QString active_wifi_network();
	bool save_wifi_network(QJsonObject obj, bool clearonly = false);
	bool remove_wifi_network(QString id);
	bool connect_to_wifi_network(QString id); //ssid or bssid

	//General Purpose functions
	QStringList readFile(QString path);
	bool writeFile(QString path, QStringList contents);

private:
	QNetworkConfigurationManager *NETMAN;
	QString activeNetworkID; //try to use bssid when possible, ssid otherwise

	QString CmdOutput(QString proc, QStringList args);
	int CmdReturn(QString proc, QStringList args);
	//QJsonObject LatestScanResults(QString device);

public slots:
	bool setDeviceState(QString device, State stat);
	//void startWifiScan(QString device);

private slots:
	//void readWifiScanResults();

signals:
	void NewWifiScanResults(QString);
};
#endif
