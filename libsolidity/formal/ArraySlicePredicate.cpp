/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0

#include <libsolidity/formal/ArraySlicePredicate.h>

#include <liblangutil/Exceptions.h>

using namespace std;
using namespace solidity;
using namespace solidity::smtutil;
using namespace solidity::frontend;
using namespace solidity::frontend::smt;

map<string, ArraySlicePredicate::SliceData> ArraySlicePredicate::m_slicePredicates;

pair<bool, ArraySlicePredicate::SliceData const&> ArraySlicePredicate::create(SortPointer _sort, EncodingContext& _context)
{
	solAssert(_sort->kind == Kind::Tuple, "");
	auto tupleSort = dynamic_pointer_cast<TupleSort>(_sort);
	solAssert(tupleSort, "");

	auto tupleName = tupleSort->name;
	if (m_slicePredicates.count(tupleName))
		return {true, m_slicePredicates.at(tupleName)};

	auto sort = tupleSort->components.at(0);
	solAssert(sort->kind == Kind::Array, "");

	smt::SymbolicArrayVariable a{sort, "a_" + tupleName, _context };
	smt::SymbolicArrayVariable b{sort, "b_" + tupleName, _context};
	smt::SymbolicIntVariable start{TypeProvider::uint256(), TypeProvider::uint256(), "start_" + tupleName, _context};
	smt::SymbolicIntVariable end{TypeProvider::uint256(), TypeProvider::uint256(), "end_" + tupleName, _context };
	smt::SymbolicIntVariable i{TypeProvider::uint256(), TypeProvider::uint256(), "i_" + tupleName, _context};

	vector<SortPointer> domain{sort, sort, start.sort(), end.sort()};
	auto sliceSort = make_shared<FunctionSort>(domain, SortProvider::boolSort);
	Predicate const& slice = *Predicate::create(sliceSort, "array_slice_" + tupleName, _context);

	domain.emplace_back(i.sort());
	auto predSort = make_shared<FunctionSort>(domain, SortProvider::boolSort);
	Predicate const& header = *Predicate::create(predSort, "array_slice_header_" + tupleName, _context);
	Predicate const& loop = *Predicate::create(predSort, "array_slice_loop_" + tupleName, _context);

	auto rule1 = smtutil::Expression::implies(
		end.currentValue() > start.currentValue(),
		header({a.elements(), b.elements(), start.currentValue(), end.currentValue(), 0})
	);

	auto rule2 = smtutil::Expression::implies(
		header({a.elements(), b.elements(), start.currentValue(), end.currentValue(), i.currentValue()}) && i.currentValue() >= end.currentValue() - start.currentValue(),
		slice({a.elements(), b.elements(), start.currentValue(), end.currentValue()})
	);

	auto rule3 = smtutil::Expression::implies(
		header({a.elements(), b.elements(), start.currentValue(), end.currentValue(), i.currentValue()}) && i.currentValue() >= 0 && i.currentValue() < end.currentValue() - start.currentValue(),
		loop({a.elements(), b.elements(), start.currentValue(), end.currentValue(), i.currentValue()})
	);

	auto rule4 = smtutil::Expression::implies(
		loop({a.elements(), b.elements(), start.currentValue(), end.currentValue(), i.currentValue()}) && smtutil::Expression::select(b.elements(), i.currentValue()) == smtutil::Expression::select(a.elements(), start.currentValue() + i.currentValue()),
		header({a.elements(), b.elements(), start.currentValue(), end.currentValue(), i.currentValue() + 1})
	);

	return {false, m_slicePredicates[tupleName] = {
		{&slice, &header, &loop},
		{move(rule1), move(rule2), move(rule3), move(rule4)}
	}};
}
