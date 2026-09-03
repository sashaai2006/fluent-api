#pragma once

#include "ast.hpp"
#include "combinators/compile.hpp"
#include "combinators/compile/runtime.hpp"
#include "flow.hpp"

#include <exprflow/graph/graph.hpp>

#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace exprflow {

class Compiled {
 private:
  std::shared_ptr<const TaskGraph> graph_;
  std::vector<TaskGraph::NodeId> outputs_;

 public:
  Compiled(std::shared_ptr<TaskGraph> graph,
           std::vector<TaskGraph::NodeId> outputs)
      : graph_(std::move(graph)), outputs_(std::move(outputs)) {}

  auto Graph() const -> const TaskGraph& { return *graph_; }
  auto Share() const -> std::shared_ptr<const TaskGraph> { return graph_; }
  auto Outputs() const -> std::span<const TaskGraph::NodeId> {
    return outputs_;
  }
};

namespace detail {

class Compiler {
  std::unique_ptr<TaskGraph> graph_ = std::make_unique<TaskGraph>();

 public:
  template <typename T>
  T* Save(T value) {
    return graph_->Save(std::move(value));
  }

  template <typename Task>
  auto AddWired(const std::vector<TaskGraph::NodeId>* ids, Task task)
      -> TaskGraph::NodeId {
    const auto id = graph_->AddNode(std::move(task));
    for (const auto in : *ids) {
      graph_->AddEdge(in, id);
    }
    return id;
  }

  template <typename Prev, typename F>
  auto EmitInvoke(const std::vector<TaskGraph::NodeId>* ids, F fn)
      -> TaskGraph::NodeId {
    F* stored = Save(std::move(fn));
    return AddWired(ids, InvokeTask<Prev, F>{stored, ids});
  }

  template <typename T>
  auto EmitLeaf(const T& value) -> TaskGraph::NodeId {
    return graph_->AddNode(LeafTask<T>{Save(value)});
  }

  template <typename Node>
  auto Emit(const Node& node) -> std::vector<TaskGraph::NodeId> {
    return EmitCombinator(*this, node);
  }

  auto Finish() -> std::shared_ptr<TaskGraph> {
    graph_->Seal();
    return std::shared_ptr<TaskGraph>(std::move(graph_));
  }
};

}  // namespace detail

template <typename Node>
Compiled Compile(const Node& ast) {
  detail::Compiler compiler;
  auto outputs = compiler.Emit(ast);
  return Compiled{compiler.Finish(), std::move(outputs)};
}

template <typename Node>
Compiled Compile(const Flow<Node>& flow) {
  return Compile(flow.Ast());
}

}  // namespace exprflow
