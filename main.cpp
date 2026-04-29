#include "main.h"

struct Error {
	const std::string message;
	const std::string filePath;
	const std::string function;
	const std::string source;
	const std::string line;
};

static bool isCommandLine = true;
static bool isProgressBarActive = false;
static uint32_t filesSkipped = 0;

static struct {
	bool showHelp = false;
	bool silentAssertions = false;
	bool forceOverwrite = false;
	bool ignoreDebugInfo = false;
	bool minimizeDiffs = false;
	bool unrestrictedAscii = false;
	std::string inputPath;
	std::string outputPath;
	std::string extensionFilter;
} arguments;

struct Directory {
	const std::string path;
	std::vector<Directory> folders;
	std::vector<std::string> files;
};

static std::string string_to_lowercase(const std::string& string) {
	std::string lowercaseString = string;

	for (uint32_t i = lowercaseString.size(); i--;) {
		if (lowercaseString[i] < 'A' || lowercaseString[i] > 'Z') continue;
		lowercaseString[i] += 'a' - 'A';
	}

	return lowercaseString;
}

static std::string get_extension(const std::string& filename) {
	size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos) return "";
	return filename.substr(dotPos);
}

static std::string get_filename(const std::string& path) {
	size_t sepPos = path.find_last_of("/\\");
	if (sepPos == std::string::npos) return path;
	return path.substr(sepPos + 1);
}

static std::string get_directory(const std::string& path) {
	size_t sepPos = path.find_last_of("/\\");
	if (sepPos == std::string::npos) return ".";
	return path.substr(0, sepPos + 1);
}

static void find_files_recursively(Directory& directory) {
	namespace fs = std::filesystem;
	std::string fullPath = arguments.inputPath + directory.path;

	try {
		for (const auto& entry : fs::directory_iterator(fullPath)) {
			if (entry.is_directory()) {
				std::string folderName = entry.path().filename().string();
				if (folderName == "." || folderName == "..") continue;
				directory.folders.emplace_back(Directory{ .path = directory.path + folderName + "/" });
				find_files_recursively(directory.folders.back());
				if (directory.folders.back().files.empty() && directory.folders.back().folders.empty()) {
					directory.folders.pop_back();
				}
				continue;
			}

			std::string fileName = entry.path().filename().string();
			std::string fileExt = string_to_lowercase(get_extension(fileName));
			if (arguments.extensionFilter.empty() || arguments.extensionFilter == fileExt) {
				directory.files.emplace_back(fileName);
			}
		}
	} catch (...) {
		return;
	}
}

static bool decompile_files_recursively(const Directory& directory) {
	namespace fs = std::filesystem;
	std::string outputDir = arguments.outputPath + directory.path;
	
	try {
		fs::create_directories(outputDir);
	} catch (...) {
		print("Failed to create output directory: " + outputDir);
		return false;
	}

	std::string outputFile;

	for (uint32_t i = 0; i < directory.files.size(); i++) {
		outputFile = directory.files[i];
		size_t dotPos = outputFile.find_last_of('.');
		if (dotPos != std::string::npos) {
			outputFile = outputFile.substr(0, dotPos);
		}
		outputFile += ".lua";

		Bytecode bytecode(arguments.inputPath + directory.path + directory.files[i]);
		Ast ast(bytecode, arguments.ignoreDebugInfo, arguments.minimizeDiffs);
		Lua lua(bytecode, ast, arguments.outputPath + directory.path + outputFile, arguments.forceOverwrite, arguments.minimizeDiffs, arguments.unrestrictedAscii);

		try {
			print("--------------------\nInput file: " + bytecode.filePath + "\nReading bytecode...");
			bytecode();
			print("Building ast...");
			ast();
			print("Writing lua source...");
			lua();
			print("Output file: " + lua.filePath);
		} catch (const Error& error) {
			erase_progress_bar();

			if (arguments.silentAssertions) {
				print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\n" + error.message);
				filesSkipped++;
				continue;
			}

			print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\nFile: " + error.filePath + "\n\n" + error.message);
			print("Options: [c]ancel, [r]etry, [s]kip");
			
			std::string choice;
			std::getline(std::cin, choice);
			
			if (choice.empty()) choice = "c";
			switch (tolower(choice[0])) {
			case 'c':
				return false;
			case 'r':
				print("Retrying...");
				i--;
				continue;
			case 's':
				print("File skipped.");
				filesSkipped++;
				continue;
			default:
				print("Unknown option, skipping...");
				filesSkipped++;
				continue;
			}
		} catch (...) {
			print("Unknown exception\n\nFile: " + bytecode.filePath);
			throw;
		}
	}

	for (uint32_t i = 0; i < directory.folders.size(); i++) {
		if (!decompile_files_recursively(directory.folders[i])) return false;
	}

	return true;
}

