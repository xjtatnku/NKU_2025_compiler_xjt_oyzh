/*
这一部分的实现稍微有些复杂
如果只是为了完成实验的话不需要理解这里是做什么的以及怎么用，只需要跟着框架的流程走就行

这里稍微说一下它的作用以及用法：
它主要用于为一系列类，提供一个统一的访问接口，下面写的这么复杂主要是为了支持多参数和返回值
在实验框架中，你可以在各个阶段的 visitor/ 子目录下找到对应的应用场景，如：
frontend/ast/visitor/printer：AST 的打印
middleend/visitor/codegen：由 AST 到 LLVM IR 的翻译

至于用法，你可以参考 frontend/ast/ast.h 与 frontend/ast/visitor/printer/ast_printer.h 的实现。
下面给一个使用示例：
有类型 Dog, Cat, Chicken：

template <typename... Ts>
using Visitor_t = VisitSetFrom<TypeList<Dog, Cat, Chicken>>::Visitor<Ts...>;
using Visitor   = VisitSetFrom<TypeList<Dog, Cat, Chicken>>::ErasedVisitor;

class Dog { public: void accept(Visitor& v) { v.visit(*this); } };
class Cat { public: void accept(Visitor& v) { v.visit(*this); } };
class Chicken { public: void accept(Visitor& v) { v.visit(*this); } };

class BarkVisitor : public Visitor_t<std::string, int n> // 这里的第一个参数表示返回类型，后续参数表示参数
Visitor_t<std::string, int n> 实际会被展开为
class visitor : public iVisit<std::string, int n>,
                public detail::TypedVisitDecl<std::string, Dog, int n>,
                public detail::TypedVisitDecl<std::string, Cat, int n>,
                public detail::TypedVisitDecl<std::string, Chicken, int n>
{};
其中 detail::TypedVisitDecl 是下方定义的类模板，用于生成虚函数，如 virtual std::string visit(Dog, int n) = 0;
然后，实际实现的访问者类 BarkVisitor 只需要继承 Visitor_t 并实现 visit 函数即可：
{
  public:
    std::string visit(Dog& d, int n) override { return "Woof! x" + std::to_string(n); }
    std::string visit(Cat& c, int n) override { return "Meow! x" + std::to_string(n); }
    std::string visit(Chicken& ch, int n) override { return "Ngm! x" + std::to_string(n); }
};

当然，你也可以不提前声明类型列表，而是使用模板来定义 accept 函数，这样在定义上会更简洁一些

最下方封装了 apply 函数，用于简化访问者的调用，它会自动推导返回值类型与访问者类型。在具体调用时，只需要：
BarkVisitor visitor;
Dog d;
std::string result = apply(visitor, d, 3);  // 第一个参数为访问者实例，第二个参数为被访问对象，后续参数为传递给 visit
函数的参数
*/

#ifndef __IVISITOR_H__
#define __IVISITOR_H__

#include <tuple>
#include <type_traits>
#include <memory>
#include <utility>
#include <optional>

template <typename... Ts>
struct TypeList
{
    static constexpr size_t size = sizeof...(Ts);
    using tuple_type             = std::tuple<Ts...>;
};

namespace type_list_utils
{
    template <typename List1, typename List2>
    struct Concat;

    template <typename... Ts1, typename... Ts2>
    struct Concat<TypeList<Ts1...>, TypeList<Ts2...>>
    {
        using type = TypeList<Ts1..., Ts2...>;
    };

    template <typename List1, typename List2>
    using Concat_t = typename Concat<List1, List2>::type;

    template <typename List, typename T>
    struct Append;

    template <typename... Ts, typename T>
    struct Append<TypeList<Ts...>, T>
    {
        using type = TypeList<Ts..., T>;
    };

    template <typename List, typename T>
    using Append_t = typename Append<List, T>::type;

    template <typename List, typename T>
    struct Contains;

    template <typename T>
    struct Contains<TypeList<>, T> : std::false_type
    {};

