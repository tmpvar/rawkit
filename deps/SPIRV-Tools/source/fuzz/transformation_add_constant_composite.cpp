// Copyright (c) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "source/fuzz/transformation_add_constant_composite.h"

#include <vector>

#include "source/fuzz/fuzzer_util.h"

namespace spvtools {
namespace fuzz {

TransformationAddConstantComposite::TransformationAddConstantComposite(
    const spvtools::fuzz::protobufs::TransformationAddConstantComposite&
        message)
    : message_(message) {}

TransformationAddConstantComposite::TransformationAddConstantComposite(
    uint32_t fresh_id, uint32_t type_id,
    const std::vector<uint32_t>& constituent_ids, bool is_irrelevant) {
  message_.set_fresh_id(fresh_id);
  message_.set_type_id(type_id);
  message_.set_is_irrelevant(is_irrelevant);
  for (auto constituent_id : constituent_ids) {
    message_.add_constituent_id(constituent_id);
  }
}

bool TransformationAddConstantComposite::IsApplicable(
    opt::IRContext* ir_context, const TransformationContext& /*unused*/) const {
  // Check that the given id is fresh.
  if (!fuzzerutil::IsFreshId(ir_context, message_.fresh_id())) {
    return false;
  }
  // Check that the composite type id is an instruction id.
  auto composite_type_instruction =
      ir_context->get_def_use_mgr()->GetDef(message_.type_id());
  if (!composite_type_instruction) {
    return false;
  }
  // Gather up the operands for the composite constant, in the process checking
  // whether the given type really defines a composite.
  std::vector<uint32_t> constituent_type_ids;
  switch (composite_type_instruction->opcode()) {
    case SpvOpTypeArray:
      for (uint32_t index = 0;
           index <
           fuzzerutil::GetArraySize(*composite_type_instruction, ir_context);
           index++) {
        constituent_type_ids.push_back(
            composite_type_instruction->GetSingleWordInOperand(0));
      }
      break;
    case SpvOpTypeMatrix:
    case SpvOpTypeVector:
      for (uint32_t index = 0;
           index < composite_type_instruction->GetSingleWordInOperand(1);
           index++) {
        constituent_type_ids.push_back(
            composite_type_instruction->GetSingleWordInOperand(0));
      }
      break;
    case SpvOpTypeStruct:
      composite_type_instruction->ForEachInOperand(
          [&constituent_type_ids](const uint32_t* member_type_id) {
            constituent_type_ids.push_back(*member_type_id);
          });
      break;
    default:
      // Not a composite type.
      return false;
  }

  // Check that the number of provided operands matches the number of
  // constituents required by the type.
  if (constituent_type_ids.size() !=
      static_cast<uint32_t>(message_.constituent_id().size())) {
    return false;
  }

  // Check that every provided operand refers to an instruction of the
  // corresponding constituent type.
  for (uint32_t index = 0; index < constituent_type_ids.size(); index++) {
    auto constituent_instruction =
        ir_context->get_def_use_mgr()->GetDef(message_.constituent_id(index));
    if (!constituent_instruction) {
      return false;
    }
    if (constituent_instruction->type_id() != constituent_type_ids.at(index)) {
      return false;
    }
  }
  return true;
}

void TransformationAddConstantComposite::Apply(
    opt::IRContext* ir_context,
    TransformationContext* transformation_context) const {
  opt::Instruction::OperandList in_operands;
  for (auto constituent_id : message_.constituent_id()) {
    in_operands.push_back({SPV_OPERAND_TYPE_ID, {constituent_id}});
  }
  ir_context->module()->AddGlobalValue(MakeUnique<opt::Instruction>(
      ir_context, SpvOpConstantComposite, message_.type_id(),
      message_.fresh_id(), in_operands));
  fuzzerutil::UpdateModuleIdBound(ir_context, message_.fresh_id());
  // We have added an instruction to the module, so need to be careful about the
  // validity of existing analyses.
  ir_context->InvalidateAnalysesExceptFor(
      opt::IRContext::Analysis::kAnalysisNone);

  if (message_.is_irrelevant()) {
    transformation_context->GetFactManager()->AddFactIdIsIrrelevant(
        message_.fresh_id());
  }
}

protobufs::Transformation TransformationAddConstantComposite::ToMessage()
    const {
  protobufs::Transformation result;
  *result.mutable_add_constant_composite() = message_;
  return result;
}

}  // namespace fuzz
}  // namespace spvtools
