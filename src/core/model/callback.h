/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef CALLBACK_H
#define CALLBACK_H

#include "ptr.h"
#include "fatal-error.h"
#include "empty.h"
#include "type-traits.h"
#include "attribute.h"
#include "attribute-helper.h"
#include "simple-ref-count.h"
#include <typeinfo>

namespace ns3 {

/***
 * \internal
 * This code was originally written based on the techniques 
 * described in http://www.codeproject.com/cpp/TTLFunction.asp
 * It was subsequently rewritten to follow the architecture
 * outlined in "Modern C++ Design" by Andrei Alexandrescu in 
 * chapter 5, "Generalized Functors".
 *
 * This code uses:
 *   - default template parameters to saves users from having to
 *     specify empty parameters when the number of parameters
 *     is smaller than the maximum supported number
 *   - the pimpl idiom: the Callback class is passed around by 
 *     value and delegates the crux of the work to its pimpl
 *     pointer.
 *   - two pimpl implementations which derive from CallbackImpl
 *     FunctorCallbackImpl can be used with any functor-type
 *     while MemPtrCallbackImpl can be used with pointers to
 *     member functions.
 *   - a reference list implementation to implement the Callback's
 *     value semantics.
 *
 * This code most notably departs from the alexandrescu 
 * implementation in that it does not use type lists to specify
 * and pass around the types of the callback arguments.
 * Of course, it also does not use copy-destruction semantics
 * and relies on a reference list rather than autoPtr to hold
 * the pointer.
 */
template <typename T>
struct CallbackTraits;

template <typename T>
struct CallbackTraits<T *>
{
  static T & GetReference (T * const p)
  {
    return *p;
  }
};

class CallbackImplBase : public SimpleRefCount<CallbackImplBase>
{
public:
  virtual ~CallbackImplBase () {}
  virtual bool IsEqual (Ptr<const CallbackImplBase> other) const = 0;
};

// declare the CallbackImpl class
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CallbackImpl;
// define CallbackImpl for 0 params
template <typename R>
class CallbackImpl<R,empty,empty,empty,empty,empty,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (void) = 0;
};
// define CallbackImpl for 1 params
template <typename R, typename T1>
class CallbackImpl<R,T1,empty,empty,empty,empty,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1) = 0;
};
// define CallbackImpl for 2 params
template <typename R, typename T1, typename T2>
class CallbackImpl<R,T1,T2,empty,empty,empty,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2) = 0;
};
// define CallbackImpl for 3 params
template <typename R, typename T1, typename T2, typename T3>
class CallbackImpl<R,T1,T2,T3,empty,empty,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3) = 0;
};
// define CallbackImpl for 4 params
template <typename R, typename T1, typename T2, typename T3, typename T4>
class CallbackImpl<R,T1,T2,T3,T4,empty,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4) = 0;
};
// define CallbackImpl for 5 params
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
class CallbackImpl<R,T1,T2,T3,T4,T5,empty,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4, T5) = 0;
};
// define CallbackImpl for 6 params
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CallbackImpl<R,T1,T2,T3,T4,T5,T6,empty,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4, T5, T6) = 0;
};
// define CallbackImpl for 7 params
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,empty,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4, T5, T6, T7) = 0;
};
// define CallbackImpl for 8 params
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,empty> : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4, T5, T6, T7, T8) = 0;
};
// define CallbackImpl for 9 params
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CallbackImpl : public CallbackImplBase {
public:
  virtual ~CallbackImpl () {}
  virtual R operator() (T1, T2, T3, T4, T5, T6, T7, T8, T9) = 0;
};


