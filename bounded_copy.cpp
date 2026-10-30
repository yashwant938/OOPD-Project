#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// The producer traverses directories. Workers copy files and never enqueue or
// wait for other workers, so a full queue cannot create a recursive deadlock.
class CopyPool {
    using Job = std::pair<fs::path, fs::path>;
    std::queue<Job> jobs;
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<std::thread> workers;
    std::exception_ptr failure;
    const std::size_t limit;
    bool closed = false;

    void worker() {
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                changed.wait(lock, [&] { return closed || failure || !jobs.empty(); });
                if (failure || (closed && jobs.empty())) return;
                job = std::move(jobs.front());
                jobs.pop();
                changed.notify_all();
            }
            try {
                fs::copy_file(job.first, job.second, fs::copy_options::none);
            } catch (...) {
                std::lock_guard<std::mutex> lock(mutex);
                if (!failure) failure = std::current_exception();
                changed.notify_all();
                return;
            }
        }
    }

public:
    explicit CopyPool(unsigned count) : limit(count * 2) {
        try {
            for (unsigned i = 0; i < count; ++i) workers.emplace_back(&CopyPool::worker, this);
        } catch (...) {
            close();
            throw;
        }
    }
    ~CopyPool() { close(); }
    void submit(const fs::path& from, const fs::path& to) {
        std::unique_lock<std::mutex> lock(mutex);
        changed.wait(lock, [&] { return failure || jobs.size() < limit; });
        if (failure) std::rethrow_exception(failure);
        jobs.emplace(from, to);
        changed.notify_one();
    }
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            closed = true;
            changed.notify_all();
        }
        for (auto& worker : workers) if (worker.joinable()) worker.join();
    }
    void finish() {
        close();
        if (failure) std::rethrow_exception(failure);
    }
};

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Usage: bounded_copy SOURCE NEW_DESTINATION WORKERS (0 = sequential, 1-32 = pool)");
        const std::string number(argv[3]);
        std::size_t used = 0;
        const int count = std::stoi(number, &used);
        if (used != number.size() || count < 0 || count > 32) throw std::runtime_error("WORKERS must be between 0 and 32");
        const fs::path source = fs::canonical(argv[1]);
        const fs::path requested = fs::absolute(argv[2]);
        const fs::path parent = fs::canonical(requested.parent_path());
        const fs::path destination = parent / requested.filename();
        if (!fs::is_directory(source) || requested.filename().empty() || fs::exists(destination) || fs::is_symlink(fs::symlink_status(destination)))
            throw std::runtime_error("Source must be a directory and destination must be a new path");
        // Never copy into the source tree, even through a symlinked parent.
        auto relative = parent.lexically_relative(source);
        if (relative.empty() || *relative.begin() != "..") throw std::runtime_error("Destination must be outside the source tree");
        if (!fs::create_directory(destination)) throw std::runtime_error("Could not create destination");
        std::unique_ptr<CopyPool> pool;
        if (count) pool = std::make_unique<CopyPool>(static_cast<unsigned>(count));
        std::size_t files = 0;
        for (const auto& entry : fs::recursive_directory_iterator(source)) {
            const auto status = entry.symlink_status();
            if (fs::is_symlink(status)) throw std::runtime_error("Symlinks are not supported");
            const fs::path target = destination / entry.path().lexically_relative(source);
            if (fs::is_directory(status)) {
                fs::create_directory(target);
            } else if (fs::is_regular_file(status)) {
                if (pool) pool->submit(entry.path(), target);
                else fs::copy_file(entry.path(), target, fs::copy_options::none);
                ++files;
            } else {
                throw std::runtime_error("Only regular files and directories are supported");
            }
        }
        if (pool) pool->finish();
        std::cout << "Copied " << files << " files with " << count << " workers\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Copy failed: " << e.what() << '\n';
        return 1;
    }
}
