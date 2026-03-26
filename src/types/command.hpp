// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#ifndef _HPP_COMMAND
#define _HPP_COMMAND

#include "defines.hpp"

#include "types/refcounted.hpp"
#include "types/lockable.hpp"

namespace blunted {

  class Command : public RefCounted {

    public:
      Command(const std::string &name);
      virtual ~Command();

      bool IsReady();
      void Reset();
      bool Handle(void *caller = nullptr);
      void Wait();

      std::string GetName() const { return name.GetData(); }

    protected:
      virtual bool Execute(void *caller = nullptr) = 0;

      std::mutex mutex; // locks 'handled & processed'
      bool handled;
      std::condition_variable processed;

      Lockable<std::string> name;

  };
}

#endif
