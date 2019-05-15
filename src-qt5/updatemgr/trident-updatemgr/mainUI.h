//===========================
//  Project Trident source code
//  Copyright (c) 2018, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================
#ifndef _TRIDENT_MAIN_WINDOW_H
#define _TRIDENT_MAIN_WINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QProcess>
#include <QShortcut>
#include <QMessageBox>
#include <QActionGroup>
#include <QAction>

#include "beMgr.h"

namespace Ui{
	class MainUI;
};

class MainUI : public QMainWindow{
	Q_OBJECT
public:
	MainUI();
	~MainUI();

	void setupConnections();

public slots:

private:
	Ui::MainUI *ui;
	QActionGroup *a_group;

	beManager *BEMGR;

	//Internal simplification routines
	void updateLastCheckTime();
	void enableButtons(bool running);
	void refreshBE_list();

private slots:
	void page_change();
	// Updates Page
	void on_tool_updates_check_clicked();
	void on_tool_updates_start_clicked();
	void on_tool_repo_infourl_clicked();
	void updateMessage(bool check, QString msg);
	void updateStarting();
	void updateFinished(bool success);
	void updateRepoInfo();
	void updateErrataInfo();
	// Boot Environments
	void be_selection_changed();
	void on_tool_be_activate_clicked();
	void on_tool_be_delete_clicked();
	// TRAINS
	void checkTrains();
	void trainsLoading();
	void trainSelChanged();
	void on_tool_change_train_clicked();

signals:

};

#endif
