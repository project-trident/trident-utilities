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

	QJsonObject scan_wifi_networks(QString device);

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
