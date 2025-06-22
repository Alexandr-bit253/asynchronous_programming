#include <iostream>
#include <cassert>

#include "sql_query_builder.hpp"


int main() {
	AdvancedSqlSelectQueryBuilder qb;

	qb.AddColumns({ "name", "phone" });
	qb.AddFrom("students");
	qb.AddWhereGreater("id", "42");

	const auto sql = qb.BuildQuery();

	assert(sql == "SELECT name, phone FROM students WHERE id>42;");

	std::cout << sql << std::endl;

	return 0;
}