// an impl for Functors:
template <typename T, typename R, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6, typename T7, typename T8, typename T9>
class FunctorCallbackImpl : public CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> {
public:
  FunctorCallbackImpl (T const &functor)
    : m_functor (functor) {}
  virtual ~FunctorCallbackImpl () {}
  R operator() (void) {
    return m_functor ();
  }
  R operator() (T1 a1) {
    return m_functor (a1);
  }
  R operator() (T1 a1,T2 a2) {
    return m_functor (a1,a2);
  }
  R operator() (T1 a1,T2 a2,T3 a3) {
    return m_functor (a1,a2,a3);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4) {
    return m_functor (a1,a2,a3,a4);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5) {
    return m_functor (a1,a2,a3,a4,a5);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6) {
    return m_functor (a1,a2,a3,a4,a5,a6);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7) {
    return m_functor (a1,a2,a3,a4,a5,a6,a7);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7,T8 a8) {
    return m_functor (a1,a2,a3,a4,a5,a6,a7,a8);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7,T8 a8,T9 a9) {
    return m_functor (a1,a2,a3,a4,a5,a6,a7,a8,a9);
  }
  virtual bool IsEqual (Ptr<const CallbackImplBase> other) const {
    FunctorCallbackImpl<T,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> const *otherDerived = 
      dynamic_cast<FunctorCallbackImpl<T,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> const *> (PeekPointer (other));
    if (otherDerived == 0)
      {
        return false;
      }
    else if (otherDerived->m_functor != m_functor)
      {
        return false;
      }
    return true;
  }
private:
  T m_functor;
};

// an impl for pointer to member functions
template <typename OBJ_PTR, typename MEM_PTR, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class MemPtrCallbackImpl : public CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> {
public:
  MemPtrCallbackImpl (OBJ_PTR const&objPtr, MEM_PTR mem_ptr)
    : m_objPtr (objPtr), m_memPtr (mem_ptr) {}
  virtual ~MemPtrCallbackImpl () {}
  R operator() (void) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)();
  }
  R operator() (T1 a1) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1);
  }
  R operator() (T1 a1,T2 a2) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2);
  }
  R operator() (T1 a1,T2 a2,T3 a3) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4, a5);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4, a5, a6);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4, a5, a6, a7);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7,T8 a8) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4, a5, a6, a7, a8);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7,T8 a8, T9 a9) {
    return ((CallbackTraits<OBJ_PTR>::GetReference (m_objPtr)).*m_memPtr)(a1, a2, a3, a4, a5, a6, a7, a8, a9);
  }
  virtual bool IsEqual (Ptr<const CallbackImplBase> other) const {
    MemPtrCallbackImpl<OBJ_PTR,MEM_PTR,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> const *otherDerived = 
      dynamic_cast<MemPtrCallbackImpl<OBJ_PTR,MEM_PTR,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> const *> (PeekPointer (other));
    if (otherDerived == 0)
      {
        return false;
      }
    else if (otherDerived->m_objPtr != m_objPtr ||
             otherDerived->m_memPtr != m_memPtr)
      {
        return false;
      }
    return true;
  }
private:
  OBJ_PTR const m_objPtr;
  MEM_PTR m_memPtr;
};

// an impl for Bound Functors:
template <typename T, typename R, typename TX, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6, typename T7, typename T8>
class BoundFunctorCallbackImpl : public CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,empty> {
public:
  template <typename FUNCTOR, typename ARG>
  BoundFunctorCallbackImpl (FUNCTOR functor, ARG a)
    : m_functor (functor), m_a (a) {}
  virtual ~BoundFunctorCallbackImpl () {}
  R operator() (void) {
    return m_functor (m_a);
  }
  R operator() (T1 a1) {
    return m_functor (m_a,a1);
  }
  R operator() (T1 a1,T2 a2) {
    return m_functor (m_a,a1,a2);
  }
  R operator() (T1 a1,T2 a2,T3 a3) {
    return m_functor (m_a,a1,a2,a3);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4) {
    return m_functor (m_a,a1,a2,a3,a4);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5) {
    return m_functor (m_a,a1,a2,a3,a4,a5);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6) {
    return m_functor (m_a,a1,a2,a3,a4,a5,a6);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7) {
    return m_functor (m_a,a1,a2,a3,a4,a5,a6,a7);
  }
  R operator() (T1 a1,T2 a2,T3 a3,T4 a4,T5 a5,T6 a6,T7 a7,T8 a8) {
    return m_functor (m_a,a1,a2,a3,a4,a5,a6,a7,a8);
  }
  virtual bool IsEqual (Ptr<const CallbackImplBase> other) const {
    BoundFunctorCallbackImpl<T,R,TX,T1,T2,T3,T4,T5,T6,T7,T8> const *otherDerived = 
      dynamic_cast<BoundFunctorCallbackImpl<T,R,TX,T1,T2,T3,T4,T5,T6,T7,T8> const *> (PeekPointer (other));
    if (otherDerived == 0)
      {
        return false;
      }
    else if (otherDerived->m_functor != m_functor ||
             otherDerived->m_a != m_a)
      {
        return false;
      }
    return true;
  }
private:
  T m_functor;
  typename TypeTraits<TX>::ReferencedType m_a;
};


