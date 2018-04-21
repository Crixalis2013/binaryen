/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Souperify - convert to Souper IR in text form.
//
// This needs 'flatten' to be run before it, as it assumes the IR is in
// flat form.
//
// See https://github.com/google/souper/issues/323
//

#include <string>

#include "wasm.h"
#include "pass.h"
#include "ir/find_all.h"
#include "ir/literal-utils.h"

namespace wasm {

// Simple IR for data flow computation for Souper.
// TODO: loops
// TODO: generalize for other things?

namespace DataFlow {

struct Node {
  enum Type {
    Var,   // an unknown variable number (not to be confused with var/param/local in wasm)
    Set,   // a register, defined by a SetLocal
    Const, // a constant value
    Phi,   // a phi from converging control flow
    Block, // a source of phis
    Bad    // something we can't handle and should ignore
  } type;

  Node(Type type) : type(type) {}

  template<Type expected>
  bool is() { return type == expected; }

  union {
    // For Var
    Index varIndex;
    // For Set
    SetLocal* set;
    // For Const
    Literal value;
    // For Phi
    struct {
      Node* block;
      std::vector<Node*> values;
    } phi;
    // For Block
    Index blockSize;
  };

  // Constructors
  static Node* makeVar(Index varIndex) {
    Node* ret = new Node(Var);
    ret->varIndex = varIndex;
    return ret;
  }
  static Node* makeSet(SetLocal* set) {
    Node* ret = new Node(Set);
    ret->set = set;
    return ret;
  }
  static Node* makeConst(Literal value) {
    Node* ret = new Node(Const);
    ret->value = value;
    return ret;
  }
  static Node* makePhi(Node* block) {
    Node* ret = new Node(Phi);
    ret->phi.block = block;
    return ret;
  }
  static Node* makeBlock(Index blockSize) {
    Node* ret = new Node(Block);
    ret->blockSize = blockSize;
    return ret;
  }
  static Node* makeBad() {
    Node* ret = new Node(Bad);
    return ret;
  }

