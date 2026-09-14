#include "tengine/interpreter.hpp"
#include "tengine/bytecode.hpp"
#include "tengine/lexer.hpp"
#include "tengine/parser.hpp"
#include "tengine/type_checker.hpp"

#include <cassert>
#include <sstream>

int main() {
    te::Lexer lexer("let x = 2 + 3 * 4; print x;");
    const auto tokens = lexer.scan();
    assert(lexer.diagnostics().empty());

    te::Parser parser(tokens);
    const auto program = parser.parse();
    assert(parser.diagnostics().empty());
    assert(program.size() == 2);

    te::Interpreter interpreter;
    interpreter.execute(program);
    assert(interpreter.diagnostics().empty());

    te::Lexer featureLexer("let flag = false; flag = !flag; print flag == true;");
    const auto featureTokens = featureLexer.scan();
    assert(featureLexer.diagnostics().empty());
    te::Parser featureParser(featureTokens);
    const auto featureProgram = featureParser.parse();
    assert(featureParser.diagnostics().empty());
    te::TypeChecker featureChecker;
    featureChecker.check(featureProgram);
    assert(featureChecker.diagnostics().empty());

    te::Lexer invalidLexer("let count = 1; count = \"wrong\";");
    const auto invalidTokens = invalidLexer.scan();
    te::Parser invalidParser(invalidTokens);
    const auto invalidProgram = invalidParser.parse();
    te::TypeChecker invalidChecker;
    invalidChecker.check(invalidProgram);
    assert(!invalidChecker.diagnostics().empty());

    te::Lexer functionLexer(
        "fn add(a, b) { return a + b; } let result = add(2, 3);");
    const auto functionTokens = functionLexer.scan();
    te::Parser functionParser(functionTokens);
    const auto functionProgram = functionParser.parse();
    assert(functionParser.diagnostics().empty());
    te::TypeChecker functionChecker;
    functionChecker.check(functionProgram);
    assert(functionChecker.diagnostics().empty());
    te::Interpreter functionInterpreter;
    functionInterpreter.execute(functionProgram);
    assert(functionInterpreter.diagnostics().empty());

    te::Lexer arityLexer("fn one(value) { return value; } print one(1, 2);");
    const auto arityTokens = arityLexer.scan();
    te::Parser arityParser(arityTokens);
    const auto arityProgram = arityParser.parse();
    te::TypeChecker arityChecker;
    arityChecker.check(arityProgram);
    assert(!arityChecker.diagnostics().empty());

    te::Lexer controlLexer(
        "let counter = 0; while (counter < 3) { counter = counter + 1; } "
        "if (counter == 3 && !false) { print counter; } else { print 0; }");
    const auto controlTokens = controlLexer.scan();
    te::Parser controlParser(controlTokens);
    const auto controlProgram = controlParser.parse();
    assert(controlParser.diagnostics().empty());
    te::TypeChecker controlChecker;
    controlChecker.check(controlProgram);
    assert(controlChecker.diagnostics().empty());
    te::Interpreter controlInterpreter;
    controlInterpreter.execute(controlProgram);
    assert(controlInterpreter.diagnostics().empty());

    te::BytecodeCompiler controlCompiler;
    const auto controlBytecode = controlCompiler.compile(controlProgram);
    assert(controlCompiler.diagnostics().empty());
    te::BytecodeVM controlVm;
    controlVm.execute(controlBytecode);
    assert(controlVm.diagnostics().empty());

    te::BytecodeCompiler functionCompiler;
    const auto functionBytecode = functionCompiler.compile(functionProgram);
    assert(functionCompiler.diagnostics().empty());
    te::BytecodeVM functionVm;
    functionVm.execute(functionBytecode);
    assert(functionVm.diagnostics().empty());

    te::Lexer builtinLexer(
        "print len(\"hello\"); print sqrt(81); print abs(-4); print typeOf(true);");
    const auto builtinTokens = builtinLexer.scan();
    te::Parser builtinParser(builtinTokens);
    const auto builtinProgram = builtinParser.parse();
    assert(builtinParser.diagnostics().empty());
    te::TypeChecker builtinChecker;
    builtinChecker.check(builtinProgram);
    assert(builtinChecker.diagnostics().empty());
    te::Interpreter builtinInterpreter;
    builtinInterpreter.execute(builtinProgram);
    assert(builtinInterpreter.diagnostics().empty());
    te::BytecodeCompiler builtinCompiler;
    const auto builtinBytecode = builtinCompiler.compile(builtinProgram);
    assert(builtinCompiler.diagnostics().empty());
    te::BytecodeVM builtinVm;
    builtinVm.execute(builtinBytecode);
    assert(builtinVm.diagnostics().empty());

    return 0;
}
