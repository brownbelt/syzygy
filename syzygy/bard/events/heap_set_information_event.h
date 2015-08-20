// Copyright 2015 Google Inc. All Rights Reserved.
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
//
// Declares an event to represent a HeapSetInformation function call.
#ifndef SYZYGY_BARD_EVENTS_HEAP_SET_INFORMATION_EVENT_H_
#define SYZYGY_BARD_EVENTS_HEAP_SET_INFORMATION_EVENT_H_

#include "syzygy/bard/events/linked_event.h"

namespace bard {
namespace events {

// An event that wraps a call to HeapSetInformation, to be played against
// a HeapBackdrop.
class HeapSetInformationEvent : public LinkedEvent {
 public:
  HeapSetInformationEvent(HANDLE trace_heap,
                          HEAP_INFORMATION_CLASS info_class,
                          PVOID info,
                          SIZE_T info_length,
                          BOOL trace_succeeded);

  // Event implementation.
  const char* name() const override;

  // @name Accessors.
  // @{
  HANDLE trace_heap() const { return trace_heap_; }
  HEAP_INFORMATION_CLASS info_class() const { return info_class_; }
  PVOID info() const { return info_; }
  SIZE_T info_length() const { return info_length_; }
  BOOL trace_succeeded() const { return trace_succeeded_; }
  // @}

 private:
  // LinkedEvent implementation.
  bool PlayImpl(void* backdrop) override;

  // Arguments to HeapSetInformation.
  HANDLE trace_heap_;
  HEAP_INFORMATION_CLASS info_class_;
  PVOID info_;
  SIZE_T info_length_;

  // Recorded return value.
  BOOL trace_succeeded_;

  DISALLOW_COPY_AND_ASSIGN(HeapSetInformationEvent);
};

}  // namespace events
}  // namespace bard

#endif  // SYZYGY_BARD_EVENTS_HEAP_SET_INFORMATION_EVENT_H_