static char* parse_arguments(const int& argc, char** const& argv) {
	if (argc < 2) return nullptr;
	arguments.inputPath = argv[1];

	bool isInputPathSet = true;

	if (!arguments.inputPath.empty() && arguments.inputPath.front() == '-') {
		arguments.inputPath.clear();
		isInputPathSet = false;
	}

	std::string argument;

	for (uint32_t i = isInputPathSet ? 2 : 1; i < argc; i++) {
		argument = argv[i];

		if (argument.size() >= 2 && argument.front() == '-') {
			if (argument[1] == '-') {
				argument = argument.substr(2);

				if (argument == "extension") {
					if (i <= argc - 2) {
						i++;
						arguments.extensionFilter = argv[i];
						continue;
					}
				} else if (argument == "force_overwrite") {
					arguments.forceOverwrite = true;
					continue;
				} else if (argument == "help") {
					arguments.showHelp = true;
					continue;
				} else if (argument == "ignore_debug_info") {
					arguments.ignoreDebugInfo = true;
					continue;
				} else if (argument == "minimize_diffs") {
					arguments.minimizeDiffs = true;
					continue;
				} else if (argument == "output") {
					if (i <= argc - 2) {
						i++;
						arguments.outputPath = argv[i];
						continue;
					}
				} else if (argument == "silent_assertions") {
					arguments.silentAssertions = true;
					continue;
				} else if (argument == "unrestricted_ascii") {
					arguments.unrestrictedAscii = true;
					continue;
				}
			} else if (argument.size() == 2) {
				switch (argument[1]) {
				case 'e':
					if (i > argc - 2) break;
					i++;
					arguments.extensionFilter = argv[i];
					continue;
				case 'f':
					arguments.forceOverwrite = true;
					continue;
				case '?':
				case 'h':
					arguments.showHelp = true;
					continue;
				case 'i':
					arguments.ignoreDebugInfo = true;
					continue;
				case 'm':
					arguments.minimizeDiffs = true;
					continue;
				case 'o':
					if (i > argc - 2) break;
					i++;
					arguments.outputPath = argv[i];
					continue;
				case 's':
					arguments.silentAssertions = true;
					continue;
				case 'u':
					arguments.unrestrictedAscii = true;
					continue;
				}
			}
		}

		return argv[i];
	}

	return nullptr;
}

static void wait_for_exit() {
	if (isCommandLine) return;
	print("Press Enter to exit.");
	std::cin.get();
}

