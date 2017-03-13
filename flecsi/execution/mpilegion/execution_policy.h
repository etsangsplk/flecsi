/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
 * /@@/////  /@@          @@////@@ @@////// /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
 * //       ///  //////   //////  ////////  //
 *
 * Copyright (c) 2016 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~--------------------------------------------------------------------------~*/

#ifndef flecsi_mpilegion_execution_policy_h
#define flecsi_mpilegion_execution_policy_h

#include <functional>
#include <tuple>
#include <type_traits>

#include "flecsi/utils/const_string.h"
#include "flecsi/utils/logging.h"
#include "flecsi/execution/context.h"
#include "flecsi/execution/legion/future.h"
#include "flecsi/execution/common/processor.h"
#include "flecsi/execution/common/task_hash.h"
#include "flecsi/execution/mpilegion/context_policy.h"
#include "flecsi/execution/legion/task_wrapper.h"
#include "flecsi/execution/task_ids.h"
#include "flecsi/data/data_handle.h"

///
/// \file mpilegion/execution_policy.h
/// \authors bergen, demeshko, nickm
/// \date Initial file creation: Nov 15, 2015
///

namespace flecsi {
namespace execution {

  template<size_t I, typename T, typename L>
  struct handle_task_args__{
    static size_t walk(T& t, L& l, size_t& region){
      handle_(std::get<std::tuple_size<T>::value - I>(t), l, region);
      return handle_task_args__<I - 1, T, L>::walk(t, l, region);
    }

    template<typename S, size_t PS, size_t PT>
    static void handle_(flecsi::data_handle_t<S, PS, PT>& h,
                        L& l,
                        size_t& region){

      flecsi::execution::field_ids_t & fid_t = 
        flecsi::execution::field_ids_t::instance();

      switch(PS){
        case size_t(data::privilege::none):
          assert(false && 
                 "no privileges found on task arg while generating "
                 "region requirements");
          break;
        case size_t(data::privilege::ro):{
          RegionRequirement rr(h.lr, READ_ONLY, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
        case size_t(data::privilege::wd):{
          RegionRequirement rr(h.lr, WRITE_DISCARD, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
        case size_t(data::privilege::rw):{
          RegionRequirement rr(h.lr, READ_WRITE, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
      }
    }

    template<typename R>
    static
    typename std::enable_if_t<!std::is_base_of<flecsi::data_handle_base, R>::
      value>
    handle_(R&, L&, size_t&){}
  };

  template<typename T, typename L>
  struct handle_task_args__<0, T, L>{
    static size_t walk(T& t, L& l, size_t& region){
      return 0;
    }
  };

  template<size_t I, typename T, typename L>
  struct handle_index_task_args__{
    static size_t walk(Legion::Runtime* runtime, Legion::Context ctx,
                       T& t, L& l, size_t& region){
      handle_(runtime, ctx,
              std::get<std::tuple_size<T>::value - I>(t), l, region);
      return handle_task_args__<I - 1, T, L>::walk(t, l, region);
    }

    template<typename S, size_t PS, size_t PT>
    static void handle_(Legion::Runtime* runtime, Legion::Context ctx,
                        flecsi::data_handle_t<S, PS, PT>& h,
                        L& l,
                        size_t& region){

      flecsi::execution::field_ids_t & fid_t = 
        flecsi::execution::field_ids_t::instance();

      //h.region = region++;

      LogicalPartition lp =
        runtime->get_logical_partition(ctx, h.lr, h.exclusive_ip);

      switch(PS){
        case size_t(data::privilege::none):
          assert(false && 
                 "no privileges found on task arg while generating "
                 "region requirements");
          break;
        case size_t(data::privilege::ro):{
          RegionRequirement rr(lp, 0, READ_ONLY, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
        case size_t(data::privilege::wd):{
          RegionRequirement rr(lp, 0, WRITE_DISCARD, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
        case size_t(data::privilege::rw):{
          RegionRequirement rr(lp, 0, READ_WRITE, EXCLUSIVE, h.lr);
          rr.add_field(fid_t.fid_value);
          l.add_region_requirement(rr);
          break;
        }
      }
    }

    template<typename R>
    static
    typename std::enable_if_t<!std::is_base_of<flecsi::data_handle_base, R>::
      value>
    handle_(Legion::Runtime* runtime, Legion::Context ctx, R&, L&, size_t&){}
  };

  template<typename T, typename L>
  struct handle_index_task_args__<0, T, L>{
    static size_t walk(Legion::Runtime* runtime, Legion::Context ctx,
                       T& t, L& l, size_t& region){
      return 0;
    }
  };

//----------------------------------------------------------------------------//
// Execution policy.
//----------------------------------------------------------------------------//

///
/// \struct mpilegion_execution_policy mpilegion/execution_policy.h
/// \brief mpilegion_execution_policy provides...
///
struct mpilegion_execution_policy_t
{
  template<typename R>
  using future__ = legion_future__<R>;

  /*--------------------------------------------------------------------------*
   * Task interface.
   *--------------------------------------------------------------------------*/

  // FIXME: add task type (leaf, inner, etc...)
  ///
  /// register FLeCSI task depending on the tasks's processor and launch types
  ///
  template<
    typename R,
    typename A
  >
  static
  bool
  register_task(
    task_hash_key_t key
  )
  {

    switch(key.processor()) {

      case loc:
      {
        switch(key.launch()) {

          case single:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<loc, 1, 0, R, A>::register_task);
          } // case single

          case index:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<loc, 0, 1, R, A>::register_task);
          } // case single

          case any:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<loc, 1, 1, R, A>::register_task);
          } // case single

          default:
            throw std::runtime_error("task type is not specified or incorrect,\
                   please chose one from single, index, any");

        } // switch
      } // case loc

      case toc:
      {
        switch(key.launch()) {

          case single:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<toc, 1, 0, R, A>::register_task);
          } // case single

          case index:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<toc, 0, 1, R, A>::register_task);
          }

          case any:
          {
          return context_t::instance().register_task(key,
            legion_task_wrapper__<toc, 1, 1, R, A>::register_task);
          } // case any
  
          throw std::runtime_error("task type is not specified or incorrect,\
                please chose one from single, index, any");

        } // switch
      } // case toc

      case mpi:
      {
        return context_t::instance().register_task(key,
          legion_task_wrapper__<mpi, 0, 1, R, A>::register_task);
      } // case mpi

      default:
        throw std::runtime_error("unsupported processor type");
    } // switch
 
  } // register_task