    template <typename T, typename... Rest>
    struct Contains<TypeList<T, Rest...>, T> : std::true_type
    {};

    template <typename U, typename T, typename... Rest>
    struct Contains<TypeList<U, Rest...>, T> : Contains<TypeList<Rest...>, T>
    {};

    template <typename List, typename T>
    constexpr bool Contains_v = Contains<List, T>::value;
}  // namespace type_list_utils

struct iVisitErased
{
    virtual ~iVisitErased() = default;
};

template <typename R, typename... Args>
struct iVisit
{
    using ReturnType       = R;
    using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
    virtual ~iVisit()      = default;
};

namespace detail
{
    template <typename T>
    struct ErasedVisitDecl
    {
        virtual void* visit(T&) = 0;
    };

    template <typename R, typename T, typename... Args>
    struct TypedVisitDecl
    {
        virtual R visit(T&, Args...) = 0;
    };
}  // namespace detail

template <typename... Ts>
struct VisitSet
{
    struct ErasedVisitor : iVisitErased, detail::ErasedVisitDecl<Ts>...
    {
        using detail::ErasedVisitDecl<Ts>::visit...;
        using VisitSetType       = VisitSet<Ts...>;
        virtual ~ErasedVisitor() = default;
    };

    template <typename R, typename... Args>
    struct Visitor : iVisit<R, Args...>, detail::TypedVisitDecl<R, Ts, Args...>...
    {
        using detail::TypedVisitDecl<R, Ts, Args...>::visit...;
        using VisitSetType = VisitSet<Ts...>;
        virtual ~Visitor() = default;
    };

    template <typename R, typename VisitorType, typename ArgsTuple, typename... Vs>
    class VisitorWrapperNV;

    template <typename R, typename VisitorType, typename ArgsTuple>
    class VisitorWrapperNV<R, VisitorType, ArgsTuple> : public ErasedVisitor
    {
      public:
        using ReturnType = R;
        explicit VisitorWrapperNV(VisitorType& concrete, ArgsTuple&& args)
            : concrete(concrete), argsTuple(std::move(args))
        {}

        R takeResult() { return std::move(*resultOpt); }

      protected:
        using TupleType = ArgsTuple;
        VisitorType&     concrete;
        ArgsTuple        argsTuple;
        std::optional<R> resultOpt;

        using ErasedVisitor::visit;

        template <typename T>
        void* dispatch(T& obj)
        {
            resultOpt.emplace(invoke(obj, std::make_index_sequence<std::tuple_size<TupleType>::value>{}));
            return &(*resultOpt);
        }

        template <typename T, std::size_t... I>
        R invoke(T& obj, std::index_sequence<I...>)
        {
            return concrete.visit(obj, std::forward<std::tuple_element_t<I, TupleType>>(std::get<I>(argsTuple))...);
        }
    };

    template <typename R, typename VisitorType, typename ArgsTuple, typename T, typename... Rest>
    class VisitorWrapperNV<R, VisitorType, ArgsTuple, T, Rest...>
        : public VisitorWrapperNV<R, VisitorType, ArgsTuple, Rest...>
    {
        using Base = VisitorWrapperNV<R, VisitorType, ArgsTuple, Rest...>;

      public:
        using Base::Base;
        using Base::dispatch;
        using Base::takeResult;
        using Base::visit;
        void* visit(T& v) override { return dispatch(v); }
    };

    template <typename VisitorType, typename ArgsTuple, typename... Vs>
    class VisitorWrapperV;

    template <typename VisitorType, typename ArgsTuple>
    class VisitorWrapperV<VisitorType, ArgsTuple> : public ErasedVisitor
    {
      public:
        explicit VisitorWrapperV(VisitorType& concrete, ArgsTuple&& args)
            : concrete(concrete), argsTuple(std::move(args))
        {}

        void takeResult() {}

      protected:
        using TupleType = ArgsTuple;
        VisitorType& concrete;
        ArgsTuple    argsTuple;

        using ErasedVisitor::visit;

