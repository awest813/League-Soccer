#include "utils/userdata.hpp"

#include <chrono>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;
using blunted::PrepareUserDataDirectory;

namespace {

class UserDataTest : public ::testing::Test {
protected:
  void SetUp() override {
    root = fs::temp_directory_path() /
           ("gf_userdata_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    resources = root / "App.app" / "Contents" / "Resources";
    user = root / "Application Support" / "Gameplay Football";
    fs::create_directories(resources / "media" / "fonts");
    fs::create_directories(resources / "data" / "locale");
    Write(resources / "football.config", "\"locale_language\" \"en\"\n");
    Write(resources / "media" / "fonts" / "font.ttf", "font");
  }
  void TearDown() override {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
  static void Write(const fs::path& path, const std::string& text) { std::ofstream(path) << text; }
  static std::string Read(const fs::path& path) {
    std::ifstream stream(path);
    return std::string(std::istreambuf_iterator<char>(stream), {});
  }

  fs::path root, resources, user;
};

TEST_F(UserDataTest, LinksDirectoriesAndCopiesFiles) {
  std::string error;
  ASSERT_TRUE(PrepareUserDataDirectory(resources, user, &error)) << error;
  EXPECT_TRUE(fs::is_symlink(user / "media"));
  EXPECT_TRUE(fs::is_symlink(user / "data"));
  EXPECT_EQ(Read(user / "media" / "fonts" / "font.ttf"), "font");
  EXPECT_FALSE(fs::is_symlink(user / "football.config"));
  EXPECT_EQ(Read(user / "football.config"), "\"locale_language\" \"en\"\n");
}

TEST_F(UserDataTest, KeepsTheUsersConfigAndSaves) {
  ASSERT_TRUE(PrepareUserDataDirectory(resources, user));
  Write(user / "football.config", "\"locale_language\" \"de\"\n");
  fs::create_directories(user / "saves" / "league");
  Write(user / "saves" / "league" / "slot_1.sqlite", "save");

  ASSERT_TRUE(PrepareUserDataDirectory(resources, user));
  EXPECT_EQ(Read(user / "football.config"), "\"locale_language\" \"de\"\n");
  EXPECT_EQ(Read(user / "saves" / "league" / "slot_1.sqlite"), "save");
}

TEST_F(UserDataTest, RelinksWhenTheBundleMoves) {
  ASSERT_TRUE(PrepareUserDataDirectory(resources, user));
  // App Translocation runs the bundle from a new path on each launch.
  const fs::path moved = root / "Translocated" / "Resources";
  fs::create_directories(moved.parent_path());
  fs::rename(resources, moved);
  Write(moved / "media" / "fonts" / "font.ttf", "moved");

  std::string error;
  ASSERT_TRUE(PrepareUserDataDirectory(moved, user, &error)) << error;
  EXPECT_EQ(fs::read_symlink(user / "media"), moved / "media");
  EXPECT_EQ(Read(user / "media" / "fonts" / "font.ttf"), "moved");
}

TEST_F(UserDataTest, LeavesARealDirectoryAlone) {
  fs::create_directories(user / "media");
  Write(user / "media" / "custom.png", "mine");
  ASSERT_TRUE(PrepareUserDataDirectory(resources, user));
  EXPECT_FALSE(fs::is_symlink(user / "media"));
  EXPECT_EQ(Read(user / "media" / "custom.png"), "mine");
}

TEST_F(UserDataTest, ReportsAMissingResourceDirectory) {
  std::string error;
  EXPECT_FALSE(PrepareUserDataDirectory(root / "missing", user, &error));
  EXPECT_NE(error.find("cannot read"), std::string::npos);
}

}  // namespace
