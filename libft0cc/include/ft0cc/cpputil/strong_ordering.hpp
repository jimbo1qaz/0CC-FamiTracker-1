/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * 0CC-FamiTracker is (C) 2014-2018 HertzDevil
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/. */


#pragma once

// placeholder for std::strong_ordering
// TODO: replace with operator<=>

#define ENABLE_STRONG_ORDERING(T) \
	inline bool operator==(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) == 0; \
	} \
	\
	inline bool operator!=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) != 0; \
	} \
	\
	inline bool operator<(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) < 0; \
	} \
	\
	inline bool operator<=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) <= 0; \
	} \
	\
	inline bool operator>(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) > 0; \
	} \
	\
	inline bool operator>=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) >= 0; \
	}

#define ENABLE_CX_STRONG_ORDERING(T) \
	constexpr bool operator==(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) == 0; \
	} \
	\
	constexpr bool operator!=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) != 0; \
	} \
	\
	constexpr bool operator<(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) < 0; \
	} \
	\
	constexpr bool operator<=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) <= 0; \
	} \
	\
	constexpr bool operator>(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) > 0; \
	} \
	\
	constexpr bool operator>=(const T &lhs, const T &rhs) \
		noexcept(noexcept(lhs.compare(rhs))) \
	{ \
		return lhs.compare(rhs) >= 0; \
	}
