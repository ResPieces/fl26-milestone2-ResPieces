#include "aiws/chunking_strategy.hpp"
#include "aiws/context_strategy.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/retrieval_strategy.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Student-written M2 tests
//
// Add your own tests to this file. Your tests are part of the submitted work
// and are evaluated under Student-Written Testing & Validation.
//
// Do not modify tests/public_tests.cpp.
//
// Your tests should exercise important M2 behavior beyond the supplied public
// tests. Consider default compatibility, custom strategies, runtime dispatch,
// invalid configuration, ownership/move behavior, and component interactions.

namespace
{
    int failures = 0;
    void check(bool condition, const std::string &message)
    {
        if (!condition)
        {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
    std::string numbered_words(int n)
    {
        std::string s;
        for (int i = 0; i < n; ++i)
        {
            if (!s.empty())
                s += ' ';
            s += "w" + std::to_string(i);
        }
        return s;
    }
}

int main()
{
    using namespace aiws;

    /*~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-
    *
    *               Tests: Public Tests From M1
    *    Purpose: Ensure M1 functionality with M2 Structure
    *
    ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-*/

    // check(ProcessingCore::normalize("Hello,  WORLD! 2026") == "hello world 2026",
    //       "normalization contract");
    // check(ProcessingCore::normalize("...\t---").empty(), "separator-only input");

    // auto r = core.search("alpha beta", 10);
    // check(r.size() == 2, "query returns candidate union");
    // check(!r.empty() && r[0].document_id == "d1", "coverage/frequency ranking");

    // auto ctx = core.build_context("alpha beta", 10, 2);
    // check(ctx.size() == 1 && ctx[0].token_count == 2 && ctx[0].truncated,
    //       "context truncates final selected chunk at token budget");

    // Workspace long_ws;
    // long_ws.add_document(Document{"long", "Long", numbered_words(121)});
    // core.rebuild(long_ws);
    // check(core.chunk_count() == 2, "121 tokens creates overlapping second chunk");
    // check(core.chunks().size() >= 2 &&
    //           core.chunks()[0].token_count == 120 && core.chunks()[1].token_count == 21,
    //       "hard limit and 20-token overlap");

    // bool negative_threw = false;
    // try
    // {
    //     (void)core.search("w1", -1);
    // }
    // catch (const std::invalid_argument &)
    // {
    //     negative_threw = true;
    // }
    // check(negative_threw, "negative k throws invalid_argument");

    // if (failures == 0)
    // {
    //     std::cout << "All M1 public tests passed.\n";
    //     return 0;
    // }
    // std::cerr << failures << " M1 public test(s) failed.\n";

    /*~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-
    *
    *               Test: Default Constructor
    *    Purpose: Ensure functionality is the same as M1
    *
    ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-*/

    failures = 0;

    Workspace ws;
    ws.add_document(Document{"document1", "Title 1", "Body Text Hurray!!"});
    ws.add_document(Document{"document2", "Title 2", "More Body Text!"});
    ProcessingCore core;
    core.rebuild(ws);
    check(core.chunk_count() == 2, "Should build to one chunk");
    check(core.document_frequency("BODY>?!") == 2, "Chunks with correct term");
    check(core.term_frequency("body", "document1#0") == 1, "Amount of times body appears in doc 1 ");

    if (failures == 0)
    {
        std::cout << "Default Contructor Tests Passed - M1 Core functionality retained\n";
        return 0;
    }
    std::cerr << failures << " Tests failed.\n";

    return 1;
}