int main(int argc, char* argv[]) {
	print(std::string(PROGRAM_NAME) + "\nCompiled on " + __DATE__);

	if (parse_arguments(argc, argv)) {
		print("Invalid argument: " + std::string(parse_arguments(argc, argv)) + "\nUse -? to show usage and options.");
		return EXIT_FAILURE;
	}

	if (arguments.showHelp) {
		print(
			"Usage: luajit-decompiler-v2 INPUT_PATH [options]\n"
			"\n"
			"Available options:\n"
			"  -h, -?, --help\t\tShow this message\n"
			"  -o, --output OUTPUT_PATH\tOverride default output directory\n"
			"  -e, --extension EXTENSION\tOnly decompile files with the specified extension\n"
			"  -s, --silent_assertions\tDisable assertion error pop-up window\n"
			"\t\t\t\t  and auto skip files that fail to decompile\n"
			"  -f, --force_overwrite\t\tAlways overwrite existing files\n"
			"  -i, --ignore_debug_info\tIgnore bytecode debug info\n"
			"  -m, --minimize_diffs\t\tOptimize output formatting to help minimize diffs\n"
			"  -u, --unrestricted_ascii\tDisable default UTF-8 encoding and string restrictions"
		);
		return EXIT_SUCCESS;
	}

	if (arguments.inputPath.empty()) {
		print("No input path specified!");
		return EXIT_FAILURE;
	}

	namespace fs = std::filesystem;

	if (arguments.outputPath.empty()) {
		char buffer[4096];
#ifdef _WIN32
		GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#else
		ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
		if (len > 0) buffer[len] = '\0';
		else buffer[0] = '\0';
#endif
		std::string exePath(buffer);
		arguments.outputPath = get_directory(exePath) + "output/";
	} else {
		try {
			auto pathAttributes = fs::status(arguments.outputPath);
			if (!fs::exists(pathAttributes)) {
				print("Failed to open output path: " + arguments.outputPath);
				return EXIT_FAILURE;
			}
			if (!fs::is_directory(pathAttributes)) {
				print("Output path is not a folder!");
				return EXIT_FAILURE;
			}
		} catch (...) {
			print("Failed to open output path: " + arguments.outputPath);
			return EXIT_FAILURE;
		}

		char lastChar = arguments.outputPath.back();
		if (lastChar != '/' && lastChar != '\\') {
			arguments.outputPath += '/';
		}
	}

	if (!arguments.extensionFilter.empty() && arguments.extensionFilter.front() != '.') {
		arguments.extensionFilter.insert(arguments.extensionFilter.begin(), '.');
	}
	arguments.extensionFilter = string_to_lowercase(arguments.extensionFilter);

	try {
		auto inputStatus = fs::status(arguments.inputPath);
		if (!fs::exists(inputStatus)) {
			print("Failed to open input path: " + arguments.inputPath);
			wait_for_exit();
			return EXIT_FAILURE;
		}

		Directory root;

		if (fs::is_directory(inputStatus)) {
			char lastChar = arguments.inputPath.back();
			if (lastChar != '/' && lastChar != '\\') {
				arguments.inputPath += '/';
			}

			find_files_recursively(root);

			if (root.files.empty() && root.folders.empty()) {
				print("No files " + (arguments.extensionFilter.size() ? "with extension " + arguments.extensionFilter + " " : "") + "found in path: " + arguments.inputPath);
				wait_for_exit();
				return EXIT_FAILURE;
			}
		} else {
			root.files.emplace_back(get_filename(arguments.inputPath));
			arguments.inputPath = get_directory(arguments.inputPath);
		}

		if (!decompile_files_recursively(root)) {
			print("--------------------\nAborted!");
			wait_for_exit();
			return EXIT_FAILURE;
		}
	} catch (...) {
		throw;
	}

	print("--------------------\n" + (filesSkipped ? "Failed to decompile " + std::to_string(filesSkipped) + " file" + (filesSkipped > 1 ? "s" : "") + ".\n" : "") + "Done!");
	wait_for_exit();

	return EXIT_SUCCESS;
}

void print(const std::string& message) {
	std::cout << message << std::endl;
}

void print_progress_bar(const double& progress, const double& total) {
	static char PROGRESS_BAR[] = "\r[====================]";

	const uint8_t threshold = static_cast<uint8_t>(std::round(20.0 / total * progress));

	for (uint8_t i = 20; i--;) {
		PROGRESS_BAR[i + 2] = i < threshold ? '=' : ' ';
	}

	std::cout << PROGRESS_BAR;
	std::cout.flush();
	isProgressBarActive = true;
}

void erase_progress_bar() {
	static constexpr char PROGRESS_BAR_ERASER[] = "\r                      \r";

	if (!isProgressBarActive) return;
	std::cout << PROGRESS_BAR_ERASER;
	std::cout.flush();
	isProgressBarActive = false;
}

void assert(const bool& assertion, const std::string& message, const std::string& filePath, const std::string& function, const std::string& source, const uint32_t& line) {
	if (!assertion) throw Error{
		.message = message,
		.filePath = filePath,
		.function = function,
		.source = source,
		.line = std::to_string(line)
	};
}

std::string byte_to_string(const uint8_t& byte) {
	char string[] = "0x00";
	uint8_t digit;

	for (uint8_t i = 2; i--;) {
		digit = (byte >> i * 4) & 0xF;
		string[3 - i] = digit >= 0xA ? 'A' + digit - 0xA : '0' + digit;
	}

	return string;
}