// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/compiler/rust/naming.h"

#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf_inworld {
namespace compiler {
namespace rust {
namespace {
std::string GetUnderscoreDelimitedFullName(Context<Descriptor> msg) {
  std::string result = msg.desc().full_name();
  absl::StrReplaceAll({{".", "_"}}, &result);
  return result;
}
}  // namespace

std::string GetCrateName(Context<FileDescriptor> dep) {
  absl::string_view path = dep.desc().name();
  auto basename = path.substr(path.rfind('/') + 1);
  return absl::StrReplaceAll(basename, {{".", "_"}, {"-", "_"}});
}

std::string GetRsFile(Context<FileDescriptor> file) {
  auto basename = StripProto(file.desc().name());
  switch (auto k = file.opts().kernel) {
    case Kernel::kUpb:
      return absl::StrCat(basename, ".u.pb.rs");
    case Kernel::kCpp:
      return absl::StrCat(basename, ".c.pb.rs");
    default:
      ABSL_LOG(FATAL) << "Unknown kernel type: " << static_cast<int>(k);
      return "";
  }
}

std::string GetThunkCcFile(Context<FileDescriptor> file) {
  auto basename = StripProto(file.desc().name());
  return absl::StrCat(basename, ".pb.thunks.cc");
}

std::string GetHeaderFile(Context<FileDescriptor> file) {
  auto basename = StripProto(file.desc().name());
  return absl::StrCat(basename, ".proto.h");
}

namespace {

template <typename T>
std::string Thunk(Context<T> field, absl::string_view op) {
  // NOTE: When field.is_upb(), this functions outputs must match the symbols
  // that the upbc plugin generates exactly. Failure to do so correctly results
  // in a link-time failure.
  absl::string_view prefix = field.is_cpp() ? "__rust_proto_thunk__" : "";
  std::string thunk =
      absl::StrCat(prefix, GetUnderscoreDelimitedFullName(
                               field.WithDesc(field.desc().containing_type())));

  absl::string_view format;
  if (field.is_upb() && op == "get") {
    // upb getter is simply the field name (no "get" in the name).
    format = "_$1";
  } else if (field.is_upb() && op == "case") {
    // upb oneof case function is x_case compared to has/set/clear which are in
    // the other order e.g. clear_x.
    format = "_$1_$0";
  } else {
    format = "_$0_$1";
  }

  absl::SubstituteAndAppend(&thunk, format, op, field.desc().name());
  return thunk;
}

}  // namespace

std::string Thunk(Context<FieldDescriptor> field, absl::string_view op) {
  return Thunk<FieldDescriptor>(field, op);
}

std::string Thunk(Context<OneofDescriptor> field, absl::string_view op) {
  return Thunk<OneofDescriptor>(field, op);
}

std::string Thunk(Context<Descriptor> msg, absl::string_view op) {
  absl::string_view prefix = msg.is_cpp() ? "__rust_proto_thunk__" : "";
  return absl::StrCat(prefix, GetUnderscoreDelimitedFullName(msg), "_", op);
}

std::string PrimitiveRsTypeName(const FieldDescriptor& desc) {
  switch (desc.type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return "i32";
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return "i64";
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
      return "u32";
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
      return "u64";
    case FieldDescriptor::TYPE_FLOAT:
      return "f32";
    case FieldDescriptor::TYPE_DOUBLE:
      return "f64";
    case FieldDescriptor::TYPE_BYTES:
      return "[u8]";
    case FieldDescriptor::TYPE_STRING:
      return "::__pb::ProtoStr";
    default:
      break;
  }
  ABSL_LOG(FATAL) << "Unsupported field type: " << desc.type_name();
  return "";
}

std::string RustModule(Context<Descriptor> msg) {
  absl::string_view package = msg.desc().file()->package();
  if (package.empty()) return "";
  return absl::StrCat("", absl::StrReplaceAll(package, {{".", "::"}}));
}

std::string RustInternalModuleName(Context<FileDescriptor> file) {
  // TODO(b/291853557): Introduce a more robust mangling here to avoid conflicts
  // between `foo/bar/baz.proto` and `foo_bar/baz.proto`.
  return absl::StrReplaceAll(StripProto(file.desc().name()), {{"/", "_"}});
}

std::string GetCrateRelativeQualifiedPath(Context<Descriptor> msg) {
  if (msg.desc().file()->package().empty()) {
    return msg.desc().name();
  }
  return absl::StrCat(RustModule(msg), "::", msg.desc().name());
}

std::string FieldInfoComment(Context<FieldDescriptor> field) {
  absl::string_view label =
      field.desc().is_repeated() ? "repeated" : "optional";
  std::string comment =
      absl::StrCat(field.desc().name(), ": ", label, " ",
                   FieldDescriptor::TypeName(field.desc().type()));

  if (auto* m = field.desc().message_type()) {
    absl::StrAppend(&comment, " ", m->full_name());
  }
  if (auto* m = field.desc().enum_type()) {
    absl::StrAppend(&comment, " ", m->full_name());
  }

  return comment;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf_inworld
}  // namespace google