class CallbackBase {
public:
  CallbackBase () : m_impl () {}
  Ptr<CallbackImplBase> GetImpl (void) const { return m_impl; }
protected:
  CallbackBase (Ptr<CallbackImplBase> impl) : m_impl (impl) {}
  Ptr<CallbackImplBase> m_impl;

  static std::string Demangle (const std::string& mangled);
};

/**
 * \brief Callback template class
 *
 * This class template implements the Functor Design Pattern.
 * It is used to declare the type of a Callback:
 *  - the first non-optional template argument represents
 *    the return type of the callback.
 *  - the second optional template argument represents
 *    the type of the first argument to the callback.
 *  - the third optional template argument represents
 *    the type of the second argument to the callback.
 *  - the fourth optional template argument represents
 *    the type of the third argument to the callback.
 *  - the fifth optional template argument represents
 *    the type of the fourth argument to the callback.
 *  - the sixth optional template argument represents
 *    the type of the fifth argument to the callback.
 *
 * Callback instances are built with the \ref MakeCallback
 * template functions. Callback instances have POD semantics:
 * the memory they allocate is managed automatically, without
 * user intervention which allows you to pass around Callback
 * instances by value.
 *
 * Sample code which shows how to use this class template 
 * as well as the function templates \ref MakeCallback :
 * \include src/core/examples/main-callback.cc
 */

template<typename R, 
         typename T1 = empty, typename T2 = empty, 
         typename T3 = empty, typename T4 = empty,
         typename T5 = empty, typename T6 = empty,
         typename T7 = empty, typename T8 = empty,
         typename T9 = empty>
class Callback : public CallbackBase {
public:
  Callback () {}

  // There are two dummy args below to ensure that this constructor is
  // always properly disambiguated by the c++ compiler
  template <typename FUNCTOR>
  Callback (FUNCTOR const &functor, bool, bool) 
    : CallbackBase (Create<FunctorCallbackImpl<FUNCTOR,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> > (functor))
  {}

  template <typename OBJ_PTR, typename MEM_PTR>
  Callback (OBJ_PTR const &objPtr, MEM_PTR mem_ptr)
    : CallbackBase (Create<MemPtrCallbackImpl<OBJ_PTR,MEM_PTR,R,T1,T2,T3,T4,T5,T6,T7,T8,T9> > (objPtr, mem_ptr))
  {}

  Callback (Ptr<CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> > const &impl)
    : CallbackBase (impl)
  {}

  template <typename T>
  Callback<R,T2,T3,T4,T5,T6,T7,T8,T9> Bind (T a) {
    Ptr<CallbackImpl<R,T2,T3,T4,T5,T6,T7,T8,T9,empty> > impl =
      Ptr<CallbackImpl<R,T2,T3,T4,T5,T6,T7,T8,T9,empty> > (
        new BoundFunctorCallbackImpl<
          Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9>,
          R,T1,T2,T3,T4,T5,T6,T7,T8,T9> (*this, a), false);
    return Callback<R,T2,T3,T4,T5,T6,T7,T8,T9> (impl);
  }

  bool IsNull (void) const {
    return (DoPeekImpl () == 0) ? true : false;
  }
  void Nullify (void) {
    m_impl = 0;
  }

  R operator() (void) const {
    return (*(DoPeekImpl ()))();
  }
  R operator() (T1 a1) const {
    return (*(DoPeekImpl ()))(a1);
  }
  R operator() (T1 a1, T2 a2) const {
    return (*(DoPeekImpl ()))(a1,a2);
  }
  R operator() (T1 a1, T2 a2, T3 a3) const {
    return (*(DoPeekImpl ()))(a1,a2,a3);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4,T5 a5) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4,a5);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4,T5 a5,T6 a6) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4,a5,a6);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4,T5 a5,T6 a6,T7 a7) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4,a5,a6,a7);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4,T5 a5,T6 a6,T7 a7,T8 a8) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4,a5,a6,a7,a8);
  }
  R operator() (T1 a1, T2 a2, T3 a3, T4 a4,T5 a5,T6 a6,T7 a7,T8 a8, T9 a9) const {
    return (*(DoPeekImpl ()))(a1,a2,a3,a4,a5,a6,a7,a8,a9);
  }

  bool IsEqual (const CallbackBase &other) const {
    return m_impl->IsEqual (other.GetImpl ());
  }

  bool CheckType (const CallbackBase & other) const {
    return DoCheckType (other.GetImpl ());
  }
  void Assign (const CallbackBase &other) {
    DoAssign (other.GetImpl ());
  }
