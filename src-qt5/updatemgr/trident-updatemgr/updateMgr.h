#ifndef _UPDATE_MANAGER_H
#define _UPDATE_MANAGER_H

#include <QProcess>
#include <QString>
#include <QFile>
#include <QObject>
#include <QDateTime>
#include <QTextStream>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class UpdateManager : public QObject {
	Q_OBJECT
private:
	QProcess PROC, TRPROC;
	QString logcontents, traincontents;
	QFile *logfile;
	bool processIsCheck, trainIsList;
	QDateTime lastcheck;
	QString system_version;
	QNetworkReply *repoCheck;
	QJsonObject repo_info;

	void clear_logfile();

public:
	UpdateManager(QObject *parent = 0);
	~UpdateManager();

	static QString readLocalFile(QString path);

	QString updatelog();
	bool lastRunWasCheck();
	bool isUpdateRunning();
	bool isRebootRequired();
	QDateTime lastCheck(){ return lastcheck; }
	bool updatesAvailable();
	QJsonObject current_repo_info(){ return repo_info; }

	bool startUpdates(bool fullupdates=false);
	bool startUpdateCheck();

	//Trains
	QJsonObject listTrains();
	bool changeTrain(QString trainname);

	QString systemVersion(){ return system_version.section("::",0,0); }
	QJsonObject currentTrainInfo();

private slots:
	void startUpdates(bool checkonly, bool fullupdate);
	void fetchRepoInfo();
	void saveRepoInfo();
	void processMessage();
	void processFinished(int retcode);
	void trainsProcFinished(int retcode);
	void injectIntoLog(QString msg);

public slots:
	void startTrainsCheck();

signals:
	//INTERNAL signals
	void startupdates(bool, bool); //INTERNAL SIGNAL for thread-safe launching
	//EXTERNAL signals
	void newUpdateMessage(bool ischeck, QString);
	void updateStarting();
	void updateFinished(bool success);
	void trainsStarting();
	void trainsAvailable();
	void repoInfoAvailable();
};

#endif
