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
	QJsonObject list_config(QString device);
	QJsonObject current_info(QString device);
	bool set_config(QString device, QJsonObject config);
	State deviceState(QString device);

	// Wifi specific functionality
	QJsonObject scan_wifi_networks(QString device);
	QStringList known_wifi_networks();
	bool save_wifi_network(QJsonObject, bool clearonly = false);
	bool connect_to_wifi_network(QString device, QString id); //ssid or bssid

	//General Purpose functions
	QStringList readFile(QString path);
	bool writeFile(QString path, QStringList contents);

private:
	QNetworkConfigurationManager *NETMAN;

	QString CmdOutput(QString proc, QStringList args);
	int CmdReturn(QString proc, QStringList args);

public slots:
	bool setDeviceState(QString device, State stat);

private slots:

signals:

};
#endif
