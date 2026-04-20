#include "frappe/traversal.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>

namespace frappe {

// ============================================================================
// cycle_detector implementation
// ============================================================================

cycle_detector::cycle_detector(cycle_detection strategy) : _strategy(strategy) {}

bool cycle_detector::check_and_enter(const path &dir, const file_id &id) {
    _cycle_target.clear();

    switch (_strategy) {
    case cycle_detection::none:
        return true;

    case cycle_detection::inode_set: {
        if (!_visited_inodes.insert(id.inode).second) {
            _cycle_target = dir;
            return false;
        }
        return true;
    }

    case cycle_detection::path_set: {
        std::error_code ec;
        auto canonical = std::filesystem::canonical(dir, ec);
        if (ec) {
            return true; // Can't canonicalize, assume safe
        }
        if (!_visited_paths.insert(canonical.string()).second) {
            _cycle_target = dir;
            return false;
        }
        return true;
    }

    case cycle_detection::depth_limit:
        // Depth limit is handled externally
        return true;

    case cycle_detection::hybrid: {
        // Check ancestor stack first (fast, low memory)
        for (const auto &ancestor : _ancestor_stack) {
            if (ancestor.inode == id.inode) {
                _cycle_target = ancestor.dir_path;
                return false;
            }
        }

        // Add to ancestor stack
        _ancestor_stack.push_back({dir, id.inode});

        // For deep traversals, also check visited set
        if (_ancestor_stack.size() > 100) {
            if (!_visited_inodes.insert(id.inode).second) {
                _cycle_target = dir;
                _ancestor_stack.pop_back();
                return false;
            }
        }
        return true;
    }
    }

    return true;
}

void cycle_detector::leave(const path &dir) {
    (void)dir;
    if (_strategy == cycle_detection::hybrid && !_ancestor_stack.empty()) {
        _ancestor_stack.pop_back();
    }
}

void cycle_detector::clear() {
    _visited_inodes.clear();
    _visited_paths.clear();
    _ancestor_stack.clear();
    _cycle_target.clear();
}

const path &cycle_detector::cycle_target() const noexcept {
    return _cycle_target;
}

// ============================================================================
// directory_traverser::impl
// ============================================================================

struct directory_traverser::impl {
    path root;
    traversal_options opts;
    traversal_stats stats;
    cycle_detector detector;
    std::atomic<bool> cancelled{false};
    std::chrono::steady_clock::time_point start_time;

    // Directory stack for DFS traversal
    struct dir_state {
        path dir_path;
        std::filesystem::directory_iterator iter;
        std::filesystem::directory_iterator end;
        int depth;
    };
    std::stack<dir_state> dir_stack;

    // Current entry
    std::optional<file_entry> current_entry;

    // Root device (for cross_device check)
    std::uint64_t root_device = 0;

    impl(const path &r, const traversal_options &o) : root(r), opts(o), detector(o.cycle_strategy) {
        start_time = std::chrono::steady_clock::now();

        // Get root device
        if (!opts.cross_device) {
            auto id_result = get_file_id(root);
            if (id_result) {
                root_device = id_result->device;
            }
        }
    }

    bool should_enter_directory(const path &p, const file_entry &entry, int depth) {
        // Check depth limit
        if (opts.max_depth >= 0 && depth >= opts.max_depth) {
            ++stats.max_depth_reached;
            return false;
        }

        // Check cross-device
        if (!opts.cross_device && root_device != 0) {
            auto id_result = get_file_id(p);
            if (id_result && id_result->device != root_device) {
                return false;
            }
        }

        // Check for cycles (only for symlinks when following)
        if (opts.follow_symlinks && entry.is_symlink) {
            auto id_result = get_file_id(p);
            if (id_result) {
                if (!detector.check_and_enter(p, *id_result)) {
                    ++stats.cycles_detected;

                    switch (opts.cycle_action) {
                    case on_cycle::skip:
                        return false;
                    case on_cycle::error:
                        if (opts.error_callback) {
                            opts.error_callback(p, std::make_error_code(std::errc::too_many_symbolic_link_levels));
                        }
                        return false;
                    case on_cycle::callback:
                        if (opts.cycle_callback) {
                            opts.cycle_callback(p, detector.cycle_target());
                        }
                        return false;
                    }
                }
            }
        } else if (entry.type == file_type::directory) {
            // For regular directories, also check cycles
            auto id_result = get_file_id(p);
            if (id_result && !detector.check_and_enter(p, *id_result)) {
                ++stats.cycles_detected;
                return false;
            }
        }

        return true;
    }

