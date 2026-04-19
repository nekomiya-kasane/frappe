#include <gtest/gtest.h>
#include "frappe/watcher.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>

class WatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_watcher_test";
        std::filesystem::create_directories(_test_dir);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    std::filesystem::path _test_dir;
};

// ============================================================================
// watch_event_type_name Tests
// ============================================================================

TEST_F(WatcherTest, EventTypeName) {
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::added), "added");
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::modified), "modified");
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::removed), "removed");
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::renamed), "renamed");
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::attributes), "attributes");
    EXPECT_EQ(frappe::watch_event_type_name(frappe::watch_event_type::error), "error");
}

// ============================================================================
// watch_options Tests
// ============================================================================

TEST_F(WatcherTest, DefaultOptions) {
    frappe::watch_options opts;
    EXPECT_TRUE(opts.recursive);
    EXPECT_FALSE(opts.follow_symlinks);
    EXPECT_FALSE(opts.ignore_initial);
    EXPECT_FALSE(opts.use_polling);
    EXPECT_EQ(opts.poll_interval.count(), 100);
    EXPECT_EQ(opts.debounce.count(), 50);
}

// ============================================================================
// file_watcher Basic Tests
// ============================================================================

TEST_F(WatcherTest, CreateWatcher) {
    frappe::file_watcher watcher;
    EXPECT_FALSE(watcher.is_running());
}

TEST_F(WatcherTest, AddPath) {
    frappe::file_watcher watcher;
    auto result = watcher.add(_test_dir);
    EXPECT_TRUE(result.has_value());

    auto paths = watcher.watched_paths();
    EXPECT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], _test_dir);
}

TEST_F(WatcherTest, AddMultiplePaths) {
    auto subdir1 = _test_dir / "subdir1";
    auto subdir2 = _test_dir / "subdir2";
    std::filesystem::create_directories(subdir1);
    std::filesystem::create_directories(subdir2);

    frappe::file_watcher watcher;
    std::vector<frappe::path> paths = { subdir1, subdir2 };
    auto result = watcher.add(paths);
    EXPECT_TRUE(result.has_value());

    auto watched = watcher.watched_paths();
    EXPECT_EQ(watched.size(), 2u);
}

TEST_F(WatcherTest, RemovePath) {
    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);

    auto result = watcher.remove(_test_dir);
    EXPECT_TRUE(result.has_value());

    auto paths = watcher.watched_paths();
    EXPECT_TRUE(paths.empty());
}

TEST_F(WatcherTest, RemoveAll) {
    auto subdir1 = _test_dir / "subdir1";
    auto subdir2 = _test_dir / "subdir2";
    std::filesystem::create_directories(subdir1);
    std::filesystem::create_directories(subdir2);

    frappe::file_watcher watcher;
    (void)watcher.add(subdir1);
    (void)watcher.add(subdir2);

    watcher.remove_all();

    auto paths = watcher.watched_paths();
    EXPECT_TRUE(paths.empty());
}

TEST_F(WatcherTest, StartStop) {
    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);

    auto result = watcher.start();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(watcher.is_running());

    watcher.stop();
    EXPECT_FALSE(watcher.is_running());
}

TEST_F(WatcherTest, StartWithoutPaths) {
    frappe::file_watcher watcher;
    auto result = watcher.start();
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Event Detection Tests
// ============================================================================

TEST_F(WatcherTest, DetectFileCreation) {
    std::atomic<bool> event_received{ false };
    frappe::watch_event received_event;

    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);
    watcher.on_add([&](const frappe::watch_event& e) {
        received_event = e;
        event_received = true;
        });

    (void)watcher.start();

    // Create a file
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto new_file = _test_dir / "new_file.txt";
    std::ofstream(new_file) << "content";

    // Wait for event
    for (int i = 0; i < 20 && !event_received; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    watcher.stop();

    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_event.type, frappe::watch_event_type::added);
}

TEST_F(WatcherTest, DetectFileModification) {
    // Create file first
    auto test_file = _test_dir / "modify_test.txt";
    std::ofstream(test_file) << "initial content";

    std::atomic<bool> event_received{ false };
    frappe::watch_event received_event;

    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);
    watcher.on_change([&](const frappe::watch_event& e) {
        received_event = e;
        event_received = true;
        });

    (void)watcher.start();

    // Modify the file
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::ofstream(test_file) << "modified content";

    // Wait for event
    for (int i = 0; i < 20 && !event_received; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    watcher.stop();

    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_event.type, frappe::watch_event_type::modified);
}

TEST_F(WatcherTest, DetectFileDeletion) {
    // Create file first
    auto test_file = _test_dir / "delete_test.txt";
    std::ofstream(test_file) << "content to delete";

    std::atomic<bool> event_received{ false };
    frappe::watch_event received_event;

    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);
    watcher.on_remove([&](const frappe::watch_event& e) {
        received_event = e;
        event_received = true;
        });

    (void)watcher.start();

    // Delete the file
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::filesystem::remove(test_file);

    // Wait for event
    for (int i = 0; i < 20 && !event_received; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    watcher.stop();

    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_event.type, frappe::watch_event_type::removed);
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST_F(WatcherTest, FilterCallback) {
    std::atomic<int> event_count{ 0 };

    frappe::file_watcher watcher;
    (void)watcher.add(_test_dir);
    watcher.set_filter([](const frappe::path& p) {
        return p.extension() == ".txt";
        });
    watcher.on_event([&](const frappe::watch_event&) {
        ++event_count;
        });

    (void)watcher.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create .txt file (should trigger event)
    std::ofstream(_test_dir / "test.txt") << "content";
    // Create .log file (should be filtered)
    std::ofstream(_test_dir / "test.log") << "content";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    watcher.stop();

    // Only .txt file should trigger event
    EXPECT_GE(event_count, 1);
}

// ============================================================================
// Convenience Function Tests
// ============================================================================

TEST_F(WatcherTest, IsWatchSupported) {
    EXPECT_TRUE(frappe::is_watch_supported(_test_dir));
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_F(WatcherTest, MoveConstruct) {
    frappe::file_watcher watcher1;
    watcher1.add(_test_dir);

    frappe::file_watcher watcher2(std::move(watcher1));

    auto paths = watcher2.watched_paths();
    EXPECT_EQ(paths.size(), 1u);
}

TEST_F(WatcherTest, MoveAssign) {
    frappe::file_watcher watcher1;
    watcher1.add(_test_dir);

    frappe::file_watcher watcher2;
    watcher2 = std::move(watcher1);

    auto paths = watcher2.watched_paths();
    EXPECT_EQ(paths.size(), 1u);
}
