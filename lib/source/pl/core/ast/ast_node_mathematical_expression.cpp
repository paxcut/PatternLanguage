#include <pl/core/ast/ast_node_mathematical_expression.hpp>

#include <pl/core/evaluator.hpp>
#include <pl/patterns/pattern.hpp>

#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/helpers/concepts.hpp>

#include <pl/patterns/pattern_enum.hpp>

template<std::integral T>
struct std::common_type<pl::u128, T> {
    using type = pl::u128;
};

template<std::floating_point T>
struct std::common_type<pl::u128, T> {
    using type = double;
};

template<>
struct std::common_type<pl::u128, pl::i128> {
    using type = pl::u128;
};

template<>
struct std::common_type<pl::i128, pl::u128> {
    using type = pl::u128;
};

template<std::integral T>
struct std::common_type<pl::i128, T> {
    using type = pl::i128;
};

template<std::floating_point T>
struct std::common_type<pl::i128, T> {
    using type = double;
};

namespace pl::core::ast {

    template<typename L, typename R>
    struct signed_result {
        constexpr static bool value = is_signed<L>::value && is_signed<R>::value;
    };


    ASTNodeMathematicalExpression::ASTNodeMathematicalExpression(std::unique_ptr<ASTNode> &&left, std::unique_ptr<ASTNode> &&right, Token::Operator op)
    : m_left(std::move(left)), m_right(std::move(right)), m_operator(op) { }

    ASTNodeMathematicalExpression::ASTNodeMathematicalExpression(const ASTNodeMathematicalExpression &other) : ASTNode(other) {
        this->m_operator = other.m_operator;
        this->m_left     = other.m_left->clone();
        this->m_right    = other.m_right->clone();
    }

