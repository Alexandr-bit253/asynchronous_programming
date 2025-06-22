#include <iostream>
#include <cassert>

#include "sql_query_builder.hpp"


int main() {
	SqlSelectQueryBuilder qb;

	qb.AddColumns({ "id", "name", "email" });
	qb.AddColumn("phone").AddColumn("address");
	qb.AddFrom("students");
	std::map<std::string, std::string> filters = {
		{ "status", "active" },
		{ "grade",  "A" }
	};
	qb.AddWhere(filters);
	qb.AddWhere("id", "42");

	const auto sql = qb.BuildQuery();
	
	std::cout << sql << std::endl;

	return 0;
}