#include "fls/csv/csv-parser/parser.hpp"
#include "fls/connection.hpp"
#include "fls/json/nlohmann/json.hpp"
#include "fls/std/filesystem.hpp"
#include <gtest/gtest.h>
#include <random>
#include <sstream>
#include <string>
#include <fstream>

namespace fastlanes {

// Generate a random CSV row with a fixed delimiter ',' and newline terminator.
static std::string random_csv_row(std::mt19937 &rng, int columns) {
    std::uniform_int_distribution<int> len_dist(0, 8);
    std::uniform_int_distribution<char> char_dist('a', 'z');

    std::string row;
    for (int c = 0; c < columns; ++c) {
        if (c > 0) row.push_back(',');
        int len = len_dist(rng);
        for (int i = 0; i < len; ++i) {
            row.push_back(char_dist(rng));
        }
    }
    row.push_back('\n');
    return row;
}

TEST(QuickFuzz, RandomCSVParsing) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> col_dist(1, 6);

    // Generate a fixed column count for this run
    int cols = col_dist(rng);

    std::string csv_data;
    for (int iter = 0; iter < 100; ++iter) {
        std::string row = random_csv_row(rng, cols);
        csv_data += row;

        std::istringstream csv_stream(row);
        aria::csv::CsvParser parser =
            aria::csv::CsvParser(csv_stream).delimiter(',').terminator('\n');

        for (auto &fields : parser) {
            EXPECT_EQ(fields.size(), static_cast<size_t>(cols));
        }
    }

    // ------------------------------------------------------------------
    // Write CSV and schema to a temporary directory
    // ------------------------------------------------------------------
    using fastlanes::fs::path;
    path temp_dir = fs::temp_directory_path() / "fls_quick_fuzz";
    fs::create_directories(temp_dir);

    path csv_path = temp_dir / "generated.csv";
    {
        std::ofstream out(csv_path);
        out << csv_data;
    }

    nlohmann::json schema;
    schema["columns"] = nlohmann::json::array();
    for (int c = 0; c < cols; ++c) {
        nlohmann::json col;
        col["name"] = "col" + std::to_string(c);
        col["type"] = "string";
        schema["columns"].push_back(col);
    }
    path schema_path = temp_dir / "schema.json";
    {
        std::ofstream out(schema_path);
        out << schema.dump(2);
    }

    // ------------------------------------------------------------------
    // Encode to FastLanes and verify round trip
    // ------------------------------------------------------------------
    path fls_path = temp_dir / "data.fls";
    Connection con1;
    con1.set_n_vectors_per_rowgroup(1).read_csv(temp_dir).to_fls(fls_path);
    const auto &original_table = con1.get_table();

    Connection con2;
    auto fls_reader = con2.reset().read_fls(fls_path);
    auto decoded_table = fls_reader->materialize();

    auto result = (original_table == *decoded_table);
    EXPECT_TRUE(result.is_equal);

    fs::remove_all(temp_dir);
}

} // namespace fastlanes

