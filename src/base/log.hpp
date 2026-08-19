#ifndef _HPP_LOG
#define _HPP_LOG

#include "defines.hpp"

namespace blunted {

enum e_LogType { e_Notice, e_Warning, e_Error, e_FatalError };

typedef boost::signals2::signal<void(e_LogType, std::string, std::string, std::string)>
    signal_LogCallback;

void LogOpen();
void LogClose();

boost::signals2::connection BindLog(const signal_LogCallback::slot_type& slot);

void Log(e_LogType logType, const std::string& className, const std::string& methodName, const std::string& message);

}  // namespace blunted

#endif