    void report_error(const path &p, std::error_code ec) {
        ++stats.errors;
        if (opts.error_callback) {
            opts.error_callback(p, ec);
        }
    }

    bool advance() {
        if (cancelled.load(std::memory_order_relaxed)) {
            return false;
        }

        // Check max entries
        if (opts.max_entries > 0) {
            auto total = stats.files_visited + stats.dirs_visited + stats.symlinks_visited;
            if (total >= opts.max_entries) {
                return false;
            }
        }

        while (!dir_stack.empty()) {
            auto &state = dir_stack.top();

            while (state.iter != state.end) {
                std::error_code ec;
                auto entry = *state.iter; // copy — ++iter invalidates the reference
                ++state.iter;

                // Get file entry
                auto file_entry_result = get_file_entry(entry);

                // Filter hidden files
                if (!opts.include_hidden && file_entry_result.is_hidden) {
                    continue;
                }

                // Update stats
                if (file_entry_result.is_symlink) {
                    ++stats.symlinks_visited;
                } else if (file_entry_result.type == file_type::directory) {
                    ++stats.dirs_visited;
                } else {
                    ++stats.files_visited;
                }

                // Check if we should recurse into directory
                bool is_dir = file_entry_result.type == file_type::directory;
                bool should_recurse = is_dir;

                if (opts.follow_symlinks && file_entry_result.is_symlink) {
                    auto target_status = std::filesystem::status(entry.path(), ec);
                    if (!ec && std::filesystem::is_directory(target_status)) {
                        should_recurse = true;
                    }
                }

                if (should_recurse && should_enter_directory(entry.path(), file_entry_result, state.depth + 1)) {
                    // Push new directory onto stack
                    auto dir_opts = std::filesystem::directory_options::none;
                    if (opts.skip_permission_denied) {
                        dir_opts |= std::filesystem::directory_options::skip_permission_denied;
                    }

                    std::filesystem::directory_iterator new_iter(entry.path(), dir_opts, ec);
                    if (ec) {
                        if (ec == std::errc::permission_denied) {
                            ++stats.permission_denied;
                        }
                        if (!opts.skip_permission_denied || ec != std::errc::permission_denied) {
                            report_error(entry.path(), ec);
                        }
                    } else {
                        dir_stack.push({entry.path(), std::move(new_iter), {}, state.depth + 1});
                    }
                }

                current_entry = std::move(file_entry_result);
                return true;
            }

            // Done with this directory
            detector.leave(state.dir_path);
            dir_stack.pop();
        }

        // Update elapsed time
        stats.elapsed = std::chrono::steady_clock::now() - start_time;
        return false;
    }

    void initialize() {
        std::error_code ec;
        auto dir_opts = std::filesystem::directory_options::none;
        if (opts.skip_permission_denied) {
            dir_opts |= std::filesystem::directory_options::skip_permission_denied;
        }

        std::filesystem::directory_iterator iter(root, dir_opts, ec);
        if (ec) {
            if (ec == std::errc::permission_denied) {
                ++stats.permission_denied;
            }
            report_error(root, ec);
            return;
        }

        // Check root for cycles
        auto id_result = get_file_id(root);
        if (id_result) {
            detector.check_and_enter(root, *id_result);
        }

        dir_stack.push({root, std::move(iter), {}, 0});
    }
};

// ============================================================================
// directory_traverser implementation
// ============================================================================

directory_traverser::directory_traverser(const path &root, const traversal_options &opts)
    : _impl(std::make_unique<impl>(root, opts)) {
    _impl->initialize();
}

directory_traverser::~directory_traverser() = default;

directory_traverser::directory_traverser(directory_traverser &&) noexcept = default;
directory_traverser &directory_traverser::operator=(directory_traverser &&) noexcept = default;

const traversal_stats &directory_traverser::stats() const noexcept {
    return _impl->stats;
}

void directory_traverser::cancel() noexcept {
    _impl->cancelled.store(true, std::memory_order_relaxed);
}

bool directory_traverser::is_cancelled() const noexcept {
    return _impl->cancelled.load(std::memory_order_relaxed);
}

#ifdef FRAPPE_HAS_GENERATOR
std::generator<file_entry> directory_traverser::entries() {
    while (_impl->advance()) {
        if (_impl->current_entry) {
            co_yield std::move(*_impl->current_entry);
        }
    }
}
#endif

// ============================================================================
// directory_traverser::iterator implementation
// ============================================================================

struct directory_traverser::iterator::impl {
    directory_traverser::impl *traverser = nullptr;
    bool at_end = true;

