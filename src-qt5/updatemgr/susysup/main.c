#include <unistd.h>

// Simple wrapper utility to allow the update GUI to run with user permissions
// but the backend updater to change to root before running as needed
int main(int argc, char ** argv){
  if(getuid()!=0){
    if(0 != setuid(0)){ return 1; } //could not drop to root permissions
  }
  if(argc<1){ return 1; }
  return execvp("/usr/local/bin/sysup", argv);
}