    [[nodiscard]] std::unique_ptr<ASTNode> ASTNodeMathematicalExpression::evaluate(Evaluator *evaluator) const {
        [[maybe_unused]] auto context = evaluator->updateRuntime(this);

        if (this->getLeftOperand() == nullptr || this->getRightOperand() == nullptr)
            err::E0002.throwError("Void expression used in ternary expression.", "If you used a function for one of the operands, make sure it returned a value.", this->getLocation());

        const auto throwInvalidOperandError = [this]() -> ASTNode * {
            err::E0002.throwError("Invalid operand used in mathematical expression.", { }, this->getLocation());
            return nullptr;
        };

        auto leftNode  = this->getLeftOperand()->evaluate(evaluator);
        auto rightNode = this->getRightOperand()->evaluate(evaluator);

        auto leftLiteral  = dynamic_cast<ASTNodeLiteral *>(leftNode.get());
        auto rightLiteral = dynamic_cast<ASTNodeLiteral *>(rightNode.get());

        if (leftLiteral == nullptr || rightLiteral == nullptr)
            throwInvalidOperandError();

        auto leftValue = leftLiteral->getValue();
        auto rightValue = rightLiteral->getValue();

        auto handlePatternOperations = [&, this](auto left, auto right) -> ASTNode * {
            switch (this->getOperator()) {
                case Token::Operator::BoolEqual:
                    return new ASTNodeLiteral(left == right);
                case Token::Operator::BoolNotEqual:
                    return new ASTNodeLiteral(left != right);
                case Token::Operator::BoolGreaterThan:
                    return new ASTNodeLiteral(left > right);
                case Token::Operator::BoolLessThan:
                    return new ASTNodeLiteral(left < right);
                case Token::Operator::BoolGreaterThanOrEqual:
                    return new ASTNodeLiteral(left >= right);
                case Token::Operator::BoolLessThanOrEqual:
                    return new ASTNodeLiteral(left <= right);
                default:
                    return throwInvalidOperandError();
            }
        };

        return std::unique_ptr<ASTNode>(std::visit(wolv::util::overloaded {
            [&](u128 left, const std::shared_ptr<ptrn::Pattern> &right)                 -> ASTNode * { return handlePatternOperations(left, right->getValue().toUnsigned());        },
            [&](i128 left, const std::shared_ptr<ptrn::Pattern> &right)                 -> ASTNode * { return handlePatternOperations(left, right->getValue().toSigned());          },
            [&](double left, const std::shared_ptr<ptrn::Pattern> &right)               -> ASTNode * { return handlePatternOperations(left, right->getValue().toFloatingPoint());   },
            [&](char left, const std::shared_ptr<ptrn::Pattern> &right)                 -> ASTNode * { return handlePatternOperations(left, right->getValue().toSigned());          },
            [&](bool left, const std::shared_ptr<ptrn::Pattern> &right)                 -> ASTNode * { return handlePatternOperations(left, right->getValue().toBoolean());         },
            [&](const std::string &left, const std::shared_ptr<ptrn::Pattern> &right)   -> ASTNode * { return handlePatternOperations(left, right->getValue().toString(true));      },
            [&](const std::shared_ptr<ptrn::Pattern> &left, u128 right)                 -> ASTNode * { return handlePatternOperations(left->getValue().toUnsigned(), right);        },
            [&](const std::shared_ptr<ptrn::Pattern> &left, i128 right)                 -> ASTNode * { return handlePatternOperations(left->getValue().toSigned(), right);          },
            [&](const std::shared_ptr<ptrn::Pattern> &left, double right)               -> ASTNode * { return handlePatternOperations(left->getValue().toFloatingPoint(), right);   },
            [&](const std::shared_ptr<ptrn::Pattern> &left, char right)                 -> ASTNode * { return handlePatternOperations(left->getValue().toSigned(), right);          },
            [&](const std::shared_ptr<ptrn::Pattern> &left, bool right)                 -> ASTNode * { return handlePatternOperations(left->getValue().toBoolean(), right);         },
            [&](const std::shared_ptr<ptrn::Pattern> &left, const std::string &right)   -> ASTNode * { return handlePatternOperations(left->getValue().toString(true), right);      },
            [&](u128, const std::string &)                              -> ASTNode * { return throwInvalidOperandError(); },
            [&](i128, const std::string &)                              -> ASTNode * { return throwInvalidOperandError(); },
            [&](double, const std::string &)                            -> ASTNode * { return throwInvalidOperandError(); },
            [&](bool, const std::string &)                              -> ASTNode * { return throwInvalidOperandError(); },
            [&, this](const std::shared_ptr<ptrn::Pattern> &left, const std::shared_ptr<ptrn::Pattern> &right) -> ASTNode * {
                std::vector<u8> leftBytes(left->getSize()), rightBytes(right->getSize());

                evaluator->readData(left->getOffset(), leftBytes.data(), leftBytes.size(), left->getSection());
                evaluator->readData(right->getOffset(), rightBytes.data(), rightBytes.size(), right->getSection());
                switch (this->getOperator()) {
                   case Token::Operator::BoolEqual:
                       return new ASTNodeLiteral(leftBytes == rightBytes);
                   case Token::Operator::BoolNotEqual:
                       return new ASTNodeLiteral(leftBytes != rightBytes);
                   default:
                       return throwInvalidOperandError();
                }
            },
            [&, this](const std::string &left, auto right) -> ASTNode * {
                switch (this->getOperator()) {
                   case Token::Operator::Star:
                   {
                       if (static_cast<i128>(right) < 0)
                           err::E0002.throwError("Cannot repeat string a negative number of times.", { }, this->getLocation());

                       std::string result;
                       for (u128 i = 0; i < static_cast<u128>(right); i++)
                           result += left;
                       return new ASTNodeLiteral(result);
                   }
                   default:
                       return throwInvalidOperandError();
                }
            },
            [&, this](const std::string &left, const std::string &right) -> ASTNode * {
                switch (this->getOperator()) {
                   case Token::Operator::Plus:
                       return new ASTNodeLiteral(left + right);
                   case Token::Operator::BoolEqual:
                       return new ASTNodeLiteral(left == right);
                   case Token::Operator::BoolNotEqual:
                       return new ASTNodeLiteral(left != right);
                   case Token::Operator::BoolGreaterThan:
                       return new ASTNodeLiteral(left > right);
                   case Token::Operator::BoolLessThan:
                       return new ASTNodeLiteral(left < right);
                   case Token::Operator::BoolGreaterThanOrEqual:
                       return new ASTNodeLiteral(left >= right);
                   case Token::Operator::BoolLessThanOrEqual:
                       return new ASTNodeLiteral(left <= right);
                   default:
                       return throwInvalidOperandError();
                }
            },
            [&, this](const std::string &left, char right) -> ASTNode * {
                switch (this->getOperator()) {
                   case Token::Operator::Plus:
                       return new ASTNodeLiteral(left + right);
                   default:
                       return throwInvalidOperandError();
                }
            },
            [&, this](char left, const std::string &right) -> ASTNode * {
                switch (this->getOperator()) {
                   case Token::Operator::Plus:
                       return new ASTNodeLiteral(left + right);
                   default:
                       return throwInvalidOperandError();
                }
            },
            [&, this](auto left, auto right) -> ASTNode * {
                using RBase = std::common_type_t<decltype(left), decltype(right)>;
                using R = std::conditional_t<std::same_as<RBase, int>, i128, RBase>;

                switch (this->getOperator()) {
                   case Token::Operator::Plus:
                        return new ASTNodeLiteral(R(R(left) + R(right)));
                   case Token::Operator::Minus:
                        return new ASTNodeLiteral(R(R(left) - R(right)));
                   case Token::Operator::Star:
                        return new ASTNodeLiteral(R(R(left) * R(right)));
                   case Token::Operator::Slash:
                       if (right == 0)
                           err::E0002.throwError("Division by zero.", { }, this->getLocation());
                       if constexpr (std::same_as<R, bool>)
						   err::E0001.throwError("Cannot divide boolean values.", { }, this->getLocation());
                       else
                           return new ASTNodeLiteral(R(R(left) / R(right)));
                   case Token::Operator::Percent:
                       if (right == 0)
                           err::E0002.throwError("Division by zero.", { }, this->getLocation());
                       if constexpr (std::same_as<R, bool>)
                           err::E0001.throwError("Cannot divide boolean values.", { }, this->getLocation());
                       else
                           return new ASTNodeLiteral(R(modulus(left, right)));
                   case Token::Operator::LeftShift:
                       return new ASTNodeLiteral(R(shiftLeft(left, right)));
                   case Token::Operator::RightShift:
                       return new ASTNodeLiteral(R(shiftRight(left, right)));
                   case Token::Operator::BitAnd:
                       return new ASTNodeLiteral(R(bitAnd(left, right)));
                   case Token::Operator::BitXor:
                       return new ASTNodeLiteral(R(bitXor(left, right)));
                   case Token::Operator::BitOr:
                       return new ASTNodeLiteral(R(bitOr(left, right)));
                   case Token::Operator::BitNot:
                       return new ASTNodeLiteral(R(bitNot(left, right)));
                   case Token::Operator::BoolEqual:
                       return new ASTNodeLiteral(bool(left == static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolNotEqual:
                       return new ASTNodeLiteral(bool(left != static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolGreaterThan:
                       return new ASTNodeLiteral(bool(left > static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolLessThan:
                       return new ASTNodeLiteral(bool(left < static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolGreaterThanOrEqual:
                       return new ASTNodeLiteral(bool(left >= static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolLessThanOrEqual:
                       return new ASTNodeLiteral(bool(left <= static_cast<decltype(left)>(right)));
                   case Token::Operator::BoolAnd:
                       return new ASTNodeLiteral(bool(left && right));
                   case Token::Operator::BoolXor:
                       return new ASTNodeLiteral(bool((left && !right) || (!left && right)));
                   case Token::Operator::BoolOr:
                       return new ASTNodeLiteral(bool(left || right));
                   case Token::Operator::BoolNot:
                       return new ASTNodeLiteral(bool(!right));
                   default:
                       return throwInvalidOperandError();
                }
            }
        },
        leftValue,
        rightValue));
    }

}