private:
  CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> *DoPeekImpl (void) const {
    return static_cast<CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> *> (PeekPointer (m_impl));
  }
  bool DoCheckType (Ptr<const CallbackImplBase> other) const {
    if (other != 0 && dynamic_cast<const CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> *> (PeekPointer (other)) != 0)
      {
        return true;
      }
    else if (other == 0)
      {
        return true;
      }
    else
      {
        return false;
      }
  }
  void DoAssign (Ptr<const CallbackImplBase> other) {
    if (!DoCheckType (other))
      {
        NS_FATAL_ERROR ("Incompatible types. (feed to \"c++filt -t\" if needed)" << std::endl <<
                        "got=" << Demangle ( typeid (*other).name () ) << std::endl <<
                        "expected=" << Demangle ( typeid (CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> *).name () ));
      }
    m_impl = const_cast<CallbackImplBase *> (PeekPointer (other));
  }
};


template <typename R, typename T1, typename T2,
          typename T3, typename T4,
          typename T5, typename T6,
          typename T7, typename T8,
          typename T9>
bool operator != (Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> a, Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> b)
{
  return !a.IsEqual (b);
}

/**
 * \ingroup core
 * \defgroup MakeCallback MakeCallback
 *
 */

/**
 * \ingroup MakeCallback
 * \param memPtr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes no arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R>
Callback<R> MakeCallback (R (T::*memPtr)(void), OBJ objPtr) {
  return Callback<R> (objPtr, memPtr);
}
template <typename T, typename OBJ, typename R>
Callback<R> MakeCallback (R (T::*mem_ptr)() const, OBJ objPtr) {
  return Callback<R> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes one argument
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1>
Callback<R,T1> MakeCallback (R (T::*mem_ptr)(T1), OBJ objPtr) {
  return Callback<R,T1> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1>
Callback<R,T1> MakeCallback (R (T::*mem_ptr)(T1) const, OBJ objPtr) {
  return Callback<R,T1> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes two arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2>
Callback<R,T1,T2> MakeCallback (R (T::*mem_ptr)(T1,T2), OBJ objPtr) {
  return Callback<R,T1,T2> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2>
Callback<R,T1,T2> MakeCallback (R (T::*mem_ptr)(T1,T2) const, OBJ objPtr) {
  return Callback<R,T1,T2> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes three arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1,typename T2, typename T3>
Callback<R,T1,T2,T3> MakeCallback (R (T::*mem_ptr)(T1,T2,T3), OBJ objPtr) {
  return Callback<R,T1,T2,T3> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1,typename T2, typename T3>
Callback<R,T1,T2,T3> MakeCallback (R (T::*mem_ptr)(T1,T2,T3) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes four arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4>
Callback<R,T1,T2,T3,T4> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4>
Callback<R,T1,T2,T3,T4> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes five arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5>
Callback<R,T1,T2,T3,T4,T5> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5>
Callback<R,T1,T2,T3,T4,T5> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5> (objPtr, mem_ptr);
}
/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes six arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5,typename T6>
Callback<R,T1,T2,T3,T4,T5,T6> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6>
Callback<R,T1,T2,T3,T4,T5,T6> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6> (objPtr, mem_ptr);
}

/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes seven arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5,typename T6, typename T7>
Callback<R,T1,T2,T3,T4,T5,T6,T7> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6, typename T7>
Callback<R,T1,T2,T3,T4,T5,T6,T7> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7> (objPtr, mem_ptr);
}


/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes eight arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5,typename T6, typename T7, typename T8>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7,T8), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6, typename T7, typename T8>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7,T8) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> (objPtr, mem_ptr);
}

/**
 * \ingroup MakeCallback
 * \param mem_ptr class method member pointer
 * \param objPtr class instance
 * \return a wrapper Callback
 * Build Callbacks for class method members which takes nine arguments
 * and potentially return a value.
 */
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5,typename T6, typename T7, typename T8, typename T9>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7,T8,T9), OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> (objPtr, mem_ptr);
}
template <typename T, typename OBJ, typename R, typename T1, typename T2, typename T3, typename T4,typename T5, typename T6, typename T7, typename T8, typename T9>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> MakeCallback (R (T::*mem_ptr)(T1,T2,T3,T4,T5,T6,T7,T8,T9) const, OBJ objPtr) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> (objPtr, mem_ptr);
}

