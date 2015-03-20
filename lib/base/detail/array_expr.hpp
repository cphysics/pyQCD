#ifndef ARRAY_EXPR_HPP
#define ARRAY_EXPR_HPP

/* This file provides expression templates for the Array class, so that
 * temporaries do not need to be created when performing arithmetic operations.
 */

#include <typeinfo>
#include <type_traits>

#include <utils/macros.hpp>
#include <utils/templates.hpp>

#include "../layout.hpp"
#include "operators.hpp"


namespace pyQCD
{
  template <typename T, template <typename> class Alloc, typename U>
  class Array;

  template <typename T1, typename T2>
  class ArrayExpr;

  template <typename T>
  class ArrayConst;

  template <typename T, template <typename> class Alloc>
  class Lattice;

  // These traits classes allow us to switch between a const ref and simple
  // value in expression subclasses, avoiding returning dangling references.
  template <typename T1, typename T2>
  struct ExprReturnTraits
  {
    typedef T2 type;
  };


  template <typename T1, template <typename> class T2, typename T3>
  struct ExprReturnTraits<Array<T1, T2, T3>, T1>
  {
    typedef T1& type;
  };


  // These traits classes allow us to switch between a const ref and simple
  // value in expression subclasses, avoiding returning dangling references.
  template <typename T>
  struct OperandTraits
  {
    typedef const T& type;
  };


  template <typename T>
  struct OperandTraits<ArrayConst<T> >
  {
    typedef ArrayConst<T> type;
  };

  // Traits to check first whether supplied type is a Lattice, then get the
  // layout from the supplied object, if applicable
  template <typename T>
  struct CrtpLayoutTraits
  {
    static const Layout* get_layout(const T& arr)
    { return arr.layout(); }
  };


  template <typename T1, template <typename> class A, typename T2>
  struct CrtpLayoutTraits<Array<T1, A, T2> >
  {
    static const Layout* get_layout(const Array<T1, A, T2>& lat)
    { return nullptr; }
  };


  // TODO: Eliminate need for second template parameter
  template <typename T1, typename T2>
  class ArrayExpr
  {
    // This is the main expression class from which all others are derived. It
    // uses CRTP to escape inheritance. Parameter T1 is the expression type
    // and T2 is the fundamental type contained in the Array. This allows
    // expressions to be abstracted to a nested hierarchy of types. When the
    // compiler goes through and does it's thing, the definitions of the
    // operations within these template classes are all spliced together.

  public:
    // CRTP magic - call functions in the Array class
    typename ExprReturnTraits<T1, T2>::type operator[](const int i)
    { return static_cast<T1&>(*this)[i]; }
    const typename ExprReturnTraits<T1, T2>::type operator[](const int i) const
    { return static_cast<const T1&>(*this)[i]; }

    unsigned long size() const { return static_cast<const T1&>(*this).size(); }
    const Layout* layout() const
    { return CrtpLayoutTraits<T1>::get_layout(static_cast<const T1&>(*this)); }

    operator T1&()
    { return static_cast<T1&>(*this); }
    operator T1 const&() const
    { return static_cast<const T1&>(*this); }
  };


  template <typename T>
  class ArrayConst
    : public ArrayExpr<ArrayConst<T>, T>
  {
    // Expression subclass for const operations
  public:
    ArrayConst(const T& scalar, const unsigned long size, const Layout* layout)
      : scalar_(scalar), size_(size), layout_(layout)
    { }
    const T& operator[](const unsigned long i) const
    { return scalar_; }
    unsigned long size() const { return size_; }
    const Layout* layout() const { return layout_; }

  private:
    const T& scalar_;
    unsigned long size_;
    const Layout* layout_;
  };


  template <typename T1, typename T2, typename Op>
  class ArrayUnary
    : public ArrayExpr<ArrayUnary<T1, T2, Op>,
        decltype(Op::apply(std::declval<T2>()))>
  {
  public:
    ArrayUnary(const ArrayExpr<T1, T2>& operand) : operand_(operand)
    { }

    const decltype(Op::apply(std::declval<T2>()))
    operator[](const unsigned int i) const
    { return Op::apply(operand_[i]); }

    unsigned long size() const { return operand_.size(); }
    const Layout* layout() const { return operand_.layout(); }

  private:
    typename OperandTraits<T1>::type operand_;
  };


