#include "tengine/interpreter.hpp"
#include "tengine/bytecode.hpp"
#include "tengine/lexer.hpp"
#include "tengine/parser.hpp"
#include "tengine/type_checker.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void printDiagnostics(const std::vector<te::Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        std::cerr << "T-Engine error [" << diagnostic.location.line << ":"
                  << diagnostic.location.column << "]: " << diagnostic.message << '\n';
    }
}

int run(const std::string& source, bool useVm) {
    te::Lexer lexer(source);
    const auto tokens = lexer.scan();
    if (!lexer.diagnostics().empty()) {
        printDiagnostics(lexer.diagnostics());
        return 1;
    }

    te::Parser parser(tokens);
    const auto program = parser.parse();
    if (!parser.diagnostics().empty()) {
        printDiagnostics(parser.diagnostics());
        return 1;
    }

    te::TypeChecker checker;
    checker.check(program);
    if (!checker.diagnostics().empty()) {
        printDiagnostics(checker.diagnostics());
        return 1;
    }

    if (useVm) {
        te::BytecodeCompiler compiler;
        const auto bytecode = compiler.compile(program);
        if (!compiler.diagnostics().empty()) {
            printDiagnostics(compiler.diagnostics());
            return 1;
        }
        te::BytecodeVM vm;
        vm.execute(bytecode);
        if (!vm.diagnostics().empty()) {
            printDiagnostics(vm.diagnostics());
            return 1;
        }
    } else {
        te::Interpreter interpreter;
        interpreter.execute(program);
        if (!interpreter.diagnostics().empty()) {
            printDiagnostics(interpreter.diagnostics());
            return 1;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "usage: tengine [--vm] <file.te>\n";
        return 64;
    }

    const bool useVm = argc == 3 && std::string(argv[1]) == "--vm";
    const char* filePath = useVm ? argv[2] : argv[1];
    if (argc == 3 && !useVm) {
        std::cerr << "unknown option: " << argv[1] << '\n';
        return 64;
    }

    std::ifstream file(filePath);
    if (!file) {
        std::cerr << "tidak bisa membuka file: " << filePath << '\n';
        return 1;
    }
    return run({std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()}, useVm);
}