/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes no arguments
 * and potentially return a value.
 */
template <typename R>
Callback<R> MakeCallback (R (*fnPtr)()) {
  return Callback<R> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes one argument
 * and potentially return a value.
 */
template <typename R, typename T1>
Callback<R,T1> MakeCallback (R (*fnPtr)(T1)) {
  return Callback<R,T1> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes two arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2>
Callback<R,T1,T2> MakeCallback (R (*fnPtr)(T1,T2)) {
  return Callback<R,T1,T2> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes three arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3>
Callback<R,T1,T2,T3> MakeCallback (R (*fnPtr)(T1,T2,T3)) {
  return Callback<R,T1,T2,T3> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes four arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4>
Callback<R,T1,T2,T3,T4> MakeCallback (R (*fnPtr)(T1,T2,T3,T4)) {
  return Callback<R,T1,T2,T3,T4> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes five arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5>
Callback<R,T1,T2,T3,T4,T5> MakeCallback (R (*fnPtr)(T1,T2,T3,T4,T5)) {
  return Callback<R,T1,T2,T3,T4,T5> (fnPtr, true, true);
}
/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes six arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6>
Callback<R,T1,T2,T3,T4,T5,T6> MakeCallback (R (*fnPtr)(T1,T2,T3,T4,T5,T6)) {
  return Callback<R,T1,T2,T3,T4,T5,T6> (fnPtr, true, true);
}

/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes seven arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7>
Callback<R,T1,T2,T3,T4,T5,T6,T7> MakeCallback (R (*fnPtr)(T1,T2,T3,T4,T5,T6,T7)) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7> (fnPtr, true, true);
}

/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes eight arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7, typename T8>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> MakeCallback (R (*fnPtr)(T1,T2,T3,T4,T5,T6,T7,T8)) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> (fnPtr, true, true);
}

/**
 * \ingroup MakeCallback
 * \param fnPtr function pointer
 * \return a wrapper Callback
 * Build Callbacks for functions which takes nine arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7, typename T8, typename T9>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> MakeCallback (R (*fnPtr)(T1,T2,T3,T4,T5,T6,T7,T8,T9)) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> (fnPtr, true, true);
}



/**
 * \ingroup MakeCallback
 * \return a wrapper Callback
 * Build a null callback which takes no arguments
 * and potentially return a value.
 */
template <typename R>
Callback<R> MakeNullCallback (void) {
  return Callback<R> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes one argument
 * and potentially return a value.
 */
template <typename R, typename T1>
Callback<R,T1> MakeNullCallback (void) {
  return Callback<R,T1> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes two arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2>
Callback<R,T1,T2> MakeNullCallback (void) {
  return Callback<R,T1,T2> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes three arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3>
Callback<R,T1,T2,T3> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes four arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4>
Callback<R,T1,T2,T3,T4> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes five arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5>
Callback<R,T1,T2,T3,T4,T5> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4,T5> ();
}
/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes six arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6>
Callback<R,T1,T2,T3,T4,T5,T6> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4,T5,T6> ();
}

/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes seven arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7>
Callback<R,T1,T2,T3,T4,T5,T6,T7> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7> ();
}

/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes eight arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7, typename T8>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> ();
}

/**
 * \ingroup MakeCallback
 * \overload Callback<R> MakeNullCallback (void)
 * \return a wrapper Callback
 * Build a null callback which takes nine arguments
 * and potentially return a value.
 */
template <typename R, typename T1, typename T2,typename T3,typename T4,typename T5,typename T6, typename T7, typename T8, typename T9>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> MakeNullCallback (void) {
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8,T9> ();
}


/*
 * The following is experimental code. It works but we have
 * not yet determined whether or not it is really useful and whether
 * or not we really want to use it.
 */

template <typename R, typename TX, typename ARG>
Callback<R> MakeBoundCallback (R (*fnPtr)(TX), ARG a) {
  Ptr<CallbackImpl<R,empty,empty,empty,empty,empty,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX),R,TX,empty,empty,empty,empty,empty,empty,empty,empty> >(fnPtr, a);
  return Callback<R> (impl);
}

template <typename R, typename TX, typename ARG, 
          typename T1>
Callback<R,T1> MakeBoundCallback (R (*fnPtr)(TX,T1), ARG a) {
  Ptr<CallbackImpl<R,T1,empty,empty,empty,empty,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1),R,TX,T1,empty,empty,empty,empty,empty,empty,empty> > (fnPtr, a);
  return Callback<R,T1> (impl);
}
template <typename R, typename TX, typename ARG, 
          typename T1, typename T2>
