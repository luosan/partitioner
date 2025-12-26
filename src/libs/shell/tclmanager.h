#ifndef TCLMANAGER_H
#define TCLMANAGER_H

#include <tcl.h>
#include <mutex>

class Tcl_Interp;

namespace Shell {

class TclManager {
public:
  ~TclManager();
  void run();
  void shutdown();

  // 单例实例获取方法 - 懒汉式，线程安全
  static TclManager *instance();
  
  // 初始化方法，只能调用一次
  static void init(int argc, char **argv);
  
  static int tclIterpInit(Tcl_Interp *interp);
  static Tcl_Interp *tclInterp();

  static int executeCmd(ClientData, Tcl_Interp *interp, int argc,
                        const char *argv[]);

private:
  // 私有构造函数，防止外部直接创建实例
  TclManager();
  
  // 删除拷贝构造函数和赋值操作符，确保单例性
  TclManager(const TclManager&) = delete;
  TclManager& operator=(const TclManager&) = delete;

  void printHelp(char **argv);
  void lec_banner();

  // 静态成员变量
  static TclManager* m_instance;
  static std::mutex m_mutex;
  static bool m_initialized;
  
  // 存储初始化参数
  static int s_argc;
  static char** s_argv;
};
} // namespace Shell
#endif // TCLMANAGER_H
