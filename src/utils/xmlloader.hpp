#ifndef _HPP_UTILS_XMLLOADER
#define _HPP_UTILS_XMLLOADER

#include "defines.hpp"

namespace blunted {

struct XMLTree;

using map_XMLTree = std::multimap<std::string, XMLTree>;

struct XMLTree {
  std::string value;
  map_XMLTree children;
};

class XMLLoader {
public:
  XMLLoader();
  ~XMLLoader();

  XMLTree LoadFile(const std::string& filename);
  XMLTree Load(const std::string& file);
  void Save(XMLTree& source, const std::string& filename);
  std::string GetSource(const XMLTree& source, int depth = 0) const;
  void PrintTree(const XMLTree& source) const;

protected:
  void BuildTree(XMLTree& tree, const std::string& source);
};

}  // namespace blunted

#endif
