//===========================================
//  Project Trident source code
//  Copyright (c) 2018, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "mainUI.h"
#include "ui_mainUI.h"
#include <QDebug>
#include <QLocale>
#include <QScrollBar>
#include <QScreen>

#include "updateMgr.h"
#include <unistd.h>

extern UpdateManager *UPMGR;

MainUI::MainUI() : QMainWindow(), ui(new Ui::MainUI){
  ui->setupUi(this); //load the designer file
  a_group = new QActionGroup(this);
  a_group->addAction(ui->actionUpdates);
  a_group->addAction(ui->actionUpdate_Paths);
  a_group->addAction(ui->actionRollback);
  a_group->setExclusive(true);
  BEMGR = new beManager(this);

  setupConnections();
  ui->actionUpdates->setChecked(true);
  enableButtons( UPMGR->isUpdateRunning() );
  ui->text_updates->setPlainText(UPMGR->updatelog());
  page_change();
  QTimer::singleShot(10, this, SLOT(checkTrains()) );
}

MainUI::~MainUI(){

}

void MainUI::setupConnections(){
  connect(a_group, SIGNAL(triggered(QAction*)), this, SLOT(page_change()) );
  connect(UPMGR, SIGNAL(updateStarting()), this, SLOT(updateStarting()) );
  connect(UPMGR, SIGNAL(updateFinished(bool)), this, SLOT(updateFinished(bool)) );
  connect(UPMGR, SIGNAL(newUpdateMessage(bool, QString)), this, SLOT(updateMessage(bool,QString)) );
  connect(UPMGR, SIGNAL(trainsStarting()), this, SLOT(trainsLoading()) );
  connect(UPMGR, SIGNAL(trainsAvailable()), this, SLOT(checkTrains()) );
  connect(ui->tree_be, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(be_selection_changed()));
  connect(ui->list_trains, SIGNAL(currentRowChanged(int)), this, SLOT(trainSelChanged()) );
}

// ===============
//    PUBLIC FUNCTIONS
// ===============


//==============
//  PRIVATE
//==============
void MainUI::updateLastCheckTime(){
  if(!UPMGR->isRebootRequired()){
    QString txt = tr("Last Check: %1");
    QDateTime chk = UPMGR->lastCheck();
    ui->label_update_checktime->setText( txt.arg( chk.toString() ) );
    ui->label_update_checktime->setVisible( chk.isValid() );
  }else{
    ui->tool_updates_check->setEnabled(false);
    ui->tool_updates_start->setEnabled(false);
    ui->label_update_checktime->setText("System reboot required to finish updates!");
  }
}

void MainUI::enableButtons(bool running){
  // quick simplification for enabling all the buttons that need to be toggled
  // whenever an update process is running/stopped
  ui->actionUpdate_Paths->setEnabled(!running);
  ui->tool_updates_check->setEnabled(!running);
  ui->tool_updates_start->setEnabled(!running && UPMGR->updatesAvailable());
  ui->progressBar->setVisible(running);
  ui->label_update_checktime->setVisible(!running);
  if(!running){ updateLastCheckTime(); }
}

void MainUI::refreshBE_list(){
  QJsonObject info = BEMGR->list_bootenv();
  ui->tree_be->clear();
  QStringList keys = info.keys();
  for(int i=0; i<keys.length(); i++){
    QJsonObject data = info.value(keys[i]).toObject();
    QString status = data.value("status").toString();
    QStringList text; text << data.value("name").toString() << data.value("size").toString() << data.value("date_created").toString();
    QTreeWidgetItem *it = new QTreeWidgetItem(text);
    it->setData(0, 1000, status);
    if(status.contains("N")){ it->setIcon(0, QIcon::fromTheme("cs-software-properties")); }
    else if(status.contains("R")){ it->setIcon(0, QIcon::fromTheme("system-reboot")); }
    ui->tree_be->addTopLevelItem(it);
  }
  ui->tree_be->sortItems(2, Qt::DescendingOrder);
  ui->tree_be->resizeColumnToContents(0);
}

//==============
//  PRIVATE SLOTS
//==============
void MainUI::page_change(){
  QAction *act = a_group->checkedAction();
  if(act == ui->actionUpdates){
    ui->stackedWidget->setCurrentWidget(ui->page_updates);
  }else if(act == ui->actionUpdate_Paths){
    ui->stackedWidget->setCurrentWidget(ui->page_trains);
  }else if(act == ui->actionRollback){
    ui->stackedWidget->setCurrentWidget(ui->page_rollback);
    refreshBE_list();
  }
}

void MainUI::on_tool_updates_check_clicked(){
  UPMGR->startUpdateCheck();
}

void MainUI::on_tool_updates_start_clicked(){
  //Ask if we should do a full update or not
  QMessageBox::StandardButton ans = QMessageBox::question(this, tr("Prepare Updates"), \
		tr("Would you like to force a full update?\n\nThis takes longer and downloads more packages, but ensures that everything is reinstalled with the latest version(s) from the package repository"),\
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
  if(ans == QMessageBox::Cancel){ return; }
  UPMGR->startUpdates(ans == QMessageBox::Yes);
}

void MainUI::updateMessage(bool, QString msg){
  ui->text_updates->append(msg);
}

void MainUI::updateStarting(){
  ui->text_updates->clear();
  enableButtons(true);
}

void MainUI::updateFinished(bool){
  enableButtons(false);
}

void MainUI::be_selection_changed(){
  QTreeWidgetItem *cur = ui->tree_be->currentItem();
  QString stat;
  if(cur==0){ stat="NR"; } //this will disable both buttons
  else{ stat = cur->data(0,1000).toString(); }
  //qDebug() << "Got Stat:" << stat;
  ui->tool_be_activate->setEnabled(!stat.contains("R"));
  ui->tool_be_delete->setEnabled(stat.isEmpty() || stat=="-");
}

void MainUI::on_tool_be_activate_clicked(){
  QTreeWidgetItem *cur = ui->tree_be->currentItem();
  if(cur==0){ return; }
  QString name = cur->text(0);
  bool ok = BEMGR->activate_be(name);
  if(!ok){
    QMessageBox::warning(this, tr("Error"), tr("Could not activate boot environment")+"\n\n"+name+"\n"+BEMGR->lastCmdLog());
  }else{
    refreshBE_list();
  }
}

void MainUI::on_tool_be_delete_clicked(){
  QTreeWidgetItem *cur = ui->tree_be->currentItem();
  if(cur==0){ return; }
  QString name = cur->text(0);
  bool ok = BEMGR->delete_be(name);
  if(!ok){
    QMessageBox::warning(this, tr("Error"), tr("Could not remove boot environment")+"\n\n"+name+"\n"+BEMGR->lastCmdLog());
  }else{
    refreshBE_list();
  }
}

void MainUI::checkTrains(){
  QJsonObject trains = UPMGR->listTrains();
  if(trains.keys().isEmpty()){
    trainsLoading();
    return;
  }
  //qDebug() << "Got Trains JSON:" << trains;
  ui->list_trains->clear();
  QStringList keys = trains.keys();
  for(int i=0; i<keys.length(); i++){
    QJsonObject obj = trains.value(keys[i]).toObject();
    QListWidgetItem *it = new QListWidgetItem(ui->list_trains);
    it->setText(keys[i]);
    it->setWhatsThis(obj.value("description").toString());
    bool current = obj.value("current").toBool();
    if(current){ it->setIcon(QIcon::fromTheme("cs-software-properties") ); }
    ui->list_trains->addItem(it);
  }
  ui->list_trains->setEnabled(true);
  QTimer::singleShot(0, this, SLOT(trainSelChanged()));
}

void MainUI::trainsLoading(){
  //qDebug() << "Got Trains Loading...";
  ui->label_train_description->setText(tr("Loading Repository Information..."));
  ui->list_trains->setEnabled(false);
  ui->tool_change_train->setEnabled(false);
}

void MainUI::trainSelChanged(){
  QListWidgetItem *it = ui->list_trains->currentItem();
  if(it==0){
    ui->tool_change_train->setEnabled(false);
    ui->label_train_description->clear();
    return;
  }
  bool current = it->data(Qt::UserRole).toBool();
  QString description = it->whatsThis();
  ui->tool_change_train->setEnabled(!current);
  ui->label_train_description->setText(description);
}

void MainUI::on_tool_change_train_clicked(){
  QListWidgetItem *it = ui->list_trains->currentItem();
  if(it==0){ return; }
  QString trainname = it->text();
  UPMGR->changeTrain(trainname);
}
