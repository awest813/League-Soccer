#ifndef _HPP_UTILS_CONSOLE
#define _HPP_UTILS_CONSOLE

#include "defines.hpp"
#include "scene/scene2d/scene2d.hpp"
#include "text2d.hpp"

namespace blunted {

class Console {
public:
  Console(std::shared_ptr<Scene2D> scene2D, int numlines = 6);
  ~Console();

  // accepts values from base/log.hpp
  void PrintLog(e_LogType logType, const std::string& className, const std::string& methodName,
                const std::string& message);
  void Print(const std::string& string, const Vector3& color);

protected:
  void UpdateImages();
  int CalculateY(int lineNum);

  std::shared_ptr<Scene2D> scene2D;

  Text2D* text2D;

  std::vector<boost::intrusive_ptr<Image2D>> images;
  std::vector<std::string> lines;
  std::vector<Vector3> colors;

  boost::intrusive_ptr<Image2D> input;

  int lineheight;
  int numlines;

  boost::signals2::connection logConn;
};

}  // namespace blunted

#endif
