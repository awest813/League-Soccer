#ifndef _HPP_SETPIECECONFIG
#define _HPP_SETPIECECONFIG

#include <string>

#include "../gamedefines.hpp"
#include "defines.hpp"

struct SetPieceParams {
  float formation_depth;  // 0.0-1.0, how deep formation sits (default 0.45)
  float formation_width;  // 0.0-1.0, how wide (default 0.95)
};

class SetPieceConfig {
 public:
  static void EnsureTable(int teamDatabaseID);
  static SetPieceParams Load(int teamDatabaseID, e_SetPiece setPiece);
  static void Save(int teamDatabaseID, e_SetPiece setPiece, const SetPieceParams& params);

 private:
  static std::string GetDbPath();
};

#endif
