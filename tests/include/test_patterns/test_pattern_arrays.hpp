#pragma once

#include "test_pattern.hpp"

#include <pl/patterns/pattern_struct.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>
#include <pl/patterns/pattern_array_static.hpp>

#include <vector>

namespace pl::test {

    class TestPatternArrays : public TestPattern {
    public:
        TestPatternArrays(core::Evaluator *evaluator) : TestPattern(evaluator, "Arrays") {
            auto first = create<PatternArrayStatic>("u8", "first", 0x0, sizeof(u8[4]), 0);
            first->setEntries(create<PatternUnsigned>("u8", "", 0x0, sizeof(u8), 0), 4);

            auto second = create<PatternArrayStatic>("u8", "second", 0x4, sizeof(u8[4]), 0);
            second->setEntries(create<PatternUnsigned>("u8", "", 0x4, sizeof(u8), 0), 4);

            auto testStruct = create<PatternStruct>("Signature", "sign", 0x0, sizeof(u8[8]), 0);
            std::vector<std::shared_ptr<Pattern>> structMembers;
            {
                structMembers.push_back(std::move(first));
                structMembers.push_back(std::move(second));
            }
            testStruct->setEntries(std::move(structMembers));

            addPattern(std::move(testStruct));
        }
        ~TestPatternArrays() override = default;

        [[nodiscard]] std::string getSourceCode() const override {
            return R"(
                fn end_of_signature() {
                    return $ >= 8;
                };

                struct Signature {
                    u8 first[4];
                    u8 second[while(!end_of_signature())];
                };

                Signature sign @ 0x0;

                std::assert(sign.first[0] == 0x89, "Invalid 1st byte of signature");
                std::assert(sign.first[1] == 0x50, "Invalid 2nd byte of signature");
                std::assert(sign.first[2] == 0x4E, "Invalid 3rd byte of signature");
                std::assert(sign.first[3] == 0x47, "Invalid 4th byte of signature");
                std::assert(sizeof(sign.second) == 4, "Invalid size of signature");
                std::assert(sign.second[0] == 0x0D, "Invalid 5th byte of signature");
                std::assert(sign.second[1] == 0x0A, "Invalid 6th byte of signature");
                std::assert(sign.second[2] == 0x1A, "Invalid 7th byte of signature");
                std::assert(sign.second[3] == 0x0A, "Invalid 8th byte of signature");
            )";
        }
    };

}