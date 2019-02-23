#ifndef _BOOT_ENVIRONMENT_MANAGER_H
#define _BOOT_ENVIRONMENT_MANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class beManager : public QObject {
	Q_OBJECT
public:
	beManager(QObject *parent = 0);
	~beManager();

	QJsonObject list_bootenv();
	bool activate_be(QString name);
	bool delete_be(QString name);

	QString lastCmdLog();

private:
	QString cmdOutput(bool &ok, QString cmd, QStringList args);
	QString lastlog;

private slots:

signals:

};
#endif
