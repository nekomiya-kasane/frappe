#include "frappe/file_lock.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

class FileLockTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_file_lock_test";
        std::filesystem::create_directories(_test_dir);

        _test_file = _test_dir / "test_lock_file.txt";
        std::ofstream(_test_file) << "test content for locking";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    std::filesystem::path _test_dir;
    std::filesystem::path _test_file;
};

// ============================================================================
// lock_type_name Tests
// ============================================================================

TEST_F(FileLockTest, LockTypeName) {
    EXPECT_EQ(frappe::lock_type_name(frappe::lock_type::shared), "shared");
    EXPECT_EQ(frappe::lock_type_name(frappe::lock_type::exclusive), "exclusive");
}

// ============================================================================
// file_lock Basic Tests
// ============================================================================

TEST_F(FileLockTest, ExclusiveLock) {
    frappe::file_lock lock(_test_file);
    auto result = lock.lock();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(lock.file_path(), _test_file);
    EXPECT_EQ(lock.type(), frappe::lock_type::exclusive);

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST_F(FileLockTest, SharedLock) {
    frappe::file_lock lock(_test_file, frappe::lock_options{.type = frappe::lock_type::shared});
    auto result = lock.lock();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(lock.owns_lock());
    EXPECT_EQ(lock.type(), frappe::lock_type::shared);

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST_F(FileLockTest, TryLock) {
    frappe::file_lock lock(_test_file);
    auto result = lock.try_lock();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
}

TEST_F(FileLockTest, TryLockFor) {
    frappe::file_lock lock(_test_file);
    auto result = lock.try_lock_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
}

TEST_F(FileLockTest, MoveConstruct) {
    frappe::file_lock lock1(_test_file);
    lock1.lock();
    EXPECT_TRUE(lock1.owns_lock());

    frappe::file_lock lock2(std::move(lock1));
    EXPECT_TRUE(lock2.owns_lock());

    lock2.unlock();
}

// ============================================================================
// scoped_file_lock Tests
// ============================================================================

TEST_F(FileLockTest, ScopedLock) {
    {
        frappe::scoped_file_lock lock(_test_file);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_TRUE(static_cast<bool>(lock));
    }
    // Lock should be released here

    // Should be able to lock again
    frappe::scoped_file_lock lock2(_test_file);
    EXPECT_TRUE(lock2.owns_lock());
}

TEST_F(FileLockTest, ScopedSharedLock) {
    frappe::scoped_file_lock lock(_test_file, frappe::lock_type::shared);
    EXPECT_TRUE(lock.owns_lock());
}

// ============================================================================
// Convenience Function Tests
// ============================================================================

TEST_F(FileLockTest, LockFileFunction) {
    auto result = frappe::lock_file(_test_file);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->owns_lock());
}

TEST_F(FileLockTest, TryLockFileFunction) {
    auto result = frappe::try_lock_file(_test_file);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(*result);
}

TEST_F(FileLockTest, IsFileLocked) {
    auto result = frappe::is_file_locked(_test_file);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(*result); // File should not be locked
}

// ============================================================================
// Concurrent Lock Tests
// ============================================================================

TEST_F(FileLockTest, ConcurrentExclusiveLock) {
    frappe::file_lock lock1(_test_file);
    lock1.lock();

    // Try to lock from another "thread" (simulated with try_lock)
    frappe::file_lock lock2(_test_file, frappe::lock_options{.mode = frappe::lock_mode::non_blocking});
    auto result = lock2.try_lock();

    // Should fail because lock1 holds exclusive lock
    EXPECT_FALSE(result.has_value());

    lock1.unlock();

    // Now should succeed
    result = lock2.try_lock();
    EXPECT_TRUE(result.has_value());

    lock2.unlock();
}

TEST_F(FileLockTest, MultipleSharedLocks) {
    frappe::file_lock lock1(_test_file, frappe::lock_options{.type = frappe::lock_type::shared});
    lock1.lock();

    frappe::file_lock lock2(_test_file, frappe::lock_options{.type = frappe::lock_type::shared});
    auto result = lock2.lock();

    // Multiple shared locks should succeed
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(lock1.owns_lock());
    EXPECT_TRUE(lock2.owns_lock());

    lock1.unlock();
    lock2.unlock();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(FileLockTest, LockNonExistentFile) {
    frappe::file_lock lock(_test_dir / "nonexistent.txt");
    auto result = lock.lock();
    EXPECT_FALSE(result.has_value());
}