        template <typename T>
        void* dispatch(T& obj)
        {
            invoke(obj, std::make_index_sequence<std::tuple_size<TupleType>::value>{});
            return nullptr;
        }

        template <typename T, std::size_t... I>
        void invoke(T& obj, std::index_sequence<I...>)
        {
            concrete.visit(obj, std::forward<std::tuple_element_t<I, TupleType>>(std::get<I>(argsTuple))...);
        }
    };

    template <typename VisitorType, typename ArgsTuple, typename T, typename... Rest>
    class VisitorWrapperV<VisitorType, ArgsTuple, T, Rest...> : public VisitorWrapperV<VisitorType, ArgsTuple, Rest...>
    {
        using Base = VisitorWrapperV<VisitorType, ArgsTuple, Rest...>;

      public:
        using Base::Base;
        using Base::dispatch;
        using Base::takeResult;
        using Base::visit;
        void* visit(T& v) override { return dispatch(v); }
    };

    template <typename R, typename Visitable, typename... DeclaredArgs, typename... CallArgs>
    static std::enable_if_t<!std::is_void_v<R>, R> apply(
        Visitable& vt, Visitor<R, DeclaredArgs...>& visitor, CallArgs&&... args)
    {
        using VisitorType   = Visitor<R, DeclaredArgs...>;
        using ArgsTuple     = std::tuple<CallArgs&&...>;
        ArgsTuple tupleArgs = std::forward_as_tuple(std::forward<CallArgs>(args)...);
        VisitorWrapperNV<R, VisitorType, ArgsTuple, Ts...> wrapper(visitor, std::move(tupleArgs));
        vt.accept(wrapper);
        return wrapper.takeResult();
    }

    template <typename R, typename Visitable, typename... DeclaredArgs, typename... CallArgs>
    static std::enable_if_t<std::is_void_v<R>, void> apply(
        Visitable& vt, Visitor<R, DeclaredArgs...>& visitor, CallArgs&&... args)
    {
        using VisitorType   = Visitor<R, DeclaredArgs...>;
        using ArgsTuple     = std::tuple<CallArgs&&...>;
        ArgsTuple tupleArgs = std::forward_as_tuple(std::forward<CallArgs>(args)...);
        VisitorWrapperV<VisitorType, ArgsTuple, Ts...> wrapper(visitor, std::move(tupleArgs));
        vt.accept(wrapper);
    }
};

namespace detail
{
    template <typename TypeListT>
    struct VisitSetFromImpl;

    template <typename... Ts>
    struct VisitSetFromImpl<TypeList<Ts...>>
    {
        using type = VisitSet<Ts...>;
    };
}  // namespace detail

template <typename TypeListT>
using VisitSetFrom = typename detail::VisitSetFromImpl<TypeListT>::type;

template <typename VisitSetT, typename... Ts>
using Visitor_t = typename VisitSetT::template Visitor<Ts...>;

template <typename VisitSetT>
using Visitor = typename VisitSetT::ErasedVisitor;

template <typename VisitorType, typename Visitable, typename... CallArgs,
    typename Return = typename std::remove_reference_t<VisitorType>::ReturnType,
    typename Set    = typename std::remove_reference_t<VisitorType>::VisitSetType>
static std::enable_if_t<!std::is_void_v<Return>, Return> apply(VisitorType& visitor, Visitable& vt, CallArgs&&... args)
{
    return Set::template apply<Return>(vt, visitor, std::forward<CallArgs>(args)...);
}

template <typename VisitorType, typename Visitable, typename... CallArgs,
    typename Return = typename std::remove_reference_t<VisitorType>::ReturnType,
    typename Set    = typename std::remove_reference_t<VisitorType>::VisitSetType>
static std::enable_if_t<std::is_void_v<Return>, void> apply(VisitorType& visitor, Visitable& vt, CallArgs&&... args)
{
    Set::template apply<Return>(vt, visitor, std::forward<CallArgs>(args)...);
}

#endif  // __IVISITOR_H__
