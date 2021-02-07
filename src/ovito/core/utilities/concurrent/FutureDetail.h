////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/core/Core.h>
#include <type_traits>

namespace Ovito {

namespace detail
{

	template<class T>
	struct is_future : std::false_type {};

	template<typename... T>
	struct is_future<Future<T...>> : std::true_type {};

	template<typename... T>
	struct is_future<SharedFuture<T...>> : std::true_type {};

	/// Part of apply() implementation below.
	template<typename F, class Tuple, std::size_t... I>
	static constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
		return f(std::get<I>(std::forward<Tuple>(t))...);
	}

	/// C++14 implementation of std::apply(), which is only available in C++17.
	template<typename F, class Tuple>
	static constexpr decltype(auto) apply_cpp14(F&& f, Tuple&& t) {
		return apply_impl(
			std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
	}

	/// Determine the return type of a continuation function object.
	template<typename FC, typename Args>
	struct continuation_func_return_type {
		using type = decltype(apply_cpp14(std::declval<FC>(), std::declval<Args>()));
	};

	/// Determines if the return type of a continuation function object is 'void'.
	template<typename FC, typename Args>
	struct is_void_continuation_func {
		static constexpr auto value = std::is_void<typename continuation_func_return_type<FC,Args>::type>::value;
	};

	/// For a value type, returns the corresponding Future<> type.
	template<typename T>
	struct future_type_from_value_type : public std::conditional<std::is_void<T>::value, Future<>, Future<T>> {};

	/// Determines the Future type that results from a continuation function.
	///
	///     Future<...> func(...)   ->   Future<...>  (automatic unwrapping)
	///               T func(...)   ->   Future<T>
	///            void func(...)   ->   Future<>
	///
	template<typename FC, typename Args>
	struct resulting_future_type : public
		std::conditional<is_future<typename continuation_func_return_type<FC,Args>::type>::value,
						typename continuation_func_return_type<FC,Args>::type,
						typename future_type_from_value_type<typename continuation_func_return_type<FC,Args>::type>::type> {};

	/// The simplest implementation of the Executor concept.
	/// The inline executor runs a work function immediately and in place.
	/// See RefTargetExecutor for another implementation of the executor concept.
	struct InlineExecutor {
		template<typename F>
		static constexpr auto createWork(F&& f) noexcept {
			return [f = std::forward<F>(f)](bool defer) mutable {
				OVITO_ASSERT_MSG(!defer, "InlineExecutor", "Execution of work scheduled with the InlineExecutor cannot be deferred.");
				std::move(f)();
			};
		}
		static constexpr TaskManager* taskManager() { return nullptr; }
	};

} // End of namespace

}	// End of namespace


