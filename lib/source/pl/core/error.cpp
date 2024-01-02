#include <pl/core/errors/error.hpp>
#include <pl/api.hpp>

#include <pl/helpers/utils.hpp>

#include <wolv/utils/string.hpp>

#include <fmt/format.h>

namespace pl::core::err::impl {

    std::string formatLocation(Location location) {
        if (location.line > 0 && location.column > 0) {
            return fmt::format("{}:{}:{}", location.source->source, location.line, location.column);
        }
        return "";
    }

    std::string formatLines(Location location) {
        std::string result;

        const auto lines = wolv::util::splitString(location.source->content, "\n");

        if (location.line < lines.size() + 1) {
            const auto lineNumberPrefix = fmt::format("{} | ", location.line);
            auto errorLine = wolv::util::replaceStrings(lines[location.line - 1], "\r", "");
            u32 arrowPosition = location.column;

            // Trim to size
            if (errorLine.length() > 40) { // shrink to [column - 20; column + 20]
                const auto column = location.column;
                auto start = column > 20 ? column - 20 : 0;
                auto end = column + 20 < errorLine.length() ? column + 20 : errorLine.length();
                // Search for whitespaces on both sides until a maxium of 10 characters and change start/end accordingly
                for(auto i = 0; i < 10; ++i) {
                    if(start > 0 && errorLine[start] != ' ') {
                        --start;
                    }
                    if(end < errorLine.length() && errorLine[end] != ' ') {
                        ++end;
                    }
                }
                errorLine = errorLine.substr(start, end - start);
                arrowPosition = column - start;
            }

            result += fmt::format("{}{}\n", lineNumberPrefix, errorLine);

            {
                const auto arrowSpacing = std::string(lineNumberPrefix.length() + arrowPosition, ' ');
                // Add arrow with length of the token
                result += arrowSpacing;
                result += std::string(location.length, '^') + '\n';
                // Add spacing for the error message
                result += arrowSpacing;
                result += std::string(location.length, ' ');
            }
        }

        return result;
    }

    std::string formatRuntimeErrorShort(
        char prefix,
        const std::string &title,
        const std::string &description)
    {
        return fmt::format("runtime error: {}\n{}", prefix, title, description);
    }

    std::string formatRuntimeError(
            const Location& location,
            char prefix,
            const std::string &title,
            const std::string &description,
            const std::string &hint)
    {
        std::string errorMessage;

        errorMessage += fmt::format("runtime error: {}\n", prefix, title);

        if (location.line > 0 && location.column > 0) {
            errorMessage += formatLocation(location) + description + '\n';
        } else {
            errorMessage += description + '\n';
        }

        if (!hint.empty()) {
            errorMessage += fmt::format("hint: {}", hint);
        }

        return errorMessage;
    }

    std::string formatCompilerError(
            const Location& location,
            const std::string& message,
            const std::string& description,
            const std::vector<Location>& trace) {
        std::string errorMessage = "error: " + message + "\n";

        if (location.line > 0) {
            errorMessage += "  -->   in " + formatLocation(location) + "\n";
        }

        for (const auto &traceLocation : trace) {
            errorMessage += "   >> from " + formatLocation(traceLocation) + "\n";
        }

        if (location.line > 0) {
            errorMessage += formatLines(location) + "\n";
        }

        if (!description.empty()) {
            errorMessage += "\n" + description + "\n";
        }

        return errorMessage;
    }

}