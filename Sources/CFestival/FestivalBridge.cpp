#include "CFestival.h"

#include "festival_3.1__src/global.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <mach-o/dyld.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

int festival_main(int argc, char **argv);
extern char **environ;

extern int forced_alg;
extern int level_sol_pushes;
extern int level_sol_moves;
extern int global_total_pushes;
extern int global_total_moves;
extern int total_sol_time;
extern int total_levels_tried;
extern int total_levels_solved;

namespace {

std::mutex festival_mutex;
constexpr const char *kChildMarkerEnv = "CFESTIVAL_CHILD";
constexpr const char *kLevelPathEnv = "CFESTIVAL_LEVEL_PATH";
constexpr const char *kOutputPathEnv = "CFESTIVAL_OUTPUT_PATH";
constexpr const char *kTimeLimitEnv = "CFESTIVAL_TIME_LIMIT";
constexpr const char *kCoresEnv = "CFESTIVAL_CORES";

char *duplicate_string(const std::string &value)
{
    auto *result = static_cast<char *>(malloc(value.size() + 1));
    if (result == nullptr) {
        return nullptr;
    }

    memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

std::string trim(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

bool is_valid_lurd(const std::string &lurd)
{
    if (lurd.empty()) {
        return false;
    }

    for (char ch : lurd) {
        switch (ch) {
        case 'u':
        case 'U':
        case 'r':
        case 'R':
        case 'd':
        case 'D':
        case 'l':
        case 'L':
            break;
        default:
            return false;
        }
    }
    return true;
}

bool extract_lurd(const std::string &solution_file_text, std::string &lurd)
{
    std::vector<std::string> lines;
    std::string current;

    for (char ch : solution_file_text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    lines.push_back(current);

    for (size_t i = 0; i < lines.size(); i++) {
        if (trim(lines[i]) != "Solution") {
            continue;
        }

        for (size_t j = i + 1; j < lines.size(); j++) {
            auto candidate = trim(lines[j]);
            if (candidate.empty()) {
                continue;
            }

            if (is_valid_lurd(candidate)) {
                lurd = candidate;
                return true;
            }

            return false;
        }
    }

    return false;
}

void reset_global_state()
{
    verbose = 0;
    level_id = 0;
    level_title[0] = 0;

    global_initial_sokoban_y = 0;
    global_initial_sokoban_x = 0;
    global_boxes_in_level = 0;

    strcpy(global_dir, ".");
    global_level_set_name[0] = 0;
    global_output_filename[0] = 0;

    YASC_mode = 0;
    save_best_flag = 0;
    extra_mem = 0;

    just_one_level = -1;
    global_from_level = -1;
    global_to_level = -1;

    time_limit = 600;
    start_time = 0;
    end_time = 0;

    strcpy(global_fail_reason, "Unknown reason");

    cores_num = 1;
    any_core_solved = 0;
    forced_alg = -1;

    level_sol_pushes = 0;
    level_sol_moves = 0;
    global_total_pushes = 0;
    global_total_moves = 0;
    total_sol_time = 0;
    total_levels_tried = 0;
    total_levels_solved = 0;
}

std::filesystem::path make_run_directory()
{
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto pid = static_cast<long long>(::getpid());
    auto path = std::filesystem::temp_directory_path() /
        ("festival-run-" + std::to_string(pid) + "-" + std::to_string(now));
    std::filesystem::create_directories(path);
    return path;
}

void set_result_message(CFestivalSolveResult *result, const std::string &message)
{
    free(result->message);
    result->message = duplicate_string(message);
}

void clear_child_environment() noexcept
{
    unsetenv(kChildMarkerEnv);
    unsetenv(kLevelPathEnv);
    unsetenv(kOutputPathEnv);
    unsetenv(kTimeLimitEnv);
    unsetenv(kCoresEnv);
}

std::string current_executable_path()
{
    uint32_t required_size = 0;
    (void)_NSGetExecutablePath(nullptr, &required_size);
    if (required_size == 0) {
        throw std::runtime_error("Failed to determine executable path size");
    }

    std::vector<char> buffer(required_size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &required_size) != 0) {
        throw std::runtime_error("Failed to determine executable path");
    }

    return std::string(buffer.data());
}

int run_solver_subprocess(
    const std::filesystem::path &level_path,
    const std::filesystem::path &output_path,
    int time_limit_seconds,
    int cores)
{
    const auto executable_path = current_executable_path();
    const auto time_limit_text = std::to_string(time_limit_seconds);
    const auto cores_text = std::to_string(cores);

    if (setenv(kChildMarkerEnv, "1", 1) != 0 ||
        setenv(kLevelPathEnv, level_path.c_str(), 1) != 0 ||
        setenv(kOutputPathEnv, output_path.c_str(), 1) != 0 ||
        setenv(kTimeLimitEnv, time_limit_text.c_str(), 1) != 0 ||
        setenv(kCoresEnv, cores_text.c_str(), 1) != 0) {
        clear_child_environment();
        throw std::runtime_error("Failed to configure Festival child process environment");
    }

    pid_t child_pid = 0;
    char *const child_argv[] = {
        const_cast<char *>(executable_path.c_str()),
        nullptr,
    };
    const int spawn_status = posix_spawn(
        &child_pid,
        executable_path.c_str(),
        nullptr,
        nullptr,
        child_argv,
        environ
    );
    clear_child_environment();

    if (spawn_status != 0) {
        throw std::runtime_error("Failed to spawn Festival child process");
    }

    int wait_status = 0;
    if (waitpid(child_pid, &wait_status, 0) < 0) {
        throw std::runtime_error("Failed waiting for Festival child process");
    }

    if (WIFEXITED(wait_status)) {
        return WEXITSTATUS(wait_status);
    }
    if (WIFSIGNALED(wait_status)) {
        return 128 + WTERMSIG(wait_status);
    }
    return 1;
}

__attribute__((constructor))
void cfestival_child_entrypoint()
{
    const char *marker = getenv(kChildMarkerEnv);
    if (marker == nullptr || strcmp(marker, "1") != 0) {
        return;
    }

    const char *level_path = getenv(kLevelPathEnv);
    const char *output_path = getenv(kOutputPathEnv);
    const char *time_limit_value = getenv(kTimeLimitEnv);
    const char *cores_value = getenv(kCoresEnv);
    if (level_path == nullptr || output_path == nullptr ||
        time_limit_value == nullptr || cores_value == nullptr) {
        _exit(64);
    }

    const int resolved_time_limit = std::max(1, atoi(time_limit_value));
    const int resolved_cores = std::max(1, atoi(cores_value));

    try {
        reset_global_state();

        std::vector<std::string> argv_storage = {
            "festival",
            level_path,
            "-from", "1",
            "-to", "1",
            "-time", std::to_string(resolved_time_limit),
            "-cores", std::to_string(resolved_cores),
            "-out_file", output_path,
        };
        std::vector<char *> argv;
        argv.reserve(argv_storage.size());
        for (auto &entry : argv_storage) {
            argv.push_back(entry.data());
        }

        const int exit_code = festival_main(static_cast<int>(argv.size()), argv.data());
        _exit(exit_code);
    } catch (...) {
        _exit(70);
    }
}

} // namespace

int cfestival_solve_level(
    const char *level_text,
    int time_limit_seconds,
    int cores,
    CFestivalSolveResult *out_result)
{
    if (out_result == nullptr) {
        return -1;
    }

    cfestival_free_result(out_result);
    out_result->status = CFESTIVAL_STATUS_ERROR;

    if (level_text == nullptr) {
        set_result_message(out_result, "Level text pointer is null");
        return -1;
    }

    std::lock_guard<std::mutex> lock(festival_mutex);

    std::filesystem::path run_directory;
    try {
        run_directory = make_run_directory();
    } catch (const std::exception &error) {
        set_result_message(out_result, std::string("Failed to create run directory: ") + error.what());
        return 0;
    }

    const auto level_path = run_directory / "level.sok";
    const auto output_path = run_directory / "solutions.sok";

    try {
        std::ofstream level_file(level_path);
        if (!level_file.is_open()) {
            throw std::runtime_error("Unable to open level file for writing");
        }
        level_file << "Generated by Festival package\n\n" << level_text << "\n";
        level_file.close();

        const int resolved_time_limit = time_limit_seconds > 0 ? time_limit_seconds : 30;
        const int resolved_cores = cores > 0 ? cores : 1;
        const int solver_exit_code = run_solver_subprocess(
            level_path,
            output_path,
            resolved_time_limit,
            resolved_cores
        );

        std::string lurd;
        bool solved = false;
        if (std::filesystem::exists(output_path)) {
            std::ifstream output_file(output_path);
            std::string output_text(
                (std::istreambuf_iterator<char>(output_file)),
                std::istreambuf_iterator<char>()
            );
            solved = extract_lurd(output_text, lurd);
        }

        if (solved) {
            int pushes = 0;
            for (char ch : lurd) {
                if (std::isupper(static_cast<unsigned char>(ch))) {
                    pushes += 1;
                }
            }

            out_result->status = CFESTIVAL_STATUS_SOLVED;
            out_result->minimum_moves = static_cast<int>(lurd.size());
            out_result->minimum_pushes = pushes;
            out_result->lurd = duplicate_string(lurd);
            out_result->message = nullptr;
        } else {
            if (solver_exit_code != 0) {
                out_result->status = CFESTIVAL_STATUS_ERROR;
                out_result->minimum_moves = 0;
                out_result->minimum_pushes = 0;
                out_result->lurd = nullptr;
                out_result->message = duplicate_string(
                    "Festival solver child failed with exit code " + std::to_string(solver_exit_code)
                );
            } else {
                out_result->status = CFESTIVAL_STATUS_UNSOLVED;
                out_result->minimum_moves = 0;
                out_result->minimum_pushes = 0;
                out_result->lurd = nullptr;
                out_result->message = nullptr;
            }
        }
    } catch (const std::exception &error) {
        out_result->status = CFESTIVAL_STATUS_ERROR;
        out_result->minimum_moves = 0;
        out_result->minimum_pushes = 0;
        out_result->lurd = nullptr;
        set_result_message(out_result, error.what());
    } catch (...) {
        out_result->status = CFESTIVAL_STATUS_ERROR;
        out_result->minimum_moves = 0;
        out_result->minimum_pushes = 0;
        out_result->lurd = nullptr;
        set_result_message(out_result, "Unknown Festival solver failure");
    }

    std::error_code remove_error;
    std::filesystem::remove_all(run_directory, remove_error);

    return 0;
}

void cfestival_free_result(CFestivalSolveResult *result)
{
    if (result == nullptr) {
        return;
    }

    free(result->lurd);
    free(result->message);

    result->lurd = nullptr;
    result->message = nullptr;
    result->minimum_moves = 0;
    result->minimum_pushes = 0;
    result->status = CFESTIVAL_STATUS_ERROR;
}
