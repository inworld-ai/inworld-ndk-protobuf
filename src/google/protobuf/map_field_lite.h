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

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__

#include <type_traits>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/map.h>
#include <google/protobuf/map_entry_lite.h>
#include <google/protobuf/port.h>
#include <google/protobuf/wire_format_lite.h>

#include <google/protobuf/port_def.inc>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf_inworld {
namespace internal {

// This class provides access to map field using generated api. It is used for
// internal generated message implementation only. Users should never use this
// directly.
template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType key_wire_type,
          WireFormatLite::FieldType value_wire_type>
class MapFieldLite {
  // Define message type for internal repeated field.
  typedef Derived EntryType;

 public:
  typedef Map<Key, T> MapType;
  typedef EntryType EntryTypeTrait;

  constexpr MapFieldLite() {}

  explicit MapFieldLite(Arena* arena) : map_(arena) {}

  // Accessors
  const Map<Key, T>& GetMap() const { return map_; }
  Map<Key, T>* MutableMap() { return &map_; }

  // Convenient methods for generated message implementation.
  int size() const { return static_cast<int>(map_.size()); }
  void Clear() { return map_.clear(); }
  void MergeFrom(const MapFieldLite& other) {
    for (typename Map<Key, T>::const_iterator it = other.map_.begin();
         it != other.map_.end(); ++it) {
      map_[it->first] = it->second;
    }
  }
  void Swap(MapFieldLite* other) { map_.swap(other->map_); }

  // Used in the implementation of parsing. Caller should take the ownership iff
  // arena_ is NULL.
  EntryType* NewEntry() const {
    return Arena::CreateMessage<EntryType>(map_.arena());
  }
  // Used in the implementation of serializing enum value type. Caller should
  // take the ownership iff arena_ is NULL.
  EntryType* NewEnumEntryWrapper(const Key& key, const T t) const {
    return EntryType::EnumWrap(key, t, map_.arena_);
  }
  // Used in the implementation of serializing other value types. Caller should
  // take the ownership iff arena_ is NULL.
  EntryType* NewEntryWrapper(const Key& key, const T& t) const {
    return EntryType::Wrap(key, t, map_.arena_);
  }

  const char* _InternalParse(const char* ptr, ParseContext* ctx) {
    typename Derived::template Parser<MapFieldLite, Map<Key, T>> parser(this);
    return parser._InternalParse(ptr, ctx);
  }

  template <typename UnknownType>
  const char* ParseWithEnumValidation(const char* ptr, ParseContext* ctx,
                                      bool (*is_valid)(int), uint32 field_num,
                                      InternalMetadata* metadata) {
    typename Derived::template Parser<MapFieldLite, Map<Key, T>> parser(this);
    return parser.template ParseWithEnumValidation<UnknownType>(
        ptr, ctx, is_valid, field_num, metadata);
  }

 private:
  typedef void DestructorSkippable_;

  Map<Key, T> map_;

  friend class ::PROTOBUF_NAMESPACE_ID::Arena;
};

template <typename UnknownType, typename T>
struct EnumParseWrapper {
  const char* _InternalParse(const char* ptr, ParseContext* ctx) {
    return map_field->template ParseWithEnumValidation<UnknownType>(
        ptr, ctx, is_valid, field_num, metadata);
  }
  T* map_field;
  bool (*is_valid)(int);
  uint32 field_num;
  InternalMetadata* metadata;
};

// Helper function because the typenames of maps are horrendous to print. This
// leverages compiler type deduction, to keep all type data out of the
// generated code
template <typename UnknownType, typename T>
EnumParseWrapper<UnknownType, T> InitEnumParseWrapper(
    T* map_field, bool (*is_valid)(int), uint32 field_num,
    InternalMetadata* metadata) {
  return EnumParseWrapper<UnknownType, T>{map_field, is_valid, field_num,
                                          metadata};
}

// True if IsInitialized() is true for value field in all elements of t. T is
// expected to be message.  It's useful to have this helper here to keep the
// protobuf compiler from ever having to emit loops in IsInitialized() methods.
// We want the C++ compiler to inline this or not as it sees fit.
template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType key_wire_type,
          WireFormatLite::FieldType value_wire_type>
bool AllAreInitialized(const MapFieldLite<Derived, Key, T, key_wire_type,
                                          value_wire_type>& field) {
  const auto& t = field.GetMap();
  for (typename Map<Key, T>::const_iterator it = t.begin(); it != t.end();
       ++it) {
    if (!it->second.IsInitialized()) return false;
  }
  return true;
}

template <typename MEntry>
struct MapEntryToMapField : MapEntryToMapField<typename MEntry::SuperType> {};

template <typename T, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
struct MapEntryToMapField<
    MapEntryLite<T, Key, Value, kKeyFieldType, kValueFieldType>> {
  typedef MapFieldLite<
      MapEntryLite<T, Key, Value, kKeyFieldType, kValueFieldType>, Key, Value,
      kKeyFieldType, kValueFieldType>
      MapFieldType;
};

}  // namespace internal
}  // namespace protobuf_inworld
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_LITE_H__