  template <typename T1, typename T2, typename T3, typename T4, typename Op>
  class ArrayBinary
    : public ArrayExpr<ArrayBinary<T1, T2, T3, T4, Op>,
        decltype(Op::apply(std::declval<T3>(), std::declval<T4>()))>
  {
  // Expression subclass for binary operations
  public:
    ArrayBinary(const ArrayExpr<T1, T3>& lhs, const ArrayExpr<T2, T4>& rhs)
      : lhs_(lhs), rhs_(rhs)
    {
      if (lhs.layout() != nullptr and rhs.layout() != nullptr) {
        pyQCDassert ((typeid(*(lhs.layout())) == typeid(*(rhs.layout()))),
          std::bad_cast());
      }
    }
    // Here we denote the actual arithmetic operation.
    const decltype(Op::apply(std::declval<T3>(), std::declval<T4>()))
    operator[](const unsigned long i) const
    { return Op::apply(lhs_[i], rhs_[i]); }

    unsigned long size() const { return lhs_.size(); }
    const Layout* layout() const { return lhs_.layout(); }

  private:
    // The members - the inputs to the binary operation
    typename OperandTraits<T1>::type lhs_;
    typename OperandTraits<T2>::type rhs_;
  };

  // Some macros for the operator overloads, as the code is almost
  // the same in each case. For the scalar multiplies I've used
  // some SFINAE to disable these more generalized functions when
  // a ArrayExpr is used.
#define ARRAY_EXPR_OPERATOR(op, trait)                                \
  template <typename T1, typename T2, typename T3, typename T4>       \
  const ArrayBinary<T1, T2, T3, T4, trait>                            \
  operator op(const ArrayExpr<T1, T3>& lhs,                           \
    const ArrayExpr<T2, T4>& rhs)                                     \
  {                                                                   \
    return ArrayBinary<T1, T2, T3, T4, trait>(lhs, rhs);              \
  }                                                                   \
                                                                      \
                                                                      \
  template <typename T1, typename T2, typename T3,                    \
    typename std::enable_if<                                          \
      !is_instance_of_type_temp<T3, pyQCD::ArrayExpr>::value>::type*  \
      = nullptr>                                                      \
  const ArrayBinary<T1, ArrayConst<T3>, T2, T3, trait>                \
  operator op(const ArrayExpr<T1, T2>& array, const T3& scalar)       \
  {                                                                   \
    return ArrayBinary<T1, ArrayConst<T3>, T2, T3, trait>             \
      (array, ArrayConst<T3>(scalar, array.size(), array.layout()));  \
  }

  // This macro is for the + and * operators where the scalar can
  // be either side of the operator.
#define ARRAY_EXPR_OPERATOR_REVERSE_SCALAR(op, trait)                 \
  template <typename T1, typename T2, typename T3,                    \
    typename std::enable_if<                                          \
      !is_instance_of_type_temp<T1, pyQCD::ArrayExpr>::value>::type*  \
      = nullptr>                                                      \
  const ArrayBinary<ArrayConst<T1>, T2, T1, T3, trait>                \
  operator op(const T1& scalar, const ArrayExpr<T2, T3>& array)       \
  {                                                                   \
    return ArrayBinary<ArrayConst<T1>, T2, T1, T3, trait>             \
      (ArrayConst<T1>(scalar, array.size(), array.layout()), array);  \
  }


  ARRAY_EXPR_OPERATOR(+, Plus);
  ARRAY_EXPR_OPERATOR_REVERSE_SCALAR(+, Plus);
  ARRAY_EXPR_OPERATOR(-, Minus);
  ARRAY_EXPR_OPERATOR(*, Multiplies);
  ARRAY_EXPR_OPERATOR_REVERSE_SCALAR(*, Multiplies);
  ARRAY_EXPR_OPERATOR(/, Divides);
}

#endif