#include <network.h>

Networking::Networking(QObject *parent) : QObject(parent){

}

Networking::~Networking(){

}

QStringList Networking::list_devices(){
  QProcess P;
    P.start("ifconfig", QStringList() << "-l" << "ether");
    P.waitForFinished();
  return QString(P.readAll()).section("\n",0,0).split(" ", QString::SkipEmptyParts);
}
