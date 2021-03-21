/*
 * TraceValue.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FLOW_TRACE_VALUE__
#define __FLOW_TRACE_VALUE__
#pragma once

#include <string>

struct TraceBool {
	bool value;

	TraceBool() : value(false) {}
	TraceBool(bool value) : value(value) {}

	std::string toString() const;

	static constexpr size_t heapSize() { return 0; }
	static constexpr void truncate(int) {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, value);
	}
};

struct TraceString final {
	std::string value;

	TraceString() = default;
	TraceString(std::string const& value) : value(value) {}
	std::string toString() const { return value; }

	size_t heapSize() const { return value.size(); }

	void truncate(int maxFieldLength) {
		if (value.size() > maxFieldLength) {
			value = value.substr(0, maxFieldLength) + "...";
		}
	}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, value);
	}
};

struct TraceNumeric final {
	std::string value;

	TraceNumeric() = default;
	TraceNumeric(std::string const& value) : value(value) {}
	std::string toString() const { return value; }

	size_t heapSize() const { return value.size(); }
	static constexpr void truncate(int) {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, value);
	}
};

struct TraceCounter {
	double rate;
	double roughness;
	int64_t value;

	TraceCounter() : rate(0.0), roughness(0.0), value(0) {}
	TraceCounter(double rate, double roughness, int64_t value) : rate(rate), roughness(roughness), value(value) {}

	std::string toString() const;
	static constexpr size_t heapSize() { return 0; }
	static constexpr void truncate(int) {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, rate, roughness, value);
	}
};

struct TraceVector {
	int maxFieldLength{ -1 };
	std::vector<struct TraceValue> values;

	TraceVector() = default;
	void push_back(TraceValue&&);
	size_t heapSize() const;
	void truncate(int maxFieldLength);
	std::string toString() const;

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, values);
	}
};

struct TraceValue {
	std::variant<TraceString, TraceBool, TraceCounter, TraceNumeric, TraceVector> value;
	template <class T, class... Args>
	explicit TraceValue(std::in_place_type_t<T> typeId, Args&&... args) : value(typeId, std::forward<Args>(args)...) {}

public:
	TraceValue(std::string const& value = "") : TraceValue(std::in_place_type<TraceString>, std::move(value)) {}

	template <class T, class... Args>
	static TraceValue create(Args&&... args) {
		return TraceValue(std::in_place_type<T>, std::forward<Args>(args)...);
	}
	template <class T>
	T const& get() const& {
		return std::get<T>(value);
	}
	template <class T>
	T& get() & {
		return std::get<T>(value);
	}
	template <class T>
	T&& get() && {
		return std::get<T>(std::move(value));
	}

	std::string toString() const {
		return std::visit([](const auto& val) { return val.toString(); }, value);
	}

	template <class Formatter>
	std::string format(Formatter const& f) const {
		return std::visit(f, value);
	}

	size_t size() const {
		return sizeof(TraceValue) + std::visit([](auto const& v) { return v.heapSize(); }, value);
	}

	void truncate(int maxFieldLength) {
		std::visit([maxFieldLength](auto& v) { v.truncate(maxFieldLength); }, value);
	}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, value);
	}
};

#endif
