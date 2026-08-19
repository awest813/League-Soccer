#include "log.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>

namespace blunted {

signal_LogCallback callback;

std::ofstream logFile;
std::mutex mutex;

void LogOpen() {
  logFile.open("log.txt", std::ios::out);
}

void LogClose() {
  if (logFile.is_open())
    logFile.close();
}

boost::signals2::connection BindLog(const signal_LogCallback::slot_type& slot) {
  return callback.connect(slot);
}

void Log(e_LogType logType, const std::string& className, const std::string& methodName, const std::string& message) {
  std::string logTypeString;

  switch (logType) {
    case e_Notice:
      logTypeString = "Notice";
      break;
    case e_Warning:
      logTypeString = "Warning";
      break;
    case e_Error:
      logTypeString = "ERROR";
      break;
    case e_FatalError:
      logTypeString = "FATAL ERROR !!! N00000 !!!";
      break;
  }

  char bla[2048];
  std::snprintf(bla, sizeof(bla), "[%s] in [%s::%s]: %s\n", logTypeString.c_str(),
                className.c_str(), methodName.c_str(), message.c_str());
  callback(logType, className.c_str(), methodName.c_str(), message.c_str());

  {
    std::lock_guard<std::mutex> lock(mutex);
    printf("%s", bla);
    if (logFile.is_open()) {
      logFile << bla;
      logFile.flush();
    }
  }

  if (logType == e_FatalError) {
    LogClose();

#ifndef NDEBUG
    // for gdb backtracing
    std::abort();
#endif

    exit(1);
  }
}

}  // namespace blunted
