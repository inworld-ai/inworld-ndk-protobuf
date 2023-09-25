// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#ifdef _WIN32
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/io/io_win32.h"

using google::protobuf_inworld::compiler::CodeGeneratorRequest;
using google::protobuf_inworld::compiler::CodeGeneratorResponse;

// This fake protoc plugin does nothing but write out the CodeGeneratorRequest
// in base64. This is not very useful except that it gives us a way to make
// assertions in tests about the contents of requests that protoc sends to
// plugins.
int main(int argc, char* argv[]) {

#ifdef _WIN32
  google::protobuf_inworld::io::win32::setmode(STDIN_FILENO, _O_BINARY);
  google::protobuf_inworld::io::win32::setmode(STDOUT_FILENO, _O_BINARY);
#endif

  CodeGeneratorRequest request;
  ABSL_CHECK(request.ParseFromFileDescriptor(STDIN_FILENO));
  ABSL_CHECK(!request.file_to_generate().empty());
  CodeGeneratorResponse response;
  response.set_supported_features(
      CodeGeneratorResponse::FEATURE_SUPPORTS_EDITIONS);
  response.add_file()->set_name(
      absl::StrCat(request.file_to_generate(0), ".request"));
  response.mutable_file(0)->set_content(
      absl::Base64Escape(request.SerializeAsString()));
  ABSL_CHECK(response.SerializeToFileDescriptor(STDOUT_FILENO));
  return 0;
}