  ///
  /// Execute FLeCSI task. Depending on runtime specified for task to be 
  /// executed at (part of the processor_type) 
  /// In case processor_type is "mpi" we first need to execute 
  /// index_launch task that will switch from Legion to MPI runtime and 
  /// pass a function pointer to every MPI thread. Then, on the MPI side, we
  /// execute the function and come back to Legion runtime.
  /// In case of processoe_type != "mpi" we launch the task through
  /// the TaskLauncher or IndexLauncher depending on the launch_type
  ///

  template<
    typename R,
    typename T,
    typename...As
  >
  static
  decltype(auto)
  execute_task(
    task_hash_key_t key,
    size_t parent,
    T user_task_handle,
    As && ... user_task_args
  )
  {
    using namespace Legion;

    context_t & context_ = context_t::instance();

    auto user_task_args_tuple = std::make_tuple(user_task_args...);
    using user_task_args_tuple_t = decltype( user_task_args_tuple );
    
    using task_args_t = 
      legion_task_args__<R, typename T::args_t, user_task_args_tuple_t>;

    // We can't use std::forward or && references here because
    // the calling state is not guarunteed to exist when the
    // task is invoked, i.e., we have to use copies...
    task_args_t task_args(user_task_handle, user_task_args_tuple);

    if(key.processor() == mpi) {
      // Executing Legion task that pass function pointer and 
      // its arguments to every MPI thread
      LegionRuntime::HighLevel::ArgumentMap arg_map;

      LegionRuntime::HighLevel::IndexLauncher index_launcher(
        context_.task_id(key),

      LegionRuntime::HighLevel::Domain::from_rect<1>(
        context_.interop_helper_.all_processes_),
        TaskArgument(&task_args, sizeof(task_args_t)), arg_map);
      if (parent == utils::const_string_t{"specialization_driver"}.hash())
         index_launcher.tag = MAPPER_FORCE_RANK_MATCH;
      else{
        clog_fatal("calling MPI task from sub-task or driver is not currently supported");
        // index_launcher.tag = MAPPER_SUBRANK_MATCH;
      }

      LegionRuntime::HighLevel::FutureMap fm1 =
        context_.runtime(parent)->execute_index_space(context_.context(parent),
        index_launcher);

      fm1.wait_all_results();

      context_.interop_helper_.handoff_to_mpi(context_.context(parent),
         context_.runtime(parent));

      // mpi task is running here
      //TOFIX:: need to return future
      //      flecsi::execution::future_t future = 
         context_.interop_helper_.wait_on_mpi(context_.context(parent),
                            context_.runtime(parent));
     context_.interop_helper_.unset_call_mpi(context_.context(parent),
                            context_.runtime(parent));

      return legion_future__<R>(mpitask_t{});
    }
    else {

      switch(key.launch()) {

        case single:
        {
          TaskLauncher task_launcher(context_.task_id(key),
            TaskArgument(&task_args, sizeof(task_args_t)));

          size_t region = 0;

          handle_task_args__<std::tuple_size<user_task_args_tuple_t>::value, user_task_args_tuple_t, Legion::TaskLauncher>::walk(
            user_task_args_tuple, task_launcher, region);

          auto future = context_.runtime(parent)->execute_task(
            context_.context(parent), task_launcher);

          return legion_future__<R>(future);
        } // single

        case index:
        {
          LegionRuntime::HighLevel::ArgumentMap arg_map;
          LegionRuntime::HighLevel::IndexLauncher index_launcher(
            context_.task_id(key),
            LegionRuntime::HighLevel::Domain::from_rect<1>(
              context_.interop_helper_.all_processes_),
            TaskArgument(&task_args, sizeof(task_args_t)), arg_map);

          if (parent == utils::const_string_t{"specialization_driver"}.hash())
            index_launcher.tag = MAPPER_FORCE_RANK_MATCH;
          else{}

          auto runtime = context_.runtime(parent);
          auto ctx = context_.context(parent);

          size_t region = 0;

          handle_index_task_args__<std::tuple_size<user_task_args_tuple_t>::value, user_task_args_tuple_t, Legion::IndexLauncher>::walk(
            runtime, ctx, user_task_args_tuple, index_launcher, region);

          auto future = runtime->execute_index_space(ctx, index_launcher);

          return legion_future__<R>(future);
        } // index

        default:
        {
          throw std::runtime_error("the task can be executed \
                      only as single or index task");
        }

      } // switch
    } // if
  } // execute_task