    void advance() {
        if (traverser && !at_end) {
            at_end = !traverser->advance();
        }
    }
};

directory_traverser::iterator::iterator(std::shared_ptr<impl> impl) : _impl(std::move(impl)) {}

const file_entry &directory_traverser::iterator::operator*() const {
    return *_impl->traverser->current_entry;
}

const file_entry *directory_traverser::iterator::operator->() const {
    return &*_impl->traverser->current_entry;
}

directory_traverser::iterator &directory_traverser::iterator::operator++() {
    _impl->advance();
    return *this;
}

directory_traverser::iterator directory_traverser::iterator::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

bool directory_traverser::iterator::operator==(const iterator &other) const {
    if (!_impl && !other._impl) return true;
    if (!_impl || !other._impl) {
        // One is end iterator
        bool this_at_end = !_impl || _impl->at_end;
        bool other_at_end = !other._impl || other._impl->at_end;
        return this_at_end == other_at_end;
    }
    return _impl->at_end == other._impl->at_end;
}

directory_traverser::iterator directory_traverser::begin() {
    auto iter_impl = std::make_shared<iterator::impl>();
    iter_impl->traverser = _impl.get();
    iter_impl->at_end = !_impl->advance();
    return iterator(std::move(iter_impl));
}

directory_traverser::iterator directory_traverser::end() {
    return iterator{};
}

// ============================================================================
// Convenience functions
// ============================================================================

#ifdef FRAPPE_HAS_GENERATOR
std::generator<file_entry> traverse(const path &root, const traversal_options &opts) {
    directory_traverser traverser(root, opts);
    for (auto &&entry : traverser.entries()) {
        co_yield std::move(entry);
    }
}
#endif

std::vector<file_entry> traverse_collect(const path &root, const traversal_options &opts) {
    std::vector<file_entry> result;
    directory_traverser traverser(root, opts);
    for (const auto &entry : traverser) {
        result.push_back(entry);
    }
    return result;
}

void traverse_with(const path &root, const std::function<bool(const file_entry &)> &callback,
                   const traversal_options &opts) {
    directory_traverser traverser(root, opts);
    for (const auto &entry : traverser) {
        if (!callback(entry)) {
            traverser.cancel();
            break;
        }
    }
}

std::vector<file_entry> traverse_parallel(const path &root, const traversal_options &opts, unsigned int threads) {
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;
    }

    std::vector<file_entry> result;
    std::mutex result_mutex;

    // Queue of directories to process
    std::queue<path> dir_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<int> active_workers{0};
    std::atomic<bool> done{false};

    // Initialize with root
    dir_queue.push(root);

    // Worker function
    auto worker = [&]() {
        cycle_detector detector(opts.cycle_strategy);

        while (true) {
            path current_dir;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_cv.wait(lock, [&]() { return !dir_queue.empty() || done.load(); });

                if (done.load() && dir_queue.empty()) {
                    return;
                }

                if (dir_queue.empty()) {
                    continue;
                }

                current_dir = std::move(dir_queue.front());
                dir_queue.pop();
                ++active_workers;
            }

            // Process directory
            std::error_code ec;
            auto dir_opts = std::filesystem::directory_options::none;
            if (opts.skip_permission_denied) {
                dir_opts |= std::filesystem::directory_options::skip_permission_denied;
            }

            std::vector<file_entry> local_entries;
            std::vector<path> subdirs;

            for (const auto &entry : std::filesystem::directory_iterator(current_dir, dir_opts, ec)) {
                auto file_entry_result = get_file_entry(entry);

                if (!opts.include_hidden && file_entry_result.is_hidden) {
                    continue;
                }

                local_entries.push_back(file_entry_result);

                // Check if directory
                bool is_dir = file_entry_result.type == file_type::directory;
                if (opts.follow_symlinks && file_entry_result.is_symlink) {
                    auto target_status = std::filesystem::status(entry.path(), ec);
                    if (!ec && std::filesystem::is_directory(target_status)) {
                        is_dir = true;
                    }
                }

                if (is_dir) {
                    // Check for cycles
                    auto id_result = get_file_id(entry.path());
                    if (id_result && detector.check_and_enter(entry.path(), *id_result)) {
                        subdirs.push_back(entry.path());
                    }
                }
            }

            // Add entries to result
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                result.insert(result.end(), std::make_move_iterator(local_entries.begin()),
                              std::make_move_iterator(local_entries.end()));
            }

