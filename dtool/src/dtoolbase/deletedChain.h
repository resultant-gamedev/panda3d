// Filename: deletedChain.h
// Created by:  drose (01Apr06)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef DELETEDCHAIN_H
#define DELETEDCHAIN_H

#include "dtoolbase.h"

#include "mutexImpl.h"
#include "atomicAdjust.h"
#include "numeric_types.h"
#include <assert.h>

#ifdef HAVE_ATOMIC_COMPARE_AND_EXCHANGE_PTR
// Actually, there appears to be a (maybe fatal) flaw in our
//implementation of DeletedChain via the atomic exchange operation.
//Specifically, a pointer may be removed from the head of the chain,
//then the same pointer reinserted in the chain, while another thread
//is waiting; and that thread will not detect the change.  For now,
//then, let's not use this implmentation, and fall back to the mutex.
//#define DELETED_CHAIN_USE_ATOMIC_EXCHANGE
#endif

////////////////////////////////////////////////////////////////////
//       Class : DeletedChain
// Description : This template class can be used to provide faster
//               allocation/deallocation for many Panda objects.  It
//               works by maintaining a linked list of deleted objects
//               that are all of the same type; when a new object is
//               allocated that matches that type, the same space is
//               just reused.
//
//               This is particularly important to do in the case of a
//               multithreaded pipeline, so we minimize contention
//               between the parallel threads.  If we didn't do this,
//               the threads might compete overmuch on the system
//               mutex protecting the malloc() call.  This way, there
//               can be a different mutex for each type of object; or on
//               some systems (e.g. i386), no mutex at all.
//
//               Of course, this trick of maintaining the deleted
//               object chain won't work in the presence of
//               polymorphism, where you might have many classes that
//               derive from a base class, and all of them have a
//               different size--unless you instantiate a DeletedChain
//               for *every* kind of derived class.  The
//               ALLOC_DELETED_CHAIN macro, below, is designed to make
//               this easy.
////////////////////////////////////////////////////////////////////
template<class Type>
class DeletedChain {
public:
  INLINE static Type *allocate(size_t size);
  INLINE static void deallocate(Type *ptr);

private:
  class ObjectNode {
  public:
    TVOLATILE ObjectNode * TVOLATILE _next;
#ifndef NDEBUG
    TVOLATILE PN_int32 _flag;
#endif
  };

  // Ideally, the compiler and linker will unify all references to
  // this static pointer for a given type, as per the C++ spec.
  // However, if the compiler fails to do this (*cough* Microsoft), it
  // won't be a big deal; it just means there will be multiple
  // unrelated chains of deleted objects for a particular type.
  static TVOLATILE ObjectNode * TVOLATILE _deleted_chain;

#ifndef DELETED_CHAIN_USE_ATOMIC_EXCHANGE
  // If we don't have atomic compare-and-exchange, we need to use a
  // Mutex to protect the above linked list.
  static INLINE void init_lock();
  static MutexImpl *_lock;
#endif
};

static const PN_int32 deleted_chain_flag_hash = 0x12345678;

// Place this macro within a class definition to define appropriate
// operator new and delete methods that take advantage of
// DeletedChain.
#define ALLOC_DELETED_CHAIN(Type)                            \
  inline void *operator new(size_t size) {                   \
    return (void *)DeletedChain< Type >::allocate(size);       \
  }                                                          \
  inline void *operator new(size_t size, void *ptr) {        \
    return ptr;                                              \
  }                                                          \
  inline void operator delete(void *ptr) {                   \
    DeletedChain< Type >::deallocate((Type *)ptr);             \
  }                                                          \
  inline void operator delete(void *, void *) {              \
  }

#include "deletedChain.T"

#endif