  // helpers
  void addPhiValue(Node* value) {
    assert(type == Phi);
    phi.values.push_back(value);
  }
};

/*
// A printable trace of IR.
struct Trace : public Visitor<Trace> {
  // Each Node in a trace has an index, from 0.
  std::unordered_map<Node, Index> indexing;

  void visitBlock(Block* curr) {
    // TODO: handle super-deep nesting
    // TODO: handle breaks to here
    for (auto* child : curr->list) {
      visit(child);
    }
  }
  void visitIf(If* curr) {
    // TODO: emit a path condition for the if-then, if it's a local
    // (otherwise, it's a constant, and not interesting)
    //if (auto* get = curr->condition->dynCast<GetLocal>()) {
    //  std::cout << "pc " << emitLocal(get->index) << " 1\n";
    //}
    // TODO: don't generate a blockIndex if we don't need one?
    // TODO: blockpc
    auto blockIndex = nextIndex++;
    std::string ret = "%" + std::to_string(blockIndex) + " = block 2\n";
    auto initialState = localState;
    ret += visit(curr->ifTrue);
    auto afterIfTrueState = localState;
    if (curr->ifFalse) {
      localState = initialState;
      ret += visit(curr->ifFalse);
      auto afterIfFalseState = localState; // TODO: optimize
      ret += merge(afterIfTrueState, afterIfFalseState, blockIndex, localState);
    } else {
      ret += merge(initialState, afterIfTrueState, blockIndex, localState);
    }
    return ret;
  }
  void visitGetLocal(GetLocal* curr) {
  }
  void visitSetLocal(SetLocal* curr) {
  }
  void visitConst(Const* curr) {
    std::string ret;
    if (isIntegerType(curr->type)) {
      ret = curr->value.getInteger();
    } else {
      ret = curr->value.getFloat();
    }
    ret += std::string(":") + printType(curr->type);
    return ret;
  }
  void visitBinary(Binary *curr) {
    std::string ret;
    switch (curr->op) {
      case AddInt32:
      case AddInt64:  ret = "add";  break;
      case SubInt32:
      case SubInt64:  ret = "sub";  break;
      case MulInt32:
      case MulInt64:  ret = "mul";  break;
      case DivSInt32:
      case DivSInt64: ret = "sdiv"; break;
      case DivUInt32:
      case DivUInt64: ret = "udiv"; break;
      case RemSInt32:
      case RemSInt64: ret = "srem"; break;
      case RemUInt32:
      case RemUInt64: ret = "urem"; break;
      case AndInt32:
      case AndInt64:  ret = "and";  break;
      case OrInt32:
      case OrInt64:   ret = "or";   break;
      case XorInt32:
      case XorInt64:  ret = "xor";  break;
      case ShlInt32:
      case ShlInt64:  ret = "shl";  break;
      case ShrUInt32:
      case ShrUInt64: ret = "ushr"; break;
      case ShrSInt32:
      case ShrSInt64: ret = "sshr"; break;
      case RotLInt32:
      case RotLInt64: ret = "rotl"; break;
      case RotRInt32:
      case RotRInt64: ret = "rotr"; break;
      case EqInt32:
      case EqInt64:   ret = "eq";   break;
      case NeInt32:
      case NeInt64:   ret = "ne";   break;
      case LtSInt32:
      case LtSInt64:  ret = "slt";  break;
      case LtUInt32:
      case LtUInt64:  ret = "ult";  break;
      case LeSInt32:
      case LeSInt64:  ret = "sle";  break;
      case LeUInt32:
      case LeUInt64:  ret = "ule";  break;
      case GtSInt32:
      case GtSInt64:  ret = "sgt";  break;
      case GtUInt32:
      case GtUInt64:  ret = "ugt";  break;
      case GeSInt32:
      case GeSInt64:  ret = "sge";  break;
      case GeUInt32:
      case GeUInt64:  ret = "uge";  break;

      default: WASM_UNREACHABLE();
    }
    ret += ' ';
    ret += visit(curr->left);
    ret += ", ";
    ret += visit(curr->right);
    return ret;
  }
  void visitReturn(Return* curr) {
    // TODO something here?
    return "; return";
  }
  void visitNop(Nop* curr) {
    return "";
  }
  void visitUnreachable(Unreachable* curr) {
    return "; unreachable";
  }

  std::string printPhi(const LocalState& aState, const LocalState& bState, Index blockIndex, LocalState& out) {
    assert(out.size() == func->getNumLocals());
    std::string ret;
    for (Index i = 0; i < func->getNumLocals(); i++) {
      auto a = aState[i];
      auto b = bState[i];
      if (a == b) {
        out[i] = a;
      } else {
        // We need to actually merge some stuff.
        auto phi = nextIndex++;
        out[i] = phi;
        ret += "%" + std::to_string(phi) + " = phi %" + std::to_string(blockIndex) + ", ";
        auto type = func->getLocalType(i);
        emitLocal(a, type);
        ret += ", ";
        emitLocal(b, type);
      }
    }
    return ret;
  }
};
*/

} // namespace DataFlow

// Main logic to generate IR for a function. This is implemented as a
// visitor on the wasm, where visitors return a bool whether the node is
// supported (false === bad).
struct SouperifyFunction : public Visitor<SouperifyFunction, bool> {
  // Tracks the state of locals in a control flow path:
  //   localState[i] = the node whose value it contains
  typedef std::vector<DataFlow::Node*> LocalState;

  // The current local state in the control flow path being emitted.
  LocalState localState;

  // Connects a specific get to the data flowing into it.
  std::unordered_map<GetLocal*, DataFlow::Node*> getNodes;

  // The function being processed.
  Function* func;

  // All of our nodes
  std::vector<std::unique_ptr<DataFlow::Node>> nodes;

  // Check if a function is relevant for us.
  static bool check(Function* func) {
    // TODO handle loops. for now, just ignore the entire function
    if (!FindAll<Loop>(func->body).list.empty()) {
      return false;
    }
    return true;
  }

  // Main entry: processes a function
  void process(Function* funcInit) {
    func = funcInit;
    std::cout << "; function: " << func->name << '\n';
    // Generate the data-flow IR nodes first.
    // Set up initial local state IR.
    localState.resize(func->getNumLocals());
    for (Index i = 0; i < func->getNumLocals(); i++) {
      DataFlow::Node* node;
      if (func->isParam(i)) {
        node = DataFlow::Node::makeVar(i);
      } else {
        node = DataFlow::Node::makeConst(LiteralUtils::makeZero(func->getLocalType(i))));
      }
      addNode(node, index);
    }
    // Process the function body, generating the rest of the IR.
    visit(func->body);
    // TODO: handle value flowing out of body
  }

  // Add a new node to our list of owned nodes.
  DataFlow::Node* addNode(DataFlow::Node* node) {
    nodes.push_back(node);
    return node;
  }

