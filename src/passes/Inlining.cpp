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
// Inlining.
//
// For now, this does a conservative inlining of all functions that have
// exactly one use. That should not increase code size, and may have
// speed benefits.
//

#include <wasm.h>
#include <pass.h>
#include <wasm-builder.h>
#include <parsing.h>
#include <ast_utils.h>

namespace wasm {

struct FunctionUseCounter : public WalkerPass<PostWalker<FunctionUseCounter, Visitor<FunctionUseCounter>>> {
  bool isFunctionParallel() override { return true; }

  FunctionUseCounter(std::map<Name, Index>* output) : output(output) {}

  FunctionUseCounter* create() override {
    return new FunctionUseCounter(output);
  }

  void visitCall(Call *curr) {
    (*output)[curr->target]++;
  }

private:
  std::map<Name, Index>* output;
};

struct Action {
  Call* call;
  Block* block; // the replacement for the call, into which we should inline
  Function* contents;

  Action(Call* call, Block* block, Function* contents) : call(call), block(block), contents(contents) {}
};

struct InliningState {
  std::set<Name> canInline;
  std::map<Name, std::vector<Action>> actionsForFunction; // function name => actions that can be performed in it
};

struct Planner : public WalkerPass<PostWalker<Planner, Visitor<Planner>>> {
  bool isFunctionParallel() override { return true; }

  Planner(InliningState* state) : state(state) {}

  Planner* create() override {
    return new Planner(state);
  }

  void visitCall(Call *curr) {
    if (state->canInline.count(curr->target)) {
      auto* block = Builder(*getModule()).makeBlock();
      block->type = curr->type;
      replaceCurrent(block);
      state->actionsForFunction[getFunction()->name].emplace_back(curr, block, getModule()->getFunction(curr->target));
    }
  }

  void doWalkFunction(Function* func) {
    // we shouldn't inline into us if we are to be inlined
    // ourselves - that has the risk of cycles
    if (state->canInline.count(func->name) == 0) {
      walk(func->body);
    }
  }

private:
  InliningState* state;
};

// Core inlining logic. Modifies the outside function (adding locals as
// needed), and returns the inlined code.
static Expression* doInlining(Module* module, Function* into, Action& action) {
  Builder builder(*module);
  auto* block = action.block;
  block->name = Name(std::string("__inlined_func$") + action.contents->name.str);
  block->type = action.contents->result;
  // set up a locals mapping
  struct Updater : public PostWalker<Updater, Visitor<Updater>> {
    std::map<Index, Index> localMapping;
    Name returnName;
    Builder* builder;

    void visitReturn(Return* curr) {
      replaceCurrent(builder->makeBreak(returnName, curr->value));
    }
    void visitGetLocal(GetLocal* curr) {
      curr->index = localMapping[curr->index];
    }
    void visitSetLocal(SetLocal* curr) {
      curr->index = localMapping[curr->index];
    }
  } updater;
  updater.returnName = block->name;
  updater.builder = &builder;
  for (Index i = 0; i < action.contents->getNumLocals(); i++) {
    updater.localMapping[i] = builder.addVar(into, action.contents->getLocalType(i));
  }
  // assign the operands into the params
  for (Index i = 0; i < action.contents->params.size(); i++) {
    block->list.push_back(builder.makeSetLocal(updater.localMapping[i], action.call->operands[i]));
  }
  // update the inlined contents
  auto* contents = ExpressionManipulator::copy(action.contents->body, *module);
  updater.walk(contents);
  block->list.push_back(contents);
  return block;
}

struct Inlining : public Pass {
  void run(PassRunner* runner, Module* module) override {
    // keep going while we inline, to handle nesting. TODO: optimize
    while (iteration(runner, module)) {}
  }

  bool iteration(PassRunner* runner, Module* module) {
    // decide which to inline
    InliningState state;
    for (auto& func : module->functions) {
      if (func->name == I32S_REM || func->name == I32S_DIV ||
          func->name == I32U_REM || func->name == I32U_DIV) {
        state.canInline.insert(func->name);
      }
    }
    // fill in actionsForFunction, as we operate on it in parallel (each function to its own entry)
    for (auto& func : module->functions) {
      state.actionsForFunction[func->name];
    }
    // find and plan inlinings
    {
      PassRunner runner(module);
      runner.add<Planner>(&state);
      runner.run();
    }
    // perform inlinings
    std::set<Name> inlined;
    std::set<Function*> inlinedInto;
    for (auto& func : module->functions) {
      for (auto& action : state.actionsForFunction[func->name]) {
        doInlining(module, func.get(), action);
        inlined.insert(action.contents->name);
        inlinedInto.insert(func.get());
      }
    }
    // anything we inlined into may now have non-unique label names, fix it up
    for (auto func : inlinedInto) {
      wasm::UniqueNameMapper::uniquify(func->body);
    }
    // remove functions that we managed to inline, their one use is gone
    auto& funcs = module->functions;
    funcs.erase(std::remove_if(funcs.begin(), funcs.end(), [&inlined](const std::unique_ptr<Function>& curr) {
      return inlined.count(curr->name) > 0;
    }), funcs.end());
    // return whether we did any work
    return inlined.size() > 0;
  }
};

Pass *createInliningPass() {
  return new Inlining();
}

} // namespace wasm

