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
#include <QNetworkConfigurationManager>

class Networking : public QObject {
	Q_OBJECT
public:
	Networking(QObject *parent = 0);
	~Networking();

	QStringList list_devices();
	QJsonObject list_config(QString device);
	QJsonObject current_info(QString device);
	bool set_config(QString device, QJsonObject config);

private:
	QNetworkConfigurationManager *NETMAN;

public slots:

private slots:

signals:

};
#endif