  // Add a new node, for a specific local index.
  DataFlow::Node* addNode(DataFlow::Node* node, Index index) {
    localState[index] = node;
    return addNode(node);
  }

  // Merge local state for multiple control flow paths
  // TODO: more than 2
  void merge(const LocalState& aState, const LocalState& bState, Index blockIndex, LocalState& out) {
    assert(out.size() == func->getNumLocals());
    // create a block only if necessary
    DataFlow::Node* block = nullptr;
    for (Index i = 0; i < func->getNumLocals(); i++) {
      auto a = aState[i];
      auto b = bState[i];
      if (a == b) {
        out[i] = a;
      } else {
        // We need to actually merge some stuff.
        if (!block) {
          block = addNode(DataFlow::Node::makeBlock(2));
        }
        auto* phi = addNode(DataFlow::Node::makePhi(block));
        phi->addPhiValue(a);
        phi->addPhiValue(b);
        out[i] = phi;
      }
    }
  }

  // Visitors.

  bool visitBlock(Block* curr) {
    // TODO: handle super-deep nesting
    // TODO: handle breaks to here
    for (auto* child : curr->list) {
      visit(child);
    }
    return true;
  }
  bool visitIf(If* curr) {
    // TODO: blockpc
    auto initialState = localState;
    visit(curr->ifTrue);
    auto afterIfTrueState = localState;
    if (curr->ifFalse) {
      localState = initialState;
      visit(curr->ifFalse);
      auto afterIfFalseState = localState; // TODO: optimize
      merge(afterIfTrueState, afterIfFalseState, blockIndex, localState);
    } else {
      merge(initialState, afterIfTrueState, blockIndex, localState);
    }
    return true;
  }
  bool visitLoop(Loop* curr) { return false; }
  bool visitBreak(Break* curr) { return false; }
  bool visitSwitch(Switch* curr) { return false; }
  bool visitCall(Call* curr) { return false; }
  bool visitCallImport(CallImport* curr) { return false; }
  bool visitCallIndirect(CallIndirect* curr) { return false; }
  bool visitGetLocal(GetLocal* curr) {
    // We now know which IR node this get refers to
    auto* node = localState[curr->index]
    getNodes[curr] = node;
    return !node->is<Bad>();
  }
  bool visitSetLocal(SetLocal* curr) {
    // If we are doing a copy, just do the copy.
    if (auto* get = curr->value->dynCast<GetLocal>()) {
      localState[curr->index] = localState[get->index];
    }
    // Make a new IR node for the new value here.
    if (visit(curr->value)) {
      addNode(DataFlow::Node::makeSet(curr), curr->index);
      return true;
    } else {
      addNode(DataFlow::Node::makeBad(), curr->index);
      return false;
    }
  }
  bool visitGetGlobal(GetGlobal* curr) {
    return false;
  }
  bool visitSetGlobal(SetGlobal* curr) {
    return false;
  }
  bool visitLoad(Load* curr) {
    return false;
  }
  bool visitStore(Store* curr) {
    return false;
  }
  bool visitAtomicRMW(AtomicRMW* curr) {
    return false;
  }
  bool visitAtomicCmpxchg(AtomicCmpxchg* curr) {
    return false;
  }
  bool visitAtomicWait(AtomicWait* curr) {
    return false;
  }
  bool visitAtomicWake(AtomicWake* curr) {
    return false;
  }
  bool visitConst(Const* curr) {
    return true;
  }
  bool visitUnary(Unary* curr) {
    return false;
  }
  bool visitBinary(Binary *curr) {
    return visit(curr->left) && visit(curr->right);
  }
  bool visitSelect(Select* curr) {
    return false;
  }
  bool visitDrop(Drop* curr) {
    return false;
  }
  bool visitReturn(Return* curr) {
    // note we don't need the value (it's a const or a get as we are flattened)
    return true;
  }
  bool visitHost(Host* curr) {
    return false;
  }
  bool visitNop(Nop* curr) {
    return true;
  }
  bool visitUnreachable(Unreachable* curr) {
    return false;
  }
};

struct Souperify : public WalkerPass<PostWalker<Souperify>> {
  // Not parallel, for now - could parallelize and combine outputs at the end.
  // If Souper is thread-safe, we could also run it in parallel.

  void doWalkFunction(Function* func) {
    if (SouperifyFunction::check(func)) {
      SouperifyFunction().process(func);
    }
  }
};

Pass *createSouperifyPass() {
  return new Souperify();
}

} // namespace wasm

