#ifndef CONTEXT_H
#define CONTEXT_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "symbol.hpp"

static llvm::LLVMContext llvm_context;

namespace spc {
struct TypeNode;  // 前置声明，因为类型信息需要类型节点 但类型节点隶属与AST，直接include会导致循环引用
///  代码生成的上下文环境 聚合了LLVM代码生成要用到的一些东西以及优化标志，符号表等
struct CodegenContext final {
 public:
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::legacy::FunctionPassManager> fpm;
  std::unique_ptr<llvm::legacy::PassManager> mpm;
  SymbolTable symbolTable;
  bool is_subroutine = false;

  CodegenContext(std::string module_id, bool optimization)
      : builder(llvm_context), module(std::make_unique<llvm::Module>(module_id, llvm_context)), symbolTable(this) {
    if (optimization) {
      // 添加常用优化
      fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
      fpm->add(llvm::createPromoteMemoryToRegisterPass());  // 添加内存-寄存器优化 因为LLVM IR的SSA特性 这个一定要加
      fpm->add(llvm::createInstructionCombiningPass());  // 添加指令合并优化
      fpm->add(llvm::createReassociatePass());           // 4 + (x + 5)  ->  x + (4 + 5) 这个叫什么优化...
      fpm->add(llvm::createGVNPass());                   // 消除冗余代码
      fpm->add(llvm::createCFGSimplificationPass());  // 合并基本块 移除不可达部分 基本上也是消除冗余代码用的
      fpm->doInitialization();
      mpm = std::make_unique<llvm::legacy::PassManager>();
      mpm->add(llvm::createConstantMergePass());     // 常亮合并
      mpm->add(llvm::createFunctionInliningPass());  // 函数内联 这个加不加区别不大
    }
  }
};
/// 代码生成异常类，是std::exception的派生类 用来输出在编译时遇到的错误
class CodegenException : public std::exception {
 public:
  explicit CodegenException(const std::string &description) : description(description) {
    // std::cerr<<this->what()<< std::endl;
    // std::exit(-1);
  }

  virtual const char *what() noexcept {
    description = std::string("Codegen error: ") + description;
    return description.c_str();
  }

 private:
  std::string description;
};
};  // namespace spc

#endif
