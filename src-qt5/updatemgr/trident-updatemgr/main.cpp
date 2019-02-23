#include <QTranslator>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <QDateTime>

#include "updateMgr.h"
#include "../../common/SingleApplication.h"
#include "SysTray.h"

UpdateManager *UPMGR = new UpdateManager();

int main(int argc, char ** argv)
{
    //Regular performance improvements
    qputenv("QT_NO_GLIB","1");
    //Create/start the application
    SingleApplication a(argc, argv, "trident-updatemgr");
    if(a.isPrimaryProcess()){
      a.setQuitOnLastWindowClosed(false);
      SysTray st;
      SingleApplication::connect( &a, SIGNAL(InputsAvailable(QStringList)), &st, SLOT(newInputs(QStringList)) );
      st.show();
      st.newInputs(a.inputlist);
      return a.exec();
    }else{
      return 0;
    }
}
