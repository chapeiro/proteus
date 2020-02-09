/*
    Proteus -- High-performance query processing on heterogeneous hardware.

                            Copyright (c) 2014
        Data Intensive Applications and Systems Laboratory (DIAS)
                École Polytechnique Fédérale de Lausanne

                            All Rights Reserved.

    Permission to use, copy, modify and distribute this software and
    its documentation is hereby granted, provided that both the
    copyright notice and this permission notice appear in all copies of
    the software, derivative works or modified versions, and any
    portions thereof, and that both notices appear in supporting
    documentation.

    This code is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
    DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
    RESULTING FROM THE USE OF THIS SOFTWARE.
*/

#include "expressions/expressions.hpp"

#include "util/context.hpp"

expression_t toExpression(Monoid m, expression_t lhs, expression_t rhs) {
  switch (m) {
    case SUM:
      return lhs + rhs;
    case MULTIPLY:
      return lhs * rhs;
    case MAX:
      return expression_t::make<expressions::MaxExpression>(lhs, rhs);
    case OR:
      return lhs | rhs;
    case AND:
      return lhs & rhs;
    default:
      string error_msg = string("Unknown monoid");
      LOG(ERROR) << error_msg;
      throw runtime_error(string(error_msg));
  }
}

llvm::Constant *getIdentityElementIfSimple(Monoid m, const ExpressionType *type,
                                           Context *context) {
  llvm::Type *llvmType = type->getLLVMType(context->getLLVMContext());
  typeID outputType = type->getTypeID();
  switch (m) {
    case SUM: {
      switch (outputType) {
        case DATE:
        case INT64:
        case INT: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType, 0);
        }
        case FLOAT: {
          return llvm::ConstantFP::get(llvmType, 0.0);
        }
        default: {
          string error_msg =
              string("[Monoid: ] Sum/Multiply/Max operate on numerics (not " +
                     type->getType() + ")");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    case MULTIPLY: {
      switch (outputType) {
        case INT64:
        case INT: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType, 1);
        }
        case FLOAT: {
          return llvm::ConstantFP::get(llvmType, 1.0);
        }
        default: {
          string error_msg =
              string("[Monoid: ] Sum/Multiply/Max operate on numerics");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    case MAX: {
      switch (outputType) {
        case DATE:
        case INT64: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType,
                                        std::numeric_limits<int64_t>::min());
        }
        case INT: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType,
                                        std::numeric_limits<int32_t>::min());
        }
        case FLOAT: {
          return llvm::ConstantFP::getInfinity(llvmType, true);
        }
        default: {
          string error_msg =
              string("[Monoid: ] Sum/Multiply/Max operate on numerics");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    case MIN: {
      switch (outputType) {
        case DATE:
        case INT64: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType,
                                        std::numeric_limits<int64_t>::max());
        }
        case INT: {
          return llvm::ConstantInt::get((llvm::IntegerType *)llvmType,
                                        std::numeric_limits<int32_t>::max());
        }
        case FLOAT: {
          return llvm::ConstantFP::getInfinity(llvmType, false);
        }
        default: {
          string error_msg =
              string("[Monoid: ] Sum/Multiply/Max operate on numerics");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    case OR: {
      switch (outputType) {
        case BOOL: {
          return llvm::ConstantInt::getFalse(context->getLLVMContext());
        }
        default: {
          string error_msg = string("[Monoid: ] Or/And operate on booleans");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    case AND: {
      switch (outputType) {
        case BOOL: {
          return llvm::ConstantInt::getTrue(context->getLLVMContext());
        }
        default: {
          string error_msg = string("[Monoid: ] Or/And operate on booleans");
          LOG(ERROR) << error_msg;
          throw runtime_error(error_msg);
        }
      }
    }
    default: {
      return nullptr;
    }
  }
}

ExpressionType *expressions::ExtractExpression::createReturnType(
    extract_unit u) {
  switch (u) {
    case extract_unit::SECOND:
    case extract_unit::MINUTE:
    case extract_unit::HOUR:
    case extract_unit::DAYOFWEEK:
    case extract_unit::DAYOFMONTH:
    case extract_unit::DAYOFYEAR:
    case extract_unit::WEEK:
    case extract_unit::MONTH:
    case extract_unit::QUARTER:
    case extract_unit::YEAR:
    case extract_unit::MILLENNIUM:
    case extract_unit::CENTURY:
    case extract_unit::DECADE: {
      return new IntType();
    }
    default: {
      string error_msg =
          "[extract_unit: ] Unknown return type for extract unit";
      LOG(ERROR) << error_msg;
      throw runtime_error(error_msg);
    }
  }
}

template <>
ProteusValue expressions::
    ExprVisitorVisitable<expression_t, expressions::Expression>::acceptTandem(
        ExprTandemVisitor &v, const expressions::Expression *expr) const {
  return static_cast<const expression_t *>(this)->acceptTandem(v, expr);
}

template <>
ProteusValue expressions::ExprVisitorVisitable<
    expression_t, expressions::Expression>::accept(ExprVisitor &v) const {
  return static_cast<const expression_t *>(this)->accept(v);
}

template <template <typename> class C = std::less, typename Tret = bool>
class ExpressionComparatorVisitor : public ExprTandemVisitor {
 public:
  Tret res;
  ExpressionComparatorVisitor() {}

  ProteusValue visit(const expressions::IntConstant *e1,
                     const expressions::IntConstant *e2) {
    res = C<expressions::IntConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::Int64Constant *e1,
                     const expressions::Int64Constant *e2) {
    res = C<expressions::Int64Constant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::DateConstant *e1,
                     const expressions::DateConstant *e2) {
    res = C<expressions::DateConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::FloatConstant *e1,
                     const expressions::FloatConstant *e2) {
    res = C<expressions::FloatConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::BoolConstant *e1,
                     const expressions::BoolConstant *e2) {
    res = C<expressions::BoolConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::StringConstant *e1,
                     const expressions::StringConstant *e2) {
    res = C<expressions::StringConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::DStringConstant *e1,
                     const expressions::DStringConstant *e2) {
    res = C<expressions::DStringConstant>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::InputArgument *e1,
                     const expressions::InputArgument *e2) {
    res = C<expressions::InputArgument>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::RecordProjection *e1,
                     const expressions::RecordProjection *e2) {
    res = C<expressions::RecordProjection>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::RecordConstruction *e1,
                     const expressions::RecordConstruction *e2) {
    res = C<expressions::RecordConstruction>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::IfThenElse *e1,
                     const expressions::IfThenElse *e2) {
    res = C<expressions::IfThenElse>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::EqExpression *e1,
                     const expressions::EqExpression *e2) {
    res = C<expressions::EqExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::NeExpression *e1,
                     const expressions::NeExpression *e2) {
    res = C<expressions::NeExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::GeExpression *e1,
                     const expressions::GeExpression *e2) {
    res = C<expressions::GeExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::GtExpression *e1,
                     const expressions::GtExpression *e2) {
    res = C<expressions::GtExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::LeExpression *e1,
                     const expressions::LeExpression *e2) {
    res = C<expressions::LeExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::LtExpression *e1,
                     const expressions::LtExpression *e2) {
    res = C<expressions::LtExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::AddExpression *e1,
                     const expressions::AddExpression *e2) {
    res = C<expressions::AddExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::SubExpression *e1,
                     const expressions::SubExpression *e2) {
    res = C<expressions::SubExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::MultExpression *e1,
                     const expressions::MultExpression *e2) {
    res = C<expressions::MultExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::DivExpression *e1,
                     const expressions::DivExpression *e2) {
    res = C<expressions::DivExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::ModExpression *e1,
                     const expressions::ModExpression *e2) {
    res = C<expressions::ModExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::AndExpression *e1,
                     const expressions::AndExpression *e2) {
    res = C<expressions::AndExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::OrExpression *e1,
                     const expressions::OrExpression *e2) {
    res = C<expressions::OrExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::ProteusValueExpression *e1,
                     const expressions::ProteusValueExpression *e2) {
    res = C<expressions::ProteusValueExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::MinExpression *e1,
                     const expressions::MinExpression *e2) {
    res = C<expressions::MinExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::MaxExpression *e1,
                     const expressions::MaxExpression *e2) {
    res = C<expressions::MaxExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::HashExpression *e1,
                     const expressions::HashExpression *e2) {
    res = C<expressions::HashExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::NegExpression *e1,
                     const expressions::NegExpression *e2) {
    res = C<expressions::NegExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::ExtractExpression *e1,
                     const expressions::ExtractExpression *e2) {
    res = C<expressions::ExtractExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::TestNullExpression *e1,
                     const expressions::TestNullExpression *e2) {
    res = C<expressions::TestNullExpression>{}(*e1, *e2);
    return {};
  }
  ProteusValue visit(const expressions::CastExpression *e1,
                     const expressions::CastExpression *e2) {
    res = C<expressions::CastExpression>{}(*e1, *e2);
    return {};
  }
};

bool expressions::Expression::operator<(
    const expressions::Expression &r) const {
  if (getTypeID() == r.getTypeID()) {
    ExpressionComparatorVisitor<std::less> v;
    acceptTandem(v, &r);
    return v.res;
  } else {
    return getTypeID() < r.getTypeID();
  }
}

int probeDictionary(void *dict, const std::string &v) {
  auto d = (std::map<int, std::string> *)dict;
  auto it = std::find_if(d->begin(), d->end(),
                         [v](const auto &o) { return v == o.second; });
  assert(it != d->end() && "String not found in dictionary");
  return it->first;
}

expression_t::expression_t(std::string v, void *dict)
    : expression_t(
          expressions::DStringConstant{probeDictionary(dict, v), dict}) {}

expressions::EqExpression eq(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::EqExpression eq(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return eq(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return eq(lhs, expression_t{rhs});
  }
}

expressions::EqExpression eq(const expression_t &lhs, const char *rhs) {
  return eq(lhs, std::string{rhs});
}

expressions::EqExpression eq(const std::string &lhs, const expression_t &rhs) {
  return eq(rhs, lhs);
}

expressions::EqExpression eq(const char *lhs, const expression_t &rhs) {
  return eq(rhs, lhs);
}

expressions::NeExpression ne(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::NeExpression ne(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return ne(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return ne(lhs, expression_t{rhs});
  }
}

expressions::NeExpression ne(const expression_t &lhs, const char *rhs) {
  return ne(lhs, std::string{rhs});
}

expressions::NeExpression ne(const std::string &lhs, const expression_t &rhs) {
  return ne(rhs, lhs);
}

expressions::NeExpression ne(const char *lhs, const expression_t &rhs) {
  return ne(rhs, lhs);
}

expressions::GeExpression ge(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::GeExpression ge(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return ge(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return ge(lhs, expression_t{rhs});
  }
}

expressions::GeExpression ge(const expression_t &lhs, const char *rhs) {
  return ge(lhs, std::string{rhs});
}

expressions::LeExpression ge(const std::string &lhs, const expression_t &rhs) {
  return le(rhs, lhs);  // Reverse inequality
}

expressions::LeExpression ge(const char *lhs, const expression_t &rhs) {
  return le(rhs, lhs);  // Reverse inequality
}

expressions::GtExpression gt(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::GtExpression gt(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return gt(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return gt(lhs, expression_t{rhs});
  }
}

expressions::GtExpression gt(const expression_t &lhs, const char *rhs) {
  return gt(lhs, std::string{rhs});
}

expressions::LtExpression gt(const std::string &lhs, const expression_t &rhs) {
  return lt(rhs, lhs);  // Reverse inequality
}

expressions::LtExpression gt(const char *lhs, const expression_t &rhs) {
  return lt(rhs, lhs);  // Reverse inequality
}

expressions::LeExpression le(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::LeExpression le(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return le(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return le(lhs, expression_t{rhs});
  }
}

expressions::LeExpression le(const expression_t &lhs, const char *rhs) {
  return le(lhs, std::string{rhs});
}

expressions::GeExpression le(const std::string &lhs, const expression_t &rhs) {
  return ge(rhs, lhs);  // Reverse inequality
}

expressions::GeExpression le(const char *lhs, const expression_t &rhs) {
  return ge(rhs, lhs);  // Reverse inequality
}

expressions::LtExpression lt(const expression_t &lhs, const expression_t &rhs) {
  return {lhs, rhs};
}

expressions::LtExpression lt(const expression_t &lhs, const std::string &rhs) {
  auto dstring = dynamic_cast<const DStringType *>(lhs.getExpressionType());
  if (dstring) {
    return lt(lhs, expression_t{rhs, dstring->getDictionary()});
  } else {
    return lt(lhs, expression_t{rhs});
  }
}

expressions::LtExpression lt(const expression_t &lhs, const char *rhs) {
  return lt(lhs, std::string{rhs});
}

expressions::GtExpression lt(const std::string &lhs, const expression_t &rhs) {
  return gt(rhs, lhs);  // Reverse inequality
}

expressions::GtExpression lt(const char *lhs, const expression_t &rhs) {
  return gt(rhs, lhs);  // Reverse inequality
}

int64_t dateToTimestamp(const std::string &d) {
  std::tm t{};
  strptime(d.c_str(), "%Y-%m-%d %H:%M:%S", &t);
  t.tm_isdst = -1;  // Unknown
  return std::mktime(&t) * 1000;
}

expressions::DateConstant::DateConstant(std::string d)
    : DateConstant(dateToTimestamp(d)) {}
