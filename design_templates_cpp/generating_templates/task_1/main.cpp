#include <iostream>
#include <cassert>

#include "sql_query_builder.hpp"


int main() {
	SqlSelectQueryBuilder qb;
	
	qb.AddColumn("name");
	qb.AddColumn("phone");
	qb.AddFrom("students");
	qb.AddWhere("id", "42");
	qb.AddWhere("name", "John");

	const auto sql = qb.BuildQuery();
	std::cout << sql << std::endl;

	assert(sql == "SELECT name, phone FROM students WHERE id=42 AND name=John;");

	return 0;
}