# M2 Technical & Design Understanding

This file is an assessed technical-understanding artifact, not ordinary project documentation.
Answer all four questions using your own submitted implementation. Concise answers are acceptable when they are technically correct and specific.

Generic descriptions of C++ concepts or restatements of the assignment that do not identify and explain corresponding parts of your code will receive limited credit.

## 1. Polymorphism and dynamic dispatch - 1.5 points

Identify one place in your M2 implementation where runtime polymorphism occurs. Name the relevant base interface, derived implementation, and `ProcessingCore` function involved. Trace the call from `ProcessingCore` to the selected strategy implementation and explain why the derived implementation is invoked.

Then explain what would change if the relevant operation were not declared `virtual`.

## 2. Ownership and lifetime - 1.5 points

Identify where one of the strategy objects is created, where ownership is transferred, and which object ultimately owns it. Explain how `std::unique_ptr` represents that ownership relationship and when the strategy object is destroyed.

Also explain why `ProcessingCore` is move-only and why the strategy base classes require virtual destructors.

## 3. Architecture, extensibility, and M1 compatibility - 1.5 points

Explain one specific architectural decision in your M2 implementation that makes the processing system extensible while preserving M1 behavior.

Identify the classes or interfaces involved and explain both:
- how the default configuration preserves M1 behavior; and
- how a different implementation can be substituted without changing the normal `ProcessingCore` API.

Include one plausible design alternative and explain why the M2 design is preferable for this milestone. The alternative does not need to be something you actually implemented.

    Answer: The big thing was the use of virtual functions for the three different strategies. By implementing
        virtual functions that allowed for the creation of custom strategies, I allowed for the ability to
        create more catered processing cores. Because of that generalized approach, my original M1 solutions
        were able to inherit from the strategy files to preserve functionality. Another solution would have been
        to simply keep creating files like chunker.cpp and hard coding them based on scenario, but that can get
        cumbersome and unintutive later down the line.

## 4. Testing and defect reasoning - 1.5 points

Select one meaningful test from your `tests/student_tests.cpp`.

Explain:
- what M2 requirement the test validates;
- what specific implementation defect the test could detect; and
- why your test provides useful evidence beyond simply rerunning the supplied public tests.

If your test uses a custom strategy, explain how its observable behavior demonstrates that `ProcessingCore` is actually using runtime substitution.

    Answer: I want to highlight my test 2. In student_test.cpp, I create a custom chunking strategy, which I then
            apply to a custom processing core. The test leans on the workspace that was used to test the default
            processing core in test 1. Upon running rebuild and checking the values of custom core, we can see
            that they did change based on the workspace, and therefore runtime substitution works. This helps to
            validate the wider goal of creating a system with dynamic rules based on a given case/strategy we wish
            to pursue.
