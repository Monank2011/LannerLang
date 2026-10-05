#include "src/sema/symbol_table.hpp"
#include <iostream>
#include <cassert>

int passed = 0, failed = 0;

void check(bool cond, const std::string& label) {
    if (cond) { std::cout << "PASS: " << label << "\n"; passed++; }
    else { std::cout << "FAIL: " << label << "\n"; failed++; }
}

int main() {
    // Test 1: basic declare + resolve in global scope
    {
        SymbolTable st;
        Symbol s;
        s.name = "x";
        s.declaredLine = 1;
        st.declare("x", s);
        Symbol* found = st.resolve("x");
        check(found != nullptr && found->name == "x", "resolve finds a declared global symbol");
    }

    // Test 2: resolving an undeclared name returns nullptr
    {
        SymbolTable st;
        check(st.resolve("nonexistent") == nullptr, "resolve returns nullptr for undeclared name");
    }

    // Test 3: shadowing in a nested scope
    {
        SymbolTable st;
        Symbol outer; outer.name = "x"; outer.declaredLine = 1;
        st.declare("x", outer);

        st.pushScope();
        Symbol inner; inner.name = "x"; inner.declaredLine = 2;
        st.declare("x", inner);

        Symbol* found = st.resolve("x");
        check(found != nullptr && found->declaredLine == 2, "inner scope shadows outer declaration");

        st.popScope();
        Symbol* foundAfterPop = st.resolve("x");
        check(foundAfterPop != nullptr && foundAfterPop->declaredLine == 1,
              "popping scope restores visibility of outer declaration");
    }

    // Test 4: parent-chain resolution (declared outside, visible inside nested scope)
    {
        SymbolTable st;
        Symbol outer; outer.name = "y"; outer.declaredLine = 5;
        st.declare("y", outer);

        st.pushScope();
        st.pushScope(); // two levels deep
        Symbol* found = st.resolve("y");
        check(found != nullptr && found->declaredLine == 5,
              "deeply nested scope resolves symbol from outer parent chain");
        st.popScope();
        st.popScope();
    }

    // Test 5: redeclaration in the same scope throws
    {
        SymbolTable st;
        Symbol a; a.name = "z"; a.declaredLine = 1;
        st.declare("z", a);
        bool threw = false;
        try {
            Symbol b; b.name = "z"; b.declaredLine = 2;
            st.declare("z", b);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "redeclaring the same name in the same scope throws");
    }

    // Test 6: popping the global scope throws
    {
        SymbolTable st;
        bool threw = false;
        try {
            st.popScope();
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "popping the global scope throws");
    }

    // Test 7: the unified memory ledger and stable binding identity persist
    // through declare/resolve and can be used to distinguish shadowed bindings.
    {
        SymbolTable st;
        Symbol s;
        s.name = "moves";
        st.declare("moves", s);

        Symbol* found = st.resolve("moves");
        check(found != nullptr && found->bindingId != 0, "declared symbols receive stable binding identities");

        const auto id = found->bindingId;
        found->memory.markMoved();
        Symbol* foundAgain = st.resolveBinding(id);
        check(foundAgain != nullptr && foundAgain->memory.isMoved(),
              "memory move state persists through binding identity lookup");

        st.pushScope();
        Symbol shadow;
        shadow.name = "moves";
        st.declare("moves", shadow);
        Symbol* inner = st.resolve("moves");
        check(inner != nullptr && inner->bindingId != id, "shadowed bindings receive distinct identities");
        check(st.resolveBinding(id) != nullptr && st.resolveBinding(id)->bindingId == id,
              "outer binding remains addressable by identity under shadowing");
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}