Callback<R,T1,T2> MakeBoundCallback (R (*fnPtr)(TX,T1,T2), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,empty,empty,empty,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2),R,TX,T1,T2,empty,empty,empty,empty,empty,empty> > (fnPtr, a);
  return Callback<R,T1,T2> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3>
Callback<R,T1,T2,T3> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,empty,empty,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3),R,TX,T1,T2,T3,empty,empty,empty,empty,empty> > (fnPtr, a);
  return Callback<R,T1,T2,T3> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3,typename T4>
Callback<R,T1,T2,T3,T4> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3,T4), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,T4,empty,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3,T4),R,TX,T1,T2,T3,T4,empty,empty,empty,empty> > (fnPtr, a);
  return Callback<R,T1,T2,T3,T4> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3,typename T4,typename T5>
Callback<R,T1,T2,T3,T4,T5> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3,T4,T5), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,T4,T5,empty,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3,T4,T5),R,TX,T1,T2,T3,T4,T5,empty,empty,empty> > (fnPtr, a);
  return Callback<R,T1,T2,T3,T4,T5> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3,typename T4,typename T5, typename T6>
Callback<R,T1,T2,T3,T4,T5,T6> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3,T4,T5,T6), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,T4,T5,T6,empty,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3,T4,T5,T6),R,TX,T1,T2,T3,T4,T5,T6,empty,empty> > (fnPtr, a);
  return Callback<R,T1,T2,T3,T4,T5,T6> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3,typename T4,typename T5, typename T6, typename T7>
Callback<R,T1,T2,T3,T4,T5,T6,T7> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3,T4,T5,T6,T7), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,empty,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3,T4,T5,T6,T7),R,TX,T1,T2,T3,T4,T5,T6,T7,empty> > (fnPtr, a);
  return Callback<R,T1,T2,T3,T4,T5,T6,T7> (impl);
}
template <typename R, typename TX, typename ARG,
          typename T1, typename T2,typename T3,typename T4,typename T5, typename T6, typename T7, typename T8>
Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> MakeBoundCallback (R (*fnPtr)(TX,T1,T2,T3,T4,T5,T6,T7,T8), ARG a) {
  Ptr<CallbackImpl<R,T1,T2,T3,T4,T5,T6,T7,T8,empty> > impl =
    Create<BoundFunctorCallbackImpl<R (*)(TX,T1,T2,T3,T4,T5,T6,T7,T8),R,TX,T1,T2,T3,T4,T5,T6,T7,T8> > (fnPtr, a);
  return Callback<R,T1,T2,T3,T4,T5,T6,T7,T8> (impl);
}
} // namespace ns3

namespace ns3 {

class CallbackValue : public AttributeValue
{
public:
  CallbackValue ();
  CallbackValue (const CallbackBase &base);
  virtual ~CallbackValue ();
  void Set (CallbackBase base);
  template <typename T>
  bool GetAccessor (T &value) const;
  virtual Ptr<AttributeValue> Copy (void) const;
  virtual std::string SerializeToString (Ptr<const AttributeChecker> checker) const;
  virtual bool DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker);
private:
  CallbackBase m_value;
};

ATTRIBUTE_ACCESSOR_DEFINE (Callback);
ATTRIBUTE_CHECKER_DEFINE (Callback);

} // namespace ns3

namespace ns3 {

template <typename T>
bool CallbackValue::GetAccessor (T &value) const
{
  if (value.CheckType (m_value))
    {
      value.Assign (m_value);
      return true;
    }
  return false;
}

} // namespace ns3


#endif /* CALLBACK_H */
