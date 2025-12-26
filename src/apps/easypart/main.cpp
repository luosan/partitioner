#include <shell/tclmanager.h>
#include <string>

int main(int argc, char *argv[]) {

  Shell::TclManager::init(argc, argv);
  Shell::TclManager::instance()->run(); 
  return 0;
}
