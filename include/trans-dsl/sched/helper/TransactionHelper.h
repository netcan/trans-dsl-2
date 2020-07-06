//
// Created by Darwin Yuan on 2020/7/4.
//

#ifndef TRANS_DSL_2_TRANSACTIONHELPER_H
#define TRANS_DSL_2_TRANSACTIONHELPER_H

#include <trans-dsl/sched/helper/MultiThreadHelper.h>
#include <trans-dsl/sched/domain/MultiThreadContext.h>
#include <trans-dsl/sched/domain/TransactionContext.h>
#include <cstddef>
#include <trans-dsl/sched/domain/Event.h>
#include "AutoActionHelper.h"


TSL_NS_BEGIN

namespace details {
   template<typename ... ACTIONS>
   struct Transaction : private TransactionContext{
      static_assert(sizeof...(ACTIONS) > 0, "transaction cannot be empty");

   private:
      template<typename T, typename = void>
      struct MultiThreadTrait {
         using type = __multi_thread(T);
      };

      template<typename T>
      struct MultiThreadTrait<T, std::enable_if_t<std::is_void_v<ThreadCreator_t<T>>>> {
         using type = T;
      };

      using Action = typename MultiThreadTrait<AutoAction::SequentialTrait_t<ACTIONS...>>::type;

   public:
      using TransactionContext::updateInstanceId;
      using TransactionContext::updateUserContext;
      using TransactionContext::updateTimerInfo;
      using TransactionContext::updateListener;

      auto start() -> Status {
         return action.exec(*this);
      }

      auto startWithEvent(Event const& event) -> Status {
         Status status = action.exec(*this);
         if(status == Result::CONTINUE) {
            return action.handleEvent(*this, event);
         }
      }

      auto handleEvent(Event const& event) -> Status {
         return action.handleEvent(*this, event);
      }

      auto stop(Status cause) -> Status {
         return action.stop(*this, cause);
      }

      auto kill(Status cause) -> void {
         action.kill(*this, cause);
      }

   private:
      Action action;
   };
}

TSL_NS_END

#define __transaction(...) TSL_NS::details::Transaction<__VA_ARGS__>

#endif //TRANS_DSL_2_TRANSACTIONHELPER_H
