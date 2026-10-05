#include "comptime_eval.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <charconv>

bool ComptimeEvaluator::isNumeric(const ComptimeValue& v) {
    return std::holds_alternative<int64_t>(v) || std::holds_alternative<double>(v);
}

double ComptimeEvaluator::asFloat(const ComptimeValue& v, int line) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    if (std::holds_alternative<int64_t>(v)) return static_cast<double>(std::get<int64_t>(v));
    throw std::runtime_error("Expected numeric comptime value at line " + std::to_string(line));
}

int64_t ComptimeEvaluator::checkedIntOp(int64_t l, int64_t r, const std::string& op, int line) {
    using limits = std::numeric_limits<int64_t>;
    if (op == "+") {
        if ((r > 0 && l > limits::max() - r) || (r < 0 && l < limits::min() - r))
            throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
        return l + r;
    }
    if (op == "-") {
        if ((r < 0 && l > limits::max() + r) || (r > 0 && l < limits::min() + r))
            throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
        return l - r;
    }
    if (op == "*") {
        if (l != 0 && r != 0) {
            if (l == -1 && r == limits::min()) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
            if (r == -1 && l == limits::min()) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
            if (l > 0) {
                if (r > 0 && l > limits::max() / r) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
                if (r < 0 && r < limits::min() / l) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
            } else {
                if (r > 0 && l < limits::min() / r) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
                if (r < 0 && l < limits::max() / r) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
            }
        }
        return l * r;
    }
    if (op == "/") {
        if (r == 0) throw std::runtime_error("Division by zero at comptime (line " + std::to_string(line) + ")");
        if (l == limits::min() && r == -1) throw std::runtime_error("comptime integer overflow at line " + std::to_string(line));
        return l / r;
    }
    throw std::runtime_error("Unknown integer operator '" + op + "' at line " + std::to_string(line));
}

ComptimeValue ComptimeEvaluator::evaluate(const Expr* expr) {
    if (!expr) throw std::runtime_error("Cannot evaluate a null expression at comptime");
    switch (expr->kind) {
        case ExprKind::IntLit: {
            const std::string& text = expr->strValue;
            int base = 10;
            std::size_t start = 0;
            if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) { base = 16; start = 2; }
            else if (text.size() > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) { base = 2; start = 2; }
            std::uint64_t raw = 0;
            const auto [ptr, ec] = std::from_chars(text.data() + start, text.data() + text.size(), raw, base);
            if (ec != std::errc{} || ptr != text.data() + text.size() ||
                raw > static_cast<std::uint64_t>(std::numeric_limits<int64_t>::max())) {
                throw std::runtime_error("invalid comptime integer literal at line " + std::to_string(expr->line));
            }
            return static_cast<int64_t>(raw);
        }
        case ExprKind::FloatLit: return std::stod(expr->strValue);
        case ExprKind::StringLit: return expr->strValue;
        case ExprKind::UnaryOp: return evalUnary(expr);
        case ExprKind::BinaryOp: return evalBinary(expr);
        case ExprKind::Identifier: {
            Symbol* sym = symbols.resolve(expr->strValue);
            if (!sym) throw std::runtime_error("Unknown identifier '" + expr->strValue + "' at comptime (line " + std::to_string(expr->line) + ")");
            if (!sym->isComptime || !sym->comptimeValue)
                throw std::runtime_error("'" + expr->strValue + "' is not a comptime value (line " + std::to_string(expr->line) + ")");
            return *sym->comptimeValue;
        }
        default:
            throw std::runtime_error("Expression is not comptime-evaluable at line " + std::to_string(expr->line));
    }
}

ComptimeValue ComptimeEvaluator::evalUnary(const Expr* expr) {
    ComptimeValue v = evaluate(expr->value.get());
    if (expr->op == "+") {
        if (!isNumeric(v)) throw std::runtime_error("Unary '+' requires a numeric comptime value at line " + std::to_string(expr->line));
        return v;
    }
    if (expr->op == "-") {
        if (std::holds_alternative<int64_t>(v)) {
            if (std::get<int64_t>(v) == std::numeric_limits<int64_t>::min())
                throw std::runtime_error("comptime integer overflow at line " + std::to_string(expr->line));
            return -std::get<int64_t>(v);
        }
        if (std::holds_alternative<double>(v)) return -std::get<double>(v);
    }
    if (expr->op == "!") {
        if (!std::holds_alternative<bool>(v)) throw std::runtime_error("Unary '!' requires bool at line " + std::to_string(expr->line));
        return !std::get<bool>(v);
    }
    throw std::runtime_error("Unknown unary operator '" + expr->op + "' at line " + std::to_string(expr->line));
}

ComptimeValue ComptimeEvaluator::evalBinary(const Expr* expr) {
    ComptimeValue left = evaluate(expr->left.get());
    ComptimeValue right = evaluate(expr->right.get());
    const std::string& op = expr->op;

    if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
        if (!std::holds_alternative<std::string>(left) || !std::holds_alternative<std::string>(right))
            throw std::runtime_error("String operators require two strings at line " + std::to_string(expr->line));
        const auto& l = std::get<std::string>(left);
        const auto& r = std::get<std::string>(right);
        if (op == "==") return l == r;
        if (op == "!=") return l != r;
        throw std::runtime_error("Operator '" + op + "' is not valid on strings at line " + std::to_string(expr->line));
    }

    if (std::holds_alternative<bool>(left) || std::holds_alternative<bool>(right)) {
        if (!std::holds_alternative<bool>(left) || !std::holds_alternative<bool>(right) || (op != "==" && op != "!="))
            throw std::runtime_error("Boolean comptime values only support == and != at line " + std::to_string(expr->line));
        return op == "==" ? std::get<bool>(left) == std::get<bool>(right)
                           : std::get<bool>(left) != std::get<bool>(right);
    }

    if (!isNumeric(left) || !isNumeric(right))
        throw std::runtime_error("Operator '" + op + "' requires numeric operands at line " + std::to_string(expr->line));

    const bool bothInt = std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right);
    if (bothInt) {
        const int64_t l = std::get<int64_t>(left), r = std::get<int64_t>(right);
        if (op == "==") return l == r;
        if (op == "!=") return l != r;
        if (op == "<") return l < r;
        if (op == ">") return l > r;
        if (op == "<=") return l <= r;
        if (op == ">=") return l >= r;
        if (op == "%") {
            if (r == 0) throw std::runtime_error("Division by zero at comptime (line " + std::to_string(expr->line) + ")");
            if (l == std::numeric_limits<int64_t>::min() && r == -1)
                throw std::runtime_error("comptime integer overflow at line " + std::to_string(expr->line));
            return l % r;
        }
        if (op == "&") return l & r;
        if (op == "|") return l | r;
        if (op == "^") return l ^ r;
        if (op == "<<" || op == ">>") {
            if (r < 0 || r >= 64) throw std::runtime_error("invalid comptime shift at line " + std::to_string(expr->line));
            const auto amount = static_cast<unsigned>(r);
            if (op == "<<") return static_cast<int64_t>(static_cast<std::uint64_t>(l) << amount);
            return l >> amount;
        }
        return checkedIntOp(l, r, op, expr->line);
    }

    const double l = asFloat(left, expr->line), r = asFloat(right, expr->line);
    if (op == "+") return l + r;
    if (op == "-") return l - r;
    if (op == "*") return l * r;
    if (op == "/") {
        if (r == 0.0) throw std::runtime_error("Division by zero at comptime (line " + std::to_string(expr->line) + ")");
        return l / r;
    }
    if (op == "==") return l == r;
    if (op == "!=") return l != r;
    if (op == "<") return l < r;
    if (op == ">") return l > r;
    if (op == "<=") return l <= r;
    if (op == ">=") return l >= r;
    throw std::runtime_error("Unknown operator '" + op + "' at comptime (line " + std::to_string(expr->line) + ")");
}

void ComptimeEvaluator::evaluateAndDeclare(const Stmt* comptimeDecl) {
    if (!comptimeDecl || comptimeDecl->kind != StmtKind::ComptimeDecl)
        throw std::runtime_error("Invalid comptime declaration");
    ComptimeValue value = evaluate(comptimeDecl->comptimeValue.get());
    Symbol sym;
    sym.name = comptimeDecl->comptimeName;
    sym.isComptime = true;
    sym.comptimeValue = value;
    sym.declaredLine = comptimeDecl->line;
    symbols.declare(comptimeDecl->comptimeName, sym);
}