  /*--------------------------------------------------------------------------*
   * Function interface.
   *--------------------------------------------------------------------------*/

  ///
  /// This method registers a user function with the current
  ///  execution context.
  ///  
  ///  \param key The function identifier.
  ///  \param user_function A reference to the user function as a std::function.
  ///
  ///  \return A boolean value indicating whether or not the function was
  ///    successfully registered.
  ///
  template<
    typename R,
    typename ... As
  >
  static
  bool
  register_function(
    const utils::const_string_t & key,
    std::function<R(As ...)> & user_function
  )
  {
    return context_t::instance().register_function(key, user_function);
  } // register_function

  ///
  ///  This method looks up a function from the \e handle argument
  ///  and executes the associated it with the provided \e args arguments.
  ///  
  ///  \param handle The function handle to execute.
  ///  \param args A variadic argument list of the function parameters.
  ///
  ///  \return The return type of the provided function handle.
  ///
  template<
    typename T,
    typename ... As
  >
  static
  decltype(auto)
  execute_function(
    T & handle,
    As && ... args
  )
  {
    auto t = std::forward_as_tuple(args ...);
    return handle(context_t::instance().function(handle.key), std::move(t));
  } // execute_function

}; // struct mpilegion_execution_policy_t

} // namespace execution 
} // namespace flecsi

#endif // flecsi_mpilegion_execution_policy_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
