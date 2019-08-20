#pragma once

#include <jsi/jsi.h>

namespace typedjsi {

template<typename T>
struct MethodReturn { typedef T type; };

template <typename Type>
typename MethodReturn<Type>::type get(facebook::jsi::Runtime& runtime, const facebook::jsi::Value& value) noexcept;

// Can't partially specialize functions ; more template trickery needed
/*template<>
template <typename Type>
typename MethodReturn<std::vector<Type>>::type get<std::vector<Type>>(facebook::jsi::Runtime& runtime, const facebook::jsi::Value& value) noexcept
{
	assert(value.isObject());
	auto obj = value.getObject(runtime);
	assert(obj.isArray(runtime));
	auto arr = obj.getArray(runtime);
	std::vector<Type> vec(arr.size());
	for (size_t i = 0; i < arr.size(); i++)
	{
		auto val = arr.getValueAtIndex(runtime, i);
		vec[i] = get<Type>(runtime, val);
	}
}*/

template <typename Type>
typename MethodReturn<std::vector<Type>>::type getarr(facebook::jsi::Runtime& runtime, const facebook::jsi::Value& value) noexcept
{
	assert(value.isObject());
	auto obj = value.getObject(runtime);
	assert(obj.isArray(runtime));
	auto arr = obj.getArray(runtime);
	auto arr_size = arr.size(runtime);
	std::vector<Type> vec(arr_size);
	for (size_t i = 0; i < arr_size; i++)
	{
		auto val = arr.getValueAtIndex(runtime, i);
		vec[i] = get<Type>(runtime, val);
	}

	return vec;
}

template <typename Type>
facebook::jsi::Array getJSIArr(facebook::jsi::Runtime& runtime, const std::vector<Type>& vec) noexcept
{
	facebook::jsi::Array arr(runtime, vec.size());
	for (size_t i = 0; i < vec.size(); i++)
	{
		arr.setValueAtIndex(runtime, i, vec[i]);
	}

	return arr;
}

} // namespace typedjsi
