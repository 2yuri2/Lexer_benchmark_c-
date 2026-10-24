
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <windows.h>
#include <psapi.h>

using namespace std;
using namespace chrono;

struct Token {
    string type;
    string value;
};

vector<Token> lexCPP(const string& source) {
    vector<Token> tokens;
    size_t i = 0;

    while (i < source.size()) {
        unsigned char c = static_cast<unsigned char>(source[i]);

        if (isspace(c)) {
            ++i;
            continue;
        }

        // Single-line comments
        if (source[i] == '/' && i + 1 < source.size()
            && source[i + 1] == '/') {
            i += 2;
            while (i < source.size() && source[i] != '\n') {
                ++i;
            }
            continue;
        }

        // Multi-line comments
        if (source[i] == '/' && i + 1 < source.size()
            && source[i + 1] == '*') {
            i += 2;

            while (i + 1 < source.size() &&
                   !(source[i] == '*' && source[i + 1] == '/')) {
                ++i;
            }

            if (i + 1 < source.size()) {
                i += 2;
            }

            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            size_t start = i++;

            while (i < source.size()) {
                unsigned char next =
                    static_cast<unsigned char>(source[i]);

                if (!isalnum(next) && next != '_') {
                    break;
                }

                ++i;
            }

            tokens.push_back({
                "name",
                source.substr(start, i - start)
            });

            continue;
        }

        // Basic numeric literals
        if (isdigit(c)) {
            size_t start = i++;

            while (i < source.size()) {
                unsigned char next =
                    static_cast<unsigned char>(source[i]);

                if (!isalnum(next) && next != '.' && next != '_') {
                    break;
                }

                ++i;
            }

            tokens.push_back({
                "num",
                source.substr(start, i - start)
            });

            continue;
        }

        // String and character literals
        if (source[i] == '"' || source[i] == '\'') {
            char quote = source[i];
            size_t start = i++;

            while (i < source.size()) {
                if (source[i] == '\\' && i + 1 < source.size()) {
                    i += 2;
                } else if (source[i] == quote) {
                    ++i;
                    break;
                } else {
                    ++i;
                }
            }

            tokens.push_back({
                "string",
                source.substr(start, i - start)
            });

            continue;
        }

        // Common multi-character operators
        if (i + 2 < source.size()) {
            string op3 = source.substr(i, 3);

            if (op3 == "..." || op3 == "->*") {
                tokens.push_back({"operator", op3});
                i += 3;
                continue;
            }
        }

        if (i + 1 < source.size()) {
            string op2 = source.substr(i, 2);

            const vector<string> operators = {
                "==", "!=", "<=", ">=", "++", "--",
                "+=", "-=", "*=", "/=", "%=", "&&",
                "||", "<<", ">>", "->", "::", "&=",
                "|=", "^=", ".*"
            };

            if (find(operators.begin(), operators.end(), op2)
                != operators.end()) {
                tokens.push_back({"operator", op2});
                i += 2;
                continue;
            }
        }

        // Single-character operators and punctuation
        string symbol(1, source[i]);

        if (string("+-*/%=<>!&|^~?:").find(source[i])
            != string::npos) {
            tokens.push_back({"operator", symbol});
        } else {
            tokens.push_back({"punct", symbol});
        }

        ++i;
    }

    return tokens;
}

SIZE_T getPeakWorkingSet() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);

    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        return counters.PeakWorkingSetSize;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: harness.exe <source.cpp>\n";
        return 1;
    }

    ifstream file(argv[1], ios::binary);

    if (!file) {
        cerr << "Error: Cannot open file: "
             << argv[1] << '\n';
        return 1;
    }

    string source(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );

    constexpr int iterations = 1000;
    vector<double> times;
    times.reserve(iterations);

    // Warm-up
    auto warmup = lexCPP(source);

    // Benchmark tokenization only
    for (int i = 0; i < iterations; ++i) {
        auto start = steady_clock::now();

        auto tokens = lexCPP(source);

        auto end = steady_clock::now();

        double elapsed =
            duration<double, milli>(end - start).count();

        times.push_back(elapsed);
    }

    sort(times.begin(), times.end());

    double median;

    if (times.size() % 2 == 0) {
        median = (
            times[times.size() / 2 - 1] +
            times[times.size() / 2]
        ) / 2.0;
    } else {
        median = times[times.size() / 2];
    }

    // Display the tokens outside the timed benchmark
    vector<Token> tokens = lexCPP(source);

    for (size_t i = 0; i < tokens.size(); ++i) {
        cout << i + 1 << " "
             << tokens[i].type << " "
             << quoted(tokens[i].value) << '\n';
    }

    SIZE_T peakMemory = getPeakWorkingSet();

    cout << "\nIterations: " << iterations << '\n';

    cout << fixed << setprecision(4);
    cout << "Median time (ms): " << median << '\n';

    if (peakMemory > 0) {
        cout << "Peak process working set (MB): "
             << static_cast<double>(peakMemory) /
                (1024.0 * 1024.0) << '\n';
    } else {
        cout << "Peak memory measurement unavailable.\n";
    }

    return 0;
}