            // Add subdirectories to queue
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                for (auto &subdir : subdirs) {
                    dir_queue.push(std::move(subdir));
                }
                --active_workers;

                // Check if we're done
                if (dir_queue.empty() && active_workers.load() == 0) {
                    done.store(true);
                }
            }
            queue_cv.notify_all();
        }
    };

    // Start workers
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (unsigned int i = 0; i < threads; ++i) {
        workers.emplace_back(worker);
    }

    // Wait for completion
    for (auto &w : workers) {
        w.join();
    }

    return result;
}

// ============================================================================
// safe_recursive_iterator implementation
// ============================================================================

struct safe_recursive_iterator::impl {
    std::filesystem::recursive_directory_iterator iter;
    std::filesystem::recursive_directory_iterator end;
    traversal_options opts;
    cycle_detector detector;
    std::filesystem::directory_entry current;
    bool at_end = false;

    impl(const path &root, const traversal_options &o) : opts(o), detector(o.cycle_strategy) {
        std::error_code ec;
        auto dir_opts = std::filesystem::directory_options::none;
        if (opts.skip_permission_denied) {
            dir_opts |= std::filesystem::directory_options::skip_permission_denied;
        }
        if (opts.follow_symlinks) {
            dir_opts |= std::filesystem::directory_options::follow_directory_symlink;
        }

        iter = std::filesystem::recursive_directory_iterator(root, dir_opts, ec);
        if (ec) {
            at_end = true;
            return;
        }

        advance_to_valid();
    }

    void advance_to_valid() {
        while (iter != end) {
            std::error_code ec;
            const auto &entry = *iter;

            // Check hidden
            if (!opts.include_hidden) {
                auto name = entry.path().filename().string();
                if (!name.empty() && name[0] == '.') {
                    iter.increment(ec);
                    continue;
                }
            }

            // Check depth
            if (opts.max_depth >= 0 && iter.depth() > opts.max_depth) {
                iter.pop();
                continue;
            }

            // Check for cycles on directories
            if (entry.is_directory(ec) && !ec) {
                auto id_result = get_file_id(entry.path());
                if (id_result && !detector.check_and_enter(entry.path(), *id_result)) {
                    // Cycle detected, skip this directory
                    iter.disable_recursion_pending();
                }
            }

            current = entry;
            return;
        }

        at_end = true;
    }

    void advance() {
        if (at_end) return;

        std::error_code ec;
        iter.increment(ec);
        if (ec) {
            at_end = true;
            return;
        }

        advance_to_valid();
    }
};

safe_recursive_iterator::safe_recursive_iterator(const path &root, const traversal_options &opts)
    : _impl(std::make_shared<impl>(root, opts)) {}

const std::filesystem::directory_entry &safe_recursive_iterator::operator*() const {
    return _impl->current;
}

const std::filesystem::directory_entry *safe_recursive_iterator::operator->() const {
    return &_impl->current;
}

safe_recursive_iterator &safe_recursive_iterator::operator++() {
    _impl->advance();
    return *this;
}

safe_recursive_iterator safe_recursive_iterator::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

bool safe_recursive_iterator::operator==(const safe_recursive_iterator &other) const {
    if (!_impl && !other._impl) return true;
    if (!_impl || !other._impl) {
        bool this_at_end = !_impl || _impl->at_end;
        bool other_at_end = !other._impl || other._impl->at_end;
        return this_at_end == other_at_end;
    }
    return _impl->at_end == other._impl->at_end;
}

int safe_recursive_iterator::depth() const noexcept {
    return _impl ? _impl->iter.depth() : 0;
}

void safe_recursive_iterator::pop() {
    if (_impl && !_impl->at_end) {
        _impl->iter.pop();
        _impl->advance_to_valid();
    }
}

void safe_recursive_iterator::disable_recursion_pending() {
    if (_impl && !_impl->at_end) {
        _impl->iter.disable_recursion_pending();
    }
}

// ============================================================================
// safe_recursive_directory_range implementation
// ============================================================================

safe_recursive_directory_range::safe_recursive_directory_range(const path &root, const traversal_options &opts)
    : _root(root), _opts(opts) {}

safe_recursive_iterator safe_recursive_directory_range::begin() const {
    return safe_recursive_iterator(_root, _opts);
}

safe_recursive_iterator safe_recursive_directory_range::end() const {
    return safe_recursive_iterator{};
}

} // namespace frappe
