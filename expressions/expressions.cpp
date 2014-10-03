/*
	RAW -- High-performance querying over raw, never-seen-before data.

							Copyright (c) 2014
		Data Intensive Applications and Systems Labaratory (DIAS)
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

namespace expressions {

BinaryExpression::~BinaryExpression() {}

Value* IntConstant::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* FloatConstant::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* BoolConstant::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* StringConstant::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* InputArgument::accept(ExprVisitor &v) {
	return v.visit(this);
}

/**
 * The binary expressions
 */
Value* EqExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* NeExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* GeExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* GtExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* LeExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* LtExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* AddExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* SubExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* MultExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

Value* DivExpression::accept(ExprVisitor &v) {
	return v.visit(this);
}

}

//===---------------------------------------------------------------------------===//
// "Visitor(s)" responsible for generating the appropriate code per Expression 'node'
//===---------------------------------------------------------------------------===//
Value* ExpressionGeneratorVisitor::visit(expressions::IntConstant *e) {
	return ConstantInt::get(context->getLLVMContext(), APInt(32, e->getVal()));
}

Value* ExpressionGeneratorVisitor::visit(expressions::FloatConstant *e) {
	return ConstantFP::get(context->getLLVMContext(), APFloat(e->getVal()));
}

Value* ExpressionGeneratorVisitor::visit(expressions::BoolConstant *e) {
	return ConstantInt::get(context->getLLVMContext(), APInt(1, e->getVal()));
}

Value* ExpressionGeneratorVisitor::visit(expressions::StringConstant *e) {
	throw runtime_error(
			string("support for string constants not implemented yet"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::InputArgument *e) {
	const std::map<std::string, AllocaInst*>& activeVars = currState.getBindings();

	//	for (std::map<std::string, AllocaInst*>::const_iterator it =
	//			activeVars.begin(); it != activeVars.end(); it++) {
	//		LOG(INFO) << "[Input Argument: ] Binding " << it->first;
	//	}

	std::map<std::string, AllocaInst*>::const_iterator it;
	it = activeVars.find(e->getArgName());
	if (it == activeVars.end()) {
		throw runtime_error(string("Unknown variable name: ") + e->getArgName());
	}
	AllocaInst* argMem = it->second;
	IRBuilder<>* const TheBuilder = context->getBuilder();
	return TheBuilder->CreateLoad(argMem);
}

/**
 * FIXME still too many cases to check with ifs / too repetitive
 * Try to simplify
 */
Value* ExpressionGeneratorVisitor::visit(expressions::EqExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpEQ(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpOEQ(left, right);
		case BOOL:
			return TheBuilder->CreateICmpEQ(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::NeExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpNE(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpONE(left, right);
		case BOOL:
			return TheBuilder->CreateICmpNE(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::GeExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpSGE(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpOGE(left, right);
		case BOOL:
			return TheBuilder->CreateICmpSGE(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::GtExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpSGT(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpOGT(left, right);
		case BOOL:
			return TheBuilder->CreateICmpSGT(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::LeExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpSLE(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpOLE(left, right);
		case BOOL:
			return TheBuilder->CreateICmpSLE(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::LtExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateICmpSLT(left, right);
		case FLOAT:
			return TheBuilder->CreateFCmpOLT(left, right);
		case BOOL:
			return TheBuilder->CreateICmpSLT(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::AddExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateAdd(left, right);
		case FLOAT:
			return TheBuilder->CreateFAdd(left, right);
		case BOOL:
			return TheBuilder->CreateAdd(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::SubExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateSub(left, right);
		case FLOAT:
			return TheBuilder->CreateFSub(left, right);
		case BOOL:
			return TheBuilder->CreateSub(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::MultExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateMul(left, right);
		case FLOAT:
			return TheBuilder->CreateFMul(left, right);
		case BOOL:
			return TheBuilder->CreateMul(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

Value* ExpressionGeneratorVisitor::visit(expressions::DivExpression *e) {
	IRBuilder<>* const TheBuilder = context->getBuilder();
	Value* left = e->getLeftOperand()->accept(*this);
	Value* right = e->getRightOperand()->accept(*this);

	ExpressionType* childType = e->getLeftOperand()->getExpressionType();
	if (childType->isPrimitive()) {
		typeID id = childType->getTypeID();

		switch (id) {
		case INT:
			return TheBuilder->CreateSDiv(left, right);
		case FLOAT:
			return TheBuilder->CreateFDiv(left, right);
		case BOOL:
			return TheBuilder->CreateSDiv(left, right);
		case STRING:
			LOG(ERROR)<< "[ExpressionGeneratorVisitor]: string operations not supported yet";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: string operations not supported yet"));
		case COLLECTION:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		case RECORD:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: invalid expression type";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: invalid expression type"));
		default:
			LOG(ERROR) << "[ExpressionGeneratorVisitor]: Unknown Input";
			throw runtime_error(string("[ExpressionGeneratorVisitor]: Unknown Input"));
		}
	}
	throw runtime_error(string("[ExpressionGeneratorVisitor]: input of binary expression can only be primitive"));
}

