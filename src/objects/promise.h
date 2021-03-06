// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_PROMISE_H_
#define V8_OBJECTS_PROMISE_H_

#include "src/objects/microtask.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class JSPromise;

// Struct to hold state required for PromiseReactionJob. See the comment on the
// PromiseReaction below for details on how this is being managed to reduce the
// memory and allocation overhead. This is the base class for the concrete
//
//   - PromiseFulfillReactionJobTask
//   - PromiseRejectReactionJobTask
//
// classes, which are used to represent either reactions, and we distinguish
// them by their instance types.
class PromiseReactionJobTask : public Microtask {
 public:
  DECL_ACCESSORS(argument, Object)
  DECL_ACCESSORS2(context, Context)
  DECL_ACCESSORS(handler, HeapObject)
  // [promise_or_capability]: Either a JSPromise (in case of native promises),
  // a PromiseCapability (general case), or undefined (in case of await).
  DECL_ACCESSORS(promise_or_capability, HeapObject)

// Layout description.
#define PROMISE_REACTION_JOB_FIELDS(V)       \
  V(kArgumentOffset, kTaggedSize)            \
  V(kContextOffset, kTaggedSize)             \
  V(kHandlerOffset, kTaggedSize)             \
  V(kPromiseOrCapabilityOffset, kTaggedSize) \
  /* Total size. */                          \
  V(kSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(Microtask::kHeaderSize,
                                PROMISE_REACTION_JOB_FIELDS)
#undef PROMISE_REACTION_JOB_FIELDS

  // Dispatched behavior.
  DECL_CAST(PromiseReactionJobTask)
  DECL_VERIFIER(PromiseReactionJobTask)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseReactionJobTask);
};

// Struct to hold state required for a PromiseReactionJob of type "Fulfill".
class PromiseFulfillReactionJobTask : public PromiseReactionJobTask {
 public:
  // Dispatched behavior.
  DECL_CAST(PromiseFulfillReactionJobTask)
  DECL_PRINTER(PromiseFulfillReactionJobTask)
  DECL_VERIFIER(PromiseFulfillReactionJobTask)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseFulfillReactionJobTask);
};

// Struct to hold state required for a PromiseReactionJob of type "Reject".
class PromiseRejectReactionJobTask : public PromiseReactionJobTask {
 public:
  // Dispatched behavior.
  DECL_CAST(PromiseRejectReactionJobTask)
  DECL_PRINTER(PromiseRejectReactionJobTask)
  DECL_VERIFIER(PromiseRejectReactionJobTask)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseRejectReactionJobTask);
};

// A container struct to hold state required for PromiseResolveThenableJob.
class PromiseResolveThenableJobTask : public Microtask {
 public:
  DECL_ACCESSORS2(context, Context)
  DECL_ACCESSORS2(promise_to_resolve, JSPromise)
  DECL_ACCESSORS2(then, JSReceiver)
  DECL_ACCESSORS2(thenable, JSReceiver)

// Layout description.
#define PROMISE_RESOLVE_THENABLE_JOB_FIELDS(V) \
  V(kContextOffset, kTaggedSize)               \
  V(kPromiseToResolveOffset, kTaggedSize)      \
  V(kThenOffset, kTaggedSize)                  \
  V(kThenableOffset, kTaggedSize)              \
  /* Total size. */                            \
  V(kSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(Microtask::kHeaderSize,
                                PROMISE_RESOLVE_THENABLE_JOB_FIELDS)
#undef PROMISE_RESOLVE_THENABLE_JOB_FIELDS

  // Dispatched behavior.
  DECL_CAST(PromiseResolveThenableJobTask)
  DECL_PRINTER(PromiseResolveThenableJobTask)
  DECL_VERIFIER(PromiseResolveThenableJobTask)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseResolveThenableJobTask);
};

// Struct to hold the state of a PromiseCapability.
class PromiseCapability : public Struct {
 public:
  DECL_ACCESSORS(promise, HeapObject)
  DECL_ACCESSORS(resolve, Object)
  DECL_ACCESSORS(reject, Object)

// Layout description.
#define PROMISE_CAPABILITY_FIELDS(V) \
  V(kPromiseOffset, kTaggedSize)     \
  V(kResolveOffset, kTaggedSize)     \
  V(kRejectOffset, kTaggedSize)      \
  /* Total size. */                  \
  V(kSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(Struct::kHeaderSize, PROMISE_CAPABILITY_FIELDS)
#undef PROMISE_CAPABILITY_FIELDS

  // Dispatched behavior.
  DECL_CAST(PromiseCapability)
  DECL_PRINTER(PromiseCapability)
  DECL_VERIFIER(PromiseCapability)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseCapability);
};

// A representation of promise reaction. This differs from the specification
// in that the PromiseReaction here holds both handlers for the fulfill and
// the reject case. When a JSPromise is eventually resolved (either via
// fulfilling it or rejecting it), we morph this PromiseReaction object in
// memory into a proper PromiseReactionJobTask and schedule it on the queue
// of microtasks. So the size of PromiseReaction and the size of the
// PromiseReactionJobTask has to be same for this to work.
//
// The PromiseReaction::promise_or_capability field can either hold a JSPromise
// instance (in the fast case of a native promise) or a PromiseCapability in
// case of a Promise subclass. In case of await it can also be undefined if
// PromiseHooks are disabled (see https://github.com/tc39/ecma262/pull/1146).
//
// The PromiseReaction objects form a singly-linked list, terminated by
// Smi 0. On the JSPromise instance they are linked in reverse order,
// and are turned into the proper order again when scheduling them on
// the microtask queue.
class PromiseReaction : public Struct {
 public:
  enum Type { kFulfill, kReject };

  DECL_ACCESSORS(next, Object)
  DECL_ACCESSORS(reject_handler, HeapObject)
  DECL_ACCESSORS(fulfill_handler, HeapObject)
  // [promise_or_capability]: Either a JSPromise (in case of native promises),
  // a PromiseCapability (general case), or undefined (in case of await).
  DECL_ACCESSORS(promise_or_capability, HeapObject)

// Layout description.
#define PROMISE_REACTION_FIELDS(V)           \
  V(kNextOffset, kTaggedSize)                \
  V(kRejectHandlerOffset, kTaggedSize)       \
  V(kFulfillHandlerOffset, kTaggedSize)      \
  V(kPromiseOrCapabilityOffset, kTaggedSize) \
  /* Total size. */                          \
  V(kSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(Struct::kHeaderSize, PROMISE_REACTION_FIELDS)
#undef PROMISE_REACTION_FIELDS

  // Dispatched behavior.
  DECL_CAST(PromiseReaction)
  DECL_PRINTER(PromiseReaction)
  DECL_VERIFIER(PromiseReaction)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PromiseReaction);
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_PROMISE_H_
