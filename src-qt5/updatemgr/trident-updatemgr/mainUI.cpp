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
#include <QDesktopServices>

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
  //QTimer::singleShot(10, this, SLOT(checkTrains()) );
  QTimer::singleShot(10, this, SLOT(updateRepoInfo()) );
}

MainUI::~MainUI(){

}

void MainUI::setupConnections(){
  connect(a_group, SIGNAL(triggered(QAction*)), this, SLOT(page_change()) );
  connect(UPMGR, SIGNAL(updateStarting()), this, SLOT(updateStarting()) );
  connect(UPMGR, SIGNAL(updateFinished(bool)), this, SLOT(updateFinished(bool)) );
  connect(UPMGR, SIGNAL(newUpdateMessage(bool, QString)), this, SLOT(updateMessage(bool,QString)) );
  //connect(UPMGR, SIGNAL(trainsStarting()), this, SLOT(trainsLoading()) );
  //connect(UPMGR, SIGNAL(trainsAvailable()), this, SLOT(checkTrains()) );
  connect(UPMGR, SIGNAL(repoInfoAvailable()), this, SLOT(updateRepoInfo()) );
  connect(ui->tree_be, SIGNAL(itemSelectionChanged()), this, SLOT(be_selection_changed()));
  //connect(ui->list_trains, SIGNAL(currentRowChanged(int)), this, SLOT(trainSelChanged()) );
  //connect(ui->tool_trains_rescan, SIGNAL(clicked()), UPMGR, SLOT(startTrainsCheck()));
  connect(ui->list_errata, SIGNAL(currentRowChanged(int)), this, SLOT(updateErrataInfo()) );
}

// ===============
//    PUBLIC FUNCTIONS
// ===============


//==============
//  PRIVATE
//==============
void MainUI::updateLastCheckTime(){
  if(!UPMGR->isRebootRequired()){
    QDateTime chk = UPMGR->lastCheck();
    ui->label_update_checktime->setText( chk.toString() );
    ui->label_update_reboot->setVisible(false);
  }else{
    ui->tool_updates_check->setEnabled(false);
    ui->tool_updates_start->setEnabled(false);
    ui->label_update_reboot->setVisible(true);
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
    ui->label_update_sysver->setText( UPMGR->systemVersion() );
  }else if(act == ui->actionUpdate_Paths){
    //ui->stackedWidget->setCurrentWidget(ui->page_trains);
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

void MainUI::on_tool_repo_infourl_clicked(){
  QString url = ui->tool_repo_infourl->whatsThis();
  QDesktopServices::openUrl( QUrl(url) );
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
void MainUI::updateRepoInfo(){
  QJsonObject repo;// = UPMGR->currentTrainInfo();
  ui->label_repo_version->setText(repo.value("version").toString());
  ui->label_repo_date->setText(repo.value("lastupdate").toString());
  ui->tool_repo_infourl->setVisible(repo.contains("info_url"));
  ui->tool_repo_infourl->setWhatsThis(repo.value("info_url").toString());
  if(repo.contains("errata")){
    ui->group_update_errata->setVisible(true);
    QJsonObject errata = repo.value("errata").toObject();
    QStringList keys = errata.keys();
    ui->list_errata->clear();
    for(int i=0; i<keys.length(); i++){
      QJsonObject err = errata.value(keys[i]).toObject();
      if(err.isEmpty()){ continue; }
      QString name = keys[i];
      if(err.contains("name")){ name = err.value("name").toString(); }
      QListWidgetItem *it = new QListWidgetItem(ui->list_errata);
        it->setText(name);
        it->setData(Qt::UserRole, err);
      ui->list_errata->addItem(it);
    }
  }else{
    ui->group_update_errata->setVisible(false);
  }
  updateErrataInfo();
}

void MainUI::updateErrataInfo(){
  QListWidgetItem *it = ui->list_errata->currentItem();
  ui->text_errata_log->setVisible(it!=0);
  ui->tool_errata_run->setVisible(it!=0);
  if(it==0){ return; } //go no further
  QJsonObject err = ui->list_errata->currentItem()->data(Qt::UserRole).toJsonObject();
  QString text = err.value("info").toString();
  if(err.contains("cmd")){
    text.append("\n\nRun this command from a terminal to fix this issue:\n");
    text.append("<b>sudo "+err.value("cmd").toString()+"</b>");
    ui->tool_errata_run->setVisible(false); //disabled for the moment - button not hooked in yet
  }else{
    ui->tool_errata_run->setVisible(false); // no fix provided
  }
  ui->text_errata_log->setText(text);
  ui->tool_errata_run->setVisible(false); //disable for the moment
}

void MainUI::be_selection_changed(){
  QList<QTreeWidgetItem*> sel = ui->tree_be->selectedItems();
  //qDebug() << "Selection Changed:" << sel.length();
  if(sel.length()>1){
    ui->tool_be_activate->setEnabled(false); //cannot activate multiple
    ui->tool_be_delete->setEnabled(true); //can delete multiple
  }else if(sel.length()==1){
    //only a single selection
    QTreeWidgetItem *cur = sel[0];
    QString stat;
    if(cur==0){ stat="NR"; } //this will disable both buttons
    else{ stat = cur->data(0,1000).toString(); }
    //qDebug() << "Got Stat:" << stat;
    ui->tool_be_activate->setEnabled(!stat.contains("R"));
    ui->tool_be_delete->setEnabled(stat.isEmpty() || stat=="-");
  }else{
    ui->tool_be_activate->setEnabled(false);
    ui->tool_be_delete->setEnabled(false);
  }
}

void MainUI::on_tool_be_activate_clicked(){
  //already checked for single-item selection in the be_selection_changed() function
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
  QList<QTreeWidgetItem*> sel = ui->tree_be->selectedItems();
  QStringList belist;
  for(int i=0; i<sel.length(); i++){
    QString state = sel[i]->data(0,1000).toString();
    if( state.contains("N") || state.contains("R")){ continue; }
    belist << sel[i]->text(0);
  }
  belist.removeDuplicates(); //just in case
  if(belist.isEmpty()){ return; }
  bool ok = BEMGR->delete_be(belist);
  if(!ok){
    QMessageBox::warning(this, tr("Error"), tr("Could not remove boot environment(s)")+"\n\n"+belist.join(", ")+"\n"+BEMGR->lastCmdLog());
  }else{
    refreshBE_list();
  }
}

/*
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
    //if( !obj.value("active").toBool() ){ continue; } //depricated repo - don't show it
    QListWidgetItem *it = new QListWidgetItem(ui->list_trains);
    it->setText(keys[i]);
    it->setWhatsThis(obj.value("description").toString());
    bool current = obj.value("current").toBool();
    if(current){ it->setIcon(QIcon::fromTheme("cs-software-properties") ); }
    if(!obj.value("active").toBool()){ it->setFlags(it->flags() & ~Qt::ItemIsEnabled); }
    ui->list_trains->addItem(it);
  }
  ui->list_trains->setEnabled(true);
  QTimer::singleShot(0, this, SLOT(trainSelChanged()));
}
*/
/*
void MainUI::trainsLoading(){
  //qDebug() << "Got Trains Loading...";
  ui->label_train_description->setText(tr("Loading Repository Information..."));
  ui->list_trains->setEnabled(false);
  ui->tool_change_train->setEnabled(false);
}
*/
/*
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
*/
/*
void MainUI::on_tool_change_train_clicked(){
  QListWidgetItem *it = ui->list_trains->currentItem();
  if(it==0){ return; }
  QString trainname = it->text();
  UPMGR->changeTrain(trainname);
}
*/
