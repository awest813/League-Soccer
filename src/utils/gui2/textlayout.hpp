#ifndef GF_GUI2_TEXTLAYOUT_HPP
#define GF_GUI2_TEXTLAYOUT_HPP
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace blunted {
inline std::vector<std::string> WrapMenuText(const std::string& text, unsigned int maxChars) {
  std::vector<std::string> lines;
  std::istringstream paragraphs(text);
  std::string paragraph;
  const size_t limit = std::max(1u, maxChars);
  while (std::getline(paragraphs, paragraph)) {
    std::istringstream words(paragraph);
    std::string line, word;
    while (words >> word) {
      if (!line.empty() && line.size() + 1 + word.size() > limit) {
        lines.push_back(line);
        line.clear();
      }
      if (!line.empty())
        line += " ";
      line += word; // Keep long words and UTF-8 sequences intact; captions fit their width.
    }
    lines.push_back(line);
  }
  if (text.empty() || text.back() == '\n')
    lines.push_back("");
  return lines;
}
}  // namespace blunted
#endif
