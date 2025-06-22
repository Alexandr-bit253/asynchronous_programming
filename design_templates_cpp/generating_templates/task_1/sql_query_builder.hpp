#pragma once

#include <string>
#include <vector>
#include <sstream>


class SqlSelectQueryBuilder {
private:
	std::vector<std::string> columns;
	std::string table;
	std::vector<std::string> where_clauses;

public:
	SqlSelectQueryBuilder& AddColumn(const std::string& column) {
		columns.push_back(column);
		return *this;
	}

	SqlSelectQueryBuilder& AddFrom(const std::string& table) {
		this->table = table;
		return *this;
	}

	SqlSelectQueryBuilder& AddWhere(const std::string& column, const std::string& value) {
		where_clauses.push_back(column + "=" + value);
		return *this;
	}

	std::string BuildQuery() const {
		std::ostringstream ss;

		// select
		if (columns.empty()) {
			ss << "SELECT *";
		}
		else {
			ss << "SELECT ";
			for (size_t i = 0; i < columns.size(); ++i) {
				if (i)
					ss << ", ";
				ss << columns[i];
			}
		}

		// from
		assert(!table.empty() && "table not specified - be sure to call AddFrom()");
		ss << " FROM " << table;

		// where
		if (!where_clauses.empty()) {
			ss << " WHERE ";
			for (size_t i = 0; i < where_clauses.size(); ++i) {
				if (i)
					ss << " AND ";
				ss << where_clauses[i];
			}
		}

		ss << ";";
		return ss.str();
	}
};