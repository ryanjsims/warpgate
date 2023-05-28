#pragma once
#include <optional>
#include <filesystem>
#include <spdlog/spdlog.h>

std::optional<std::filesystem::path> executable_location();