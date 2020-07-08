//
// Created by Darwin Yuan on 2020/6/7.
//

#ifndef TRANS_DSL_2_SEQUENTIALHELPER_H
#define TRANS_DSL_2_SEQUENTIALHELPER_H

#include <trans-dsl/tsl_ns.h>
#include <trans-dsl/sched/action/SchedSequential.h>
#include <trans-dsl/utils/SeqInt.h>
#include <trans-dsl/sched/concepts/SchedActionConcept.h>
#include <trans-dsl/sched/helper/VolatileSeq.h>
#include <trans-dsl/sched/helper/InlineSeqHelper.h>
#include <trans-dsl/sched/domain/TransListenerObservedAids.h>
#include <trans-dsl/sched/helper/ActionRealTypeTraits.h>
#include <cub/type-list/TypeListTransform.h>
#include <trans-dsl/utils/ThreadActionTrait.h>

TSL_NS_BEGIN

struct SchedAction;

namespace details {

   template<typename ... T_ACTIONS>
   class Sequential final {
      enum { Num_Of_Actions = sizeof...(T_ACTIONS) };
      static_assert(Num_Of_Actions >= 2, "__sequential must contain at least 2 actions");
      static_assert(Num_Of_Actions <= 50, "too many actions in a __sequential");

      ///////////////////////////////////////////////////////////////////////////////////////////

      template<typename ... T_TRANSFORMED>
      struct Base  {
         // for thread-resource-transfer
         using ThreadActionCreator = ThreadCreator_t<T_TRANSFORMED...>;

         // for inline sequential
         template<size_t N>
         using ActionType = typename inline_seq::Extractor<N, T_TRANSFORMED...>::type;
         constexpr static size_t totalNumOfActions = (inline_seq::TotalSeqActions<T_TRANSFORMED> + ... );

         template<typename ... Ts>
         using Seq = VolatileSeq<SchedAction, Ts...>;

         using CombType = inline_seq::Comb_t<Seq, T_TRANSFORMED...>;
         using type = typename CombType::type;
      };

      template<TransListenerObservedAids const& AIDs>
      struct Trait {
         template<typename T>
         using Transformer = ActionRealTypeTraits<AIDs, T, void>;

         using BaseType = CUB_NS::Transform_t<Transformer, Base, T_ACTIONS...>;
      };

   public:
      template<TransListenerObservedAids const& AIDs>
      struct ActionRealType : SchedSequential, private Trait<AIDs>::BaseType::type {
      private:
         using BaseType = typename Trait<AIDs>::BaseType;
      public:
         using ThreadActionCreator = typename BaseType::ThreadActionCreator;

         // for seq inline
         template<size_t N>
         using ActionType = typename BaseType::template ActionType<N>;
         constexpr static size_t totalActions = BaseType::CombType::totalNumOfActions;

      private:
         OVERRIDE(getNumOfActions()->SeqInt) { return Num_Of_Actions; }
         OVERRIDE(getNext(SeqInt seq) -> SchedAction*) { return BaseType::type::get(seq); }
      };
   };

   template<typename ... Ts>
   using Sequential_t = typename Sequential<Ts...>::template ActionRealType<EmptyAids>;
}

#define __sequential(...) TSL_NS::details::Sequential<__VA_ARGS__>
#define __def_sequential(...) TSL_NS::details::Sequential_t<__VA_ARGS__>

TSL_NS_END

#endif //TRANS_DSL_2_SEQUENTIALHELPER_H
