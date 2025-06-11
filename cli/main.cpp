// cli/main.cpp

#include "fastlanes.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h> // for file_size()
#include <vector>

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void usage() {
	std::cout << "fastlanes <command> [options] <file> …\n"
	             "Commands:\n"
	             "  meta        Show file metadata\n"
	             "  schema      Print schema\n"
	             "  head        Print first N rows   (-n <num>)\n"
	             "  cat         Dump all rows        (--json --max-rows N)\n"
	             "  csv         Convert file → CSV\n"
	             "  inspect     Row-group & column stats\n"
	             "  rowcount    Print total row count\n"
	             "  size        File & row-group sizes\n"
	             "  merge       Merge files → one\n"
	             "  dump        Low-level dump\n"
	             "  pages       Dump page headers & contents\n"
	             "  dictionary  Show dictionary pages\n"
	             "  check-stats Verify row-group statistics\n"
	             "  convert     Parquet → CSV (alias of csv)\n"
	             "  to-avro     Parquet → Avro\n"
	             "  csv-schema  Print CSV schema\n"
	             "  help        Show this message\n";
}

static void not_impl(std::string_view cmd) {
	std::cout << "[fastlanes] '" << cmd << "' is recognised but not implemented yet.\n";
}

// -----------------------------------------------------------------------------
// Command stubs
// -----------------------------------------------------------------------------

static int cmd_meta(const std::vector<std::string>& files) {
	not_impl("meta");
	return 0;
}
static int cmd_schema(const std::vector<std::string>& files) {
	not_impl("schema");
	return 0;
}
static int cmd_head(const std::vector<std::string>& files, int64_t n = 10) {
	not_impl("head");
	return 0;
}
static int cmd_cat(const std::vector<std::string>& files, bool /*json_out*/ = false, int64_t /*max_rows*/ = -1) {
	not_impl("cat");
	return 0;
}
static int cmd_csv(const std::vector<std::string>& files) {
	not_impl("csv");
	return 0;
}
static int cmd_inspect(const std::vector<std::string>& files) {
	not_impl("inspect");
	return 0;
}
static int cmd_rowcount(const std::vector<std::string>& files) {
	not_impl("rowcount");
	return 0;
}
static int cmd_size(const std::vector<std::string>& files) {
	not_impl("size");
	return 0;
}
static int cmd_merge(const std::vector<std::string>& args) {
	not_impl("merge");
	return 0;
}
static int cmd_dump(const std::vector<std::string>& files) {
	not_impl("dump");
	return 0;
}
static int cmd_pages(const std::vector<std::string>& files) {
	not_impl("pages");
	return 0;
}
static int cmd_dictionary(const std::vector<std::string>& files) {
	not_impl("dictionary");
	return 0;
}
static int cmd_check_stats(const std::vector<std::string>& files) {
	not_impl("check-stats");
	return 0;
}
static int cmd_convert(const std::vector<std::string>& args) {
	not_impl("convert");
	return 0;
}
static int cmd_to_avro(const std::vector<std::string>& files) {
	not_impl("to-avro");
	return 0;
}
static int cmd_csv_schema(const std::vector<std::string>& files) {
	not_impl("csv-schema");
	return 0;
}

// -----------------------------------------------------------------------------
// Dispatch
// -----------------------------------------------------------------------------

int dispatch(const std::string& cmd, std::vector<std::string> args) {
	if (cmd == "help" || cmd == "--help" || cmd == "-h") {
		usage();
		return 0;
	}

	if (cmd == "merge") {
		if (args.size() < 2) {
			std::cerr << "Error: merge needs <output.parquet> <in1.parquet> [...]\n";
			return 1;
		}
		return cmd_merge(args);
	}

	if (args.empty()) {
		usage();
		return 1;
	}

	bool    json_out = false;
	int64_t max_rows = -1;
	int64_t head_n   = 10;

	if (cmd == "head") {
		for (std::size_t i = 0; i + 1 < args.size(); ++i) {
			if (args[i] == "-n") {
				head_n     = std::stoll(args[i + 1]);
				auto first = args.begin() + static_cast<std::vector<std::string>::difference_type>(i);
				auto last  = args.begin() + static_cast<std::vector<std::string>::difference_type>(i + 2);
				args.erase(first, last);
				break;
			}
		}
		return cmd_head(args, head_n);
	} else if (cmd == "cat") {
		for (std::size_t i = 0; i < args.size();) {
			if (args[i] == "--json") {
				json_out = true;
				auto it  = args.begin() + static_cast<std::vector<std::string>::difference_type>(i);
				args.erase(it);
			} else if (args[i] == "--max-rows" && i + 1 < args.size()) {
				max_rows   = std::stoll(args[i + 1]);
				auto first = args.begin() + static_cast<std::vector<std::string>::difference_type>(i);
				auto last  = args.begin() + static_cast<std::vector<std::string>::difference_type>(i + 2);
				args.erase(first, last);
			} else {
				++i;
			}
		}
		return cmd_cat(args, json_out, max_rows);
	}

	if (cmd == "meta") {
		return cmd_meta(args);
	} else if (cmd == "schema") {
		return cmd_schema(args);
	} else if (cmd == "csv") {
		return cmd_csv(args);
	} else if (cmd == "inspect") {
		return cmd_inspect(args);
	} else if (cmd == "rowcount") {
		return cmd_rowcount(args);
	} else if (cmd == "size") {
		return cmd_size(args);
	} else if (cmd == "dump") {
		return cmd_dump(args);
	} else if (cmd == "pages") {
		return cmd_pages(args);
	} else if (cmd == "dictionary") {
		return cmd_dictionary(args);
	} else if (cmd == "check-stats") {
		return cmd_check_stats(args);
	} else if (cmd == "convert") {
		return cmd_convert(args);
	} else if (cmd == "to-avro") {
		return cmd_to_avro(args);
	} else if (cmd == "csv-schema") {
		return cmd_csv_schema(args);
	} else {
		std::cerr << "Unknown subcommand: " << cmd << "\n";
		usage();
		return 1;
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		usage();
		return 1;
	}

	std::string              cmd = argv[1];
	std::vector<std::string> args;
	for (int i = 2; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	return dispatch(cmd, std::move(args